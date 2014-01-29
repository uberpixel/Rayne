//
//  RNLight.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLight.h"
#include "RNWorld.h"
#include "RNCamera.h"
#include "RNResourceCoordinator.h"
#include "RNLightManager.h"
#include "RNCameraInternal.h"
#include "RNOpenGLQueue.h"

namespace RN
{
	RNDeclareMeta(Light)
	
	Light::Light(Type lighttype) :
		_lightType(lighttype),
		_color("color", Color(1.0f), &Light::GetColor, &Light::SetColor),
		_intensity("intensity", 10.0f, &Light::GetIntensity, &Light::SetIntensity),
		_range("range", 10.0f, &Light::GetRange, &Light::SetRange),
		_angle("angle", 45.0f, &Light::GetAngle, &Light::SetAngle),
		_angleCos(0.797),
		_shadowTarget(nullptr),
		_suppressShadows(false)
	{
		AddObservables({ &_color, &_intensity, &_range, &_angle });
		
		SetPriority(SceneNode::Priority::UpdateLate);
		SetCollisionGroup(25);
		
		ReCalculateColor();
	}
	
	Light::~Light()
	{
		RemoveShadowCameras();
	}
	
	bool Light::IsVisibleInCamera(Camera *camera)
	{
		if(_lightType == Type::DirectionalLight)
			return true;
		
		return SceneNode::IsVisibleInCamera(camera);
	}
	
	void Light::Render(Renderer *renderer, Camera *camera)
	{
		LightManager *manager = camera->GetLightManager();
		
		if(manager)
			manager->AddLight(this);
	}
	
	void Light::SetRange(float range)
	{
		_range = range;
		
		SetBoundingSphere(Sphere(Vector3(), range));
		SetBoundingBox(AABB(Vector3(), range), false);
	}
	
	void Light::SetColor(const Color& color)
	{
		_color = color;
		ReCalculateColor();
	}
	
	void Light::SetIntensity(float intensity)
	{
		_intensity = intensity;
		ReCalculateColor();
	}
	
	void Light::SetAngle(float angle)
	{
		_angle = angle;
		_angleCos = cosf(angle*k::DegToRad);
	}
	
	void Light::RemoveShadowCameras()
	{
		_shadowDepthCameras.Enumerate<Camera>([&](Camera *camera, size_t index, bool &stop) {
			RemoveDependency(camera);
		});
		
		_shadowDepthCameras.RemoveAllObjects();
		
		if(_shadowTarget)
		{
			RemoveDependency(_shadowTarget);
			_shadowTarget->Release();
			_shadowTarget = nullptr;
		}
	}
	
	bool Light::ActivateShadows(const ShadowParameter &parameter)
	{
		_shadowParameter = parameter;
		
		switch(_lightType)
		{
			case Type::PointLight:
				return ActivatePointShadows();
				
			case Type::SpotLight:
				return ActivateSpotShadows();
				
			case Type::DirectionalLight:
				return ActivateDirectionalShadows();

			default:
				return false;
		}
	}
	
	void Light::DeactivateShadows()
	{
		RemoveShadowCameras();
	}
	
	void Light::SetSuppressShadows(bool suppress)
	{
		_suppressShadows = suppress;
		
		if(_suppressShadows)
		{
			_shadowDepthCameras.Enumerate<Camera>([&](Camera *camera, size_t index, bool &stop) {
				camera->SetFlags(camera->GetFlags()|Camera::Flags::NoRender);
			});
		}
		else
		{
			_shadowDepthCameras.Enumerate<Camera>([&](Camera *camera, size_t index, bool &stop) {
				camera->SetFlags(camera->GetFlags()&~Camera::Flags::NoRender);
			});
		}
	}
	
	void Light::UpdateShadowParameters(const ShadowParameter &parameter)
	{
		RN_ASSERT(HasShadows(), "Shadows need to be activated in order to update their parameters and may not be suppressed!");
		
		if(_shadowParameter.resolution != parameter.resolution || _shadowParameter.splits.size() != parameter.splits.size())
		{
			DeactivateShadows();
			ActivateShadows(parameter);
		}
		else
		{
			if(parameter.shadowTarget != _shadowTarget)
			{
				if(_shadowTarget)
				{
					RemoveDependency(_shadowTarget);
					_shadowTarget->Release();
				}
				
				if(parameter.shadowTarget)
				{
					_shadowTarget = parameter.shadowTarget;
					_shadowTarget->Retain();
					AddDependency(_shadowTarget);
				}
			}
			
			_shadowDepthCameras.Enumerate<Camera>([&](Camera *camera, size_t index, bool &stop) {
				camera->GetMaterial()->SetPolygonOffsetFactor(parameter.splits[index].biasFactor);
				camera->GetMaterial()->SetPolygonOffsetUnits(parameter.splits[index].biasUnits);
			});
			
			_shadowParameter = parameter;
		}
	}
	
	
	bool Light::ActivateDirectionalShadows()
	{
		if(_shadowDepthCameras.GetCount() > 0)
			DeactivateShadows();
		
		RN_ASSERT(_shadowParameter.shadowTarget, "Directional shadows need the shadowTarget to be set to a valid value!");
		
		RN_ASSERT(_shadowParameter.splits.size() > 0, "The shadow parameter for directional lights needs one or more splits!");
		
		if(_shadowTarget)
		{
			RemoveDependency(_shadowTarget);
			_shadowTarget->Release();
		}
		
		_shadowTarget = _shadowParameter.shadowTarget;
		_shadowTarget->Retain();
		AddDependency(_shadowTarget);
		
		Texture::Parameter textureParameter;
		textureParameter.wrapMode = Texture::WrapMode::Clamp;
		textureParameter.filter = Texture::Filter::Linear;
		textureParameter.format = Texture::Format::Depth24I;
		textureParameter.depthCompare = true;
		textureParameter.maxMipMaps = 0;
		
		Texture2DArray *depthtex = new Texture2DArray(textureParameter);
		depthtex->SetSize(_shadowParameter.resolution, _shadowParameter.resolution, _shadowParameter.splits.size());
		depthtex->Autorelease();
		
		Shader *depthShader = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyDirectionalShadowDepthShader, nullptr);
		
		_shadowCameraMatrices.clear();
		
		for(uint32 i = 0; i < _shadowParameter.splits.size(); i++)
		{
			_shadowCameraMatrices.push_back(Matrix());
			
			Material *depthMaterial = new Material(depthShader);
			depthMaterial->SetPolygonOffset(true);
			depthMaterial->SetPolygonOffsetFactor(_shadowParameter.splits[i].biasFactor);
			depthMaterial->SetPolygonOffsetUnits(_shadowParameter.splits[i].biasUnits);
			
			RenderStorage *storage = new RenderStorage(RenderStorage::BufferFormatDepth, 0, 1.0f);
			storage->SetDepthTarget(depthtex, i);
			
			Camera *tempcam = new Camera(Vector2(_shadowParameter.resolution), storage, Camera::Flags::UpdateAspect | Camera::Flags::UpdateStorageFrame | Camera::Flags::Orthogonal | Camera::Flags::NoFlush, 1.0f);
			tempcam->SetMaterial(depthMaterial);
			tempcam->SetLODCamera(_shadowTarget);
			tempcam->SetLightManager(nullptr);
			tempcam->SetPriority(kRNShadowCameraPriority);
			tempcam->SetClipNear(1.0f);

			_shadowDepthCameras.AddObject(tempcam);
			AddDependency(tempcam);
			
			tempcam->Release();
			storage->Release();
			
			try
			{
				OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
					storage->BindAndUpdateBuffer();
				}, true);
			}
			catch(Exception e)
			{
				RemoveShadowCameras();
				return false;
			}
		}
		
		return true;
	}
	
	bool Light::ActivatePointShadows()
	{
		if(_shadowDepthCameras.GetCount() > 0)
			DeactivateShadows();
		
		RN_ASSERT(_shadowParameter.splits.size() == 1, "The shadow parameter for point lights needs exactly one split!");
		
		Texture::Parameter textureParameter;
		textureParameter.wrapMode = Texture::WrapMode::Repeat;
		textureParameter.filter = Texture::Filter::Nearest;
		textureParameter.format = Texture::Format::Depth24I;
		textureParameter.depthCompare = false;
		textureParameter.maxMipMaps = 0;
		
		Texture *depthtex = (new TextureCubeMap(textureParameter))->Autorelease();
		
		Shader   *depthShader = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyPointShadowDepthShader, nullptr);
		Material *depthMaterial = (new Material(depthShader))->Autorelease();
		depthMaterial->SetPolygonOffset(true);
		depthMaterial->SetPolygonOffsetFactor(_shadowParameter.splits[0].biasFactor);
		depthMaterial->SetPolygonOffsetUnits(_shadowParameter.splits[0].biasUnits);
		
		RenderStorage *storage = new RenderStorage(RenderStorage::BufferFormatDepth, 0, 1.0f);
		storage->SetDepthTarget(depthtex, -1);
		
		RN::Camera *shadowcam = new CubemapCamera(Vector2(_shadowParameter.resolution), storage, Camera::Flags::UpdateAspect | Camera::Flags::UpdateStorageFrame | Camera::Flags::NoFlush, 1.0f);
		shadowcam->Retain();
		shadowcam->Autorelease();
		shadowcam->SetMaterial(depthMaterial);
		shadowcam->SetPriority(kRNShadowCameraPriority);
		shadowcam->SetClipNear(0.01f);
		shadowcam->SetClipFar(_range);
		shadowcam->SetFOV(90.0f);
		shadowcam->SetLightManager(nullptr);
		shadowcam->SetWorldRotation(Vector3(0.0f, 0.0f, 0.0f));
		
		_shadowDepthCameras.AddObject(shadowcam);
		AddDependency(shadowcam);
		storage->Release();
		
		try
		{
			OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
				storage->BindAndUpdateBuffer();
			}, true);
		}
		catch(Exception e)
		{
			RemoveShadowCameras();
			return false;
		}
		
		return true;
	}
	
	bool Light::ActivateSpotShadows()
	{
		if(_shadowDepthCameras.GetCount() > 0)
			DeactivateShadows();
		
		RN_ASSERT(_shadowParameter.splits.size() == 1, "The shadow parameter for spot lights needs exactly one split!");
		
		Texture::Parameter textureParameter;
		textureParameter.wrapMode = Texture::WrapMode::Clamp;
		textureParameter.filter = Texture::Filter::Linear;
		textureParameter.format = Texture::Format::Depth24I;
		textureParameter.depthCompare = true;
		textureParameter.maxMipMaps = 0;
		
		Texture *depthtex = new Texture2D(textureParameter);
		depthtex->Autorelease();
		
		Shader   *depthShader = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyDirectionalShadowDepthShader, nullptr);
		Material *depthMaterial = (new Material(depthShader))->Autorelease();
		depthMaterial->SetPolygonOffset(true);
		depthMaterial->SetPolygonOffsetFactor(_shadowParameter.splits[0].biasFactor);
		depthMaterial->SetPolygonOffsetUnits(_shadowParameter.splits[0].biasUnits);
		
		RenderStorage *storage = new RenderStorage(RenderStorage::BufferFormatDepth, 0, 1.0f);
		storage->SetDepthTarget(depthtex, -1);
		
		RN::Camera *shadowcam = new Camera(Vector2(_shadowParameter.resolution), storage, Camera::Flags::UpdateAspect | Camera::Flags::UpdateStorageFrame | Camera::Flags::NoFlush, 1.0f);
		shadowcam->Retain();
		shadowcam->Autorelease();
		shadowcam->SetMaterial(depthMaterial);
		shadowcam->SetPriority(kRNShadowCameraPriority);
		shadowcam->SetClipNear(0.01f);
		shadowcam->SetClipFar(_range);
		shadowcam->SetFOV(_angle * 2.0f);
		shadowcam->SetLightManager(nullptr);
		shadowcam->SetWorldRotation(Vector3(0.0f, 0.0f, 0.0f));
		
		_shadowDepthCameras.AddObject(shadowcam);
		AddDependency(shadowcam);
		storage->Release();
		
		try
		{
			OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
				storage->BindAndUpdateBuffer();
			}, true);
		}
		catch(Exception e)
		{
			RemoveShadowCameras();
			return false;
		}
		
		return true;
	}
	
	
	void Light::Update(float delta)
	{
		SceneNode::Update(delta);
		
		if(_suppressShadows || _shadowDepthCameras.GetCount() == 0)
			return;
		
		if(_lightType == Type::DirectionalLight)
		{
			if(_shadowTarget)
			{
				float near = _shadowTarget->GetClipNear();
				float far;
				
				for(uint32 i = 0; i < _shadowParameter.splits.size(); i++)
				{
					if(((GetLastFrame()+_shadowParameter.splits[i].updateOffset) % _shadowParameter.splits[i].updateInterval) == 0)
					{
						Camera *cam = _shadowDepthCameras.GetObjectAtIndex<Camera>(i);
						cam->SetFlags(cam->GetFlags()&(~Camera::Flags::NoRender));
						cam->SetClearMask(Camera::ClearMask::Depth);
					}
					else
					{
						Camera *cam = _shadowDepthCameras.GetObjectAtIndex<Camera>(i);
						cam->SetFlags(cam->GetFlags()|Camera::Flags::NoRender);
						cam->SetClearMask(0);
						continue;
					}
					
					float linear = _shadowTarget->GetClipNear() + (_shadowTarget->GetClipFar() - _shadowTarget->GetClipNear())*(i+1.0f) / float(_shadowParameter.splits.size());
					float log = _shadowTarget->GetClipNear() * powf(_shadowTarget->GetClipFar() / _shadowTarget->GetClipNear(), (i+1.0f) / float(_shadowParameter.splits.size()));
					far = linear*_shadowParameter.distanceBlendFactor+log*(1.0f-_shadowParameter.distanceBlendFactor);
					
					Camera *tempcam = _shadowDepthCameras.GetObjectAtIndex<Camera>(i);
					tempcam->SetWorldRotation(GetWorldRotation());
					
					_shadowCameraMatrices[i] = std::move(tempcam->MakeShadowSplit(_shadowTarget, this, near, far));
					
					near = far;
				}
			}
		}
		else if(_lightType == Type::SpotLight)
		{
			RN::Camera *shadowcam = static_cast<RN::Camera*>(_shadowDepthCameras.GetFirstObject());
			
			shadowcam->SetWorldPosition(GetWorldPosition());
			shadowcam->SetWorldRotation(GetWorldRotation());
			shadowcam->SetClipFar(_range);
			shadowcam->SetFOV(_angle * 2.0f);
			
			_shadowCameraMatrices.clear();
			_shadowCameraMatrices.emplace_back(shadowcam->GetProjectionMatrix() * shadowcam->GetViewMatrix());
		}
		else
		{
			RN::Camera *shadowcam = static_cast<RN::Camera*>(_shadowDepthCameras.GetFirstObject());
			shadowcam->SetWorldPosition(GetWorldPosition());
			shadowcam->SetClipFar(_range);
		}
	}

	void Light::ReCalculateColor()
	{
		_finalColor = Vector3(_color->r, _color->g, _color->b);
		_finalColor *= (float)_intensity;
	}
}
