//
//  RNLight.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLight.h"
#include "RNCamera.h"
#include "../Rendering/RNRenderer.h"
#include "../Rendering/RNFramebuffer.h"
#include "RNScene.h"

namespace RN
{
	RNDefineMeta(Light, SceneNode)
	
	Light::Light(Type lighttype) :
		_lightType(lighttype),
		_color(Color::White()),
		_intensity(10.0f),
		_range(10.0f),
		_angle(45.0f),
		_angleCos(0.797f),
		_shadowTarget(nullptr),
		_shadowDepthTexture(nullptr),
		_suppressShadows(false)
	{
		SetBoundingSphere(Sphere(Vector3(), 1.0f));
		SetBoundingBox(AABB(Vector3(), 1.0f), false);
		
		SetPriority(SceneNode::Priority::UpdateLate);
		SetCollisionGroup(25);
		
		ReCalculateColor();
	}
	
	Light::Light(const Light *other) :
		SceneNode(other),
		_shadowTarget(nullptr),
		_shadowDepthTexture(nullptr)
	{
		SetBoundingSphere(Sphere(Vector3(), 1.0f));
		SetBoundingBox(AABB(Vector3(), 1.0f), false);

		Light *temp = const_cast<Light *>(other);
		LockWrapper<Object *> wrapper(temp);
		LockGuard<LockWrapper<Object *>> lock(wrapper);

		_lightType = other->_lightType;
		SetColor(other->GetColor());
		SetIntensity(other->GetIntensity());
		SetAngle(other->GetAngle());
		SetRange(other->GetRange());
		_angleCos = other->_angleCos;
		
		_suppressShadows = other->_suppressShadows;
		
		if(other->HasShadows())
			ActivateShadows(other->_shadowParameter);
	}
	
	Light::~Light()
	{
		RemoveShadowCameras();
	}
	
	
	bool Light::CanRender(Renderer *renderer, Camera *camera) const
	{
		if(HasFlags(Flags::Hidden))
			return false;

		if(_lightType == Type::DirectionalLight)
			return true;

		return camera->InFrustum(GetBoundingSphere());
	}
	
	void Light::Render(Renderer *renderer, Camera *camera) const
	{
		renderer->SubmitLight(this);
	}
	
	void Light::SetType(Type type)
	{
		_lightType = type;
		
		if(_suppressShadows || _shadowDepthCameras.GetCount() == 0)
			return;
		
		RemoveShadowCameras();
		ActivateShadows();
	}
	
	void Light::SetRange(float range)
	{
		SetWorldScale(RN::Vector3(range));
	}
	
	void Light::SetRangeInternal(float range)
	{
		_range = range;
	}
	
	void Light::SetColor(const Color &color)
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
		_shadowDepthCameras.RemoveAllObjects();
		
		SafeRelease(_shadowTarget);
		SafeRelease(_shadowDepthTexture);
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
		if(!HasShadows())
		{
			ActivateShadows(parameter);
			return;
		}
		
		
		if(_shadowParameter.resolution != parameter.resolution || _shadowParameter.splits.size() != parameter.splits.size())
		{
			DeactivateShadows();
			ActivateShadows(parameter);
		}
		else
		{
			if(parameter.shadowTarget != _shadowTarget)
			{
				SafeRelease(_shadowTarget);
				
				if(parameter.shadowTarget)
				{
					_shadowTarget = parameter.shadowTarget->Retain();
				}
			}
			
			_shadowDepthCameras.Enumerate<Camera>([&](Camera *camera, size_t index, bool &stop) {
				camera->GetMaterial()->SetPolygonOffset(parameter.splits[index].biasFactor, parameter.splits[index].biasUnits);
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
		
		_shadowTarget = _shadowParameter.shadowTarget->Retain();
		
		Texture::Descriptor textureDescriptor;
		textureDescriptor.type = Texture::Type::Type2DArray;
		textureDescriptor.format = Texture::Format::Depth_32F;
		textureDescriptor.usageHint = Texture::UsageHint::RenderTarget | Texture::UsageHint::ShaderRead;
		textureDescriptor.accessOptions = GPUResource::AccessOptions::Private;
		textureDescriptor.width = _shadowParameter.resolution;
		textureDescriptor.height = _shadowParameter.resolution;
		textureDescriptor.depth = _shadowParameter.splits.size();
		_shadowDepthTexture = Texture::WithDescriptor(textureDescriptor)->Retain();
		Framebuffer::TargetView depthTargetView;
		depthTargetView.texture = _shadowDepthTexture;
		depthTargetView.mipmap = 0;
		depthTargetView.slice = 0;
		depthTargetView.length = 1;
		
		Shader::Options *shaderOptions = Shader::Options::WithNone();
		Shader *depthVertexShader = Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Depth);
		Shader *depthFragmentShader = Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Depth);
		
		_shadowCameraMatrices.clear();
		for(uint32 i = 0; i < _shadowParameter.splits.size(); i++)
		{
			_shadowCameraMatrices.push_back(Matrix());
			
			//TODO: Get rid of the need for a shader if not needed for rendering!?
			Material *depthMaterial = Material::WithShaders(depthVertexShader, depthFragmentShader);
			depthMaterial->SetPolygonOffset(true, _shadowParameter.splits[i].biasFactor, _shadowParameter.splits[i].biasUnits);
			depthMaterial->SetOverride(Material::Override::DefaultDepth);

			Framebuffer *framebuffer = Renderer::GetActiveRenderer()->CreateFramebuffer(Vector2(_shadowParameter.resolution));
			depthTargetView.slice = i;
			framebuffer->SetDepthStencilTarget(depthTargetView);
			framebuffer->Autorelease();
			
			Camera *tempcam = new Camera();
			tempcam->GetRenderPass()->SetFramebuffer(framebuffer);
			tempcam->SetFlags(Camera::Flags::Orthogonal | Camera::Flags::RenderEarly);
			tempcam->SceneNode::SetPriority(SceneNode::Priority::UpdateLate);
			tempcam->SetMaterial(depthMaterial);
			tempcam->SetShaderHint(Shader::UsageHint::Depth);
			tempcam->SetLODCamera(_shadowTarget);
			tempcam->SetClipNear(1.0f);
			tempcam->Autorelease();

			_shadowTarget->GetSceneInfo()->GetScene()->AddNode(tempcam);
			_shadowDepthCameras.AddObject(tempcam);
		}
		
		return true;
	}
	
	bool Light::ActivatePointShadows()
	{
		return false;

/*		if(_shadowDepthCameras.GetCount() > 0)
			DeactivateShadows();
		
		RN_ASSERT(_shadowParameter.splits.size() == 1, "The shadow parameter for point lights needs exactly one split!");
		
		Texture::Parameter textureParameter;
		textureParameter.wrapMode = Texture::WrapMode::Repeat;
		textureParameter.filter = Texture::Filter::Nearest;
		textureParameter.format = Texture::Format::Depth24I;
		textureParameter.depthCompare = false;
		textureParameter.maxMipMaps = 0;
		textureParameter.anisotropy = 1.0f;
		
		Texture *depthtex = (new TextureCubeMap(textureParameter))->Autorelease();
		
		Shader   *depthShader = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyPointShadowDepthShader, nullptr);
		Material *depthMaterial = (new Material(depthShader))->Autorelease();
		depthMaterial->SetPolygonOffset(true);
		depthMaterial->SetPolygonOffsetFactor(_shadowParameter.splits[0].biasFactor);
		depthMaterial->SetPolygonOffsetUnits(_shadowParameter.splits[0].biasUnits);
		depthMaterial->SetOverride(Material::Override::GroupDiscard | Material::Override::Culling);
		
		RenderStorage *storage = new RenderStorage(RenderStorage::BufferFormatDepth, 0, 1.0f);
		storage->SetDepthTarget(depthtex, -1);
		storage->Autorelease();
		
		RN::Camera *shadowcam = new CubemapCamera(Vector2(_shadowParameter.resolution), storage, Camera::Flags::UpdateAspect | Camera::Flags::UpdateStorageFrame | Camera::Flags::NoFlush, 1.0f);
		shadowcam->SetMaterial(depthMaterial);
		shadowcam->SetPriority(kRNShadowCameraPriority);
		shadowcam->SetClipNear(0.01f);
		shadowcam->SetClipFar(_range);
		shadowcam->SetFOV(90.0f);
		shadowcam->SetLightManager(nullptr);
		shadowcam->SetWorldRotation(Vector3(0.0f, 0.0f, 0.0f));
		shadowcam->SceneNode::SetFlags(shadowcam->SceneNode::GetFlags() | SceneNode::Flags::HideInEditor | SceneNode::Flags::NoSave);
		shadowcam->Autorelease();
		
		_shadowDepthCameras.AddObject(shadowcam);
		AddDependency(shadowcam);
		
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
		
		return true;*/
	}
	
	bool Light::ActivateSpotShadows()
	{
		return false;

/*		if(_shadowDepthCameras.GetCount() > 0)
			DeactivateShadows();
		
		RN_ASSERT(_shadowParameter.splits.size() == 1, "The shadow parameter for spot lights needs exactly one split!");
		
		Texture::Parameter textureParameter;
		textureParameter.wrapMode = Texture::WrapMode::Clamp;
		textureParameter.filter = Texture::Filter::Linear;
		textureParameter.format = Texture::Format::Depth24I;
		textureParameter.depthCompare = true;
		textureParameter.maxMipMaps = 0;
		textureParameter.anisotropy = 1.0f;
		
		Texture *depthtex = new Texture2D(textureParameter);
		depthtex->Autorelease();
		
		Shader   *depthShader = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyDirectionalShadowDepthShader, nullptr);
		Material *depthMaterial = (new Material(depthShader))->Autorelease();
		depthMaterial->SetPolygonOffset(true);
		depthMaterial->SetPolygonOffsetFactor(_shadowParameter.splits[0].biasFactor);
		depthMaterial->SetPolygonOffsetUnits(_shadowParameter.splits[0].biasUnits);
		depthMaterial->SetOverride(Material::Override::GroupDiscard | Material::Override::Culling);
		
		RenderStorage *storage = new RenderStorage(RenderStorage::BufferFormatDepth, 0, 1.0f);
		storage->SetDepthTarget(depthtex, -1);
		storage->Autorelease();
		
		RN::Camera *shadowcam = new Camera(Vector2(_shadowParameter.resolution), storage, Camera::Flags::UpdateAspect | Camera::Flags::UpdateStorageFrame | Camera::Flags::NoFlush, 1.0f);
		shadowcam->SetMaterial(depthMaterial);
		shadowcam->SetPriority(kRNShadowCameraPriority);
		shadowcam->SetClipNear(0.01f);
		shadowcam->SetClipFar(_range);
		shadowcam->SetFOV(_angle * 2.0f);
		shadowcam->SetLightManager(nullptr);
		shadowcam->SetWorldRotation(Vector3(0.0f, 0.0f, 0.0f));
		shadowcam->SceneNode::SetFlags(shadowcam->SceneNode::GetFlags() | SceneNode::Flags::HideInEditor | SceneNode::Flags::NoSave);
		shadowcam->Autorelease();
		
		_shadowDepthCameras.AddObject(shadowcam);
		AddDependency(shadowcam);
		
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
		
		return true;*/
	}
	
	void Light::DidUpdate(ChangeSet change)
	{
		SceneNode::DidUpdate(change);
		if(change == ChangeSet::Position)
		{
			float range = GetScale().GetMax();
			if(!Math::Compare(range, _range))
			{
				SetRangeInternal(range);
			}
		}
	}
	
	void Light::Update(float delta)
	{
		SceneNode::Update(delta);
		UpdateShadows();
	}
	
	void Light::UpdateShadows()
	{
		if(_suppressShadows || _shadowDepthCameras.GetCount() == 0)
			return;
		
		if(_lightType == Type::DirectionalLight)
		{
			if(_shadowTarget)
			{
				float near = _shadowTarget->GetClipNear();
				float far = 0.01f;
				
				for(uint32 i = 0; i < _shadowParameter.splits.size(); i++)
				{
				/*	if(((GetLastFrame()+_shadowParameter.splits[i].updateOffset) % _shadowParameter.splits[i].updateInterval) == 0)
					{
						Camera *cam = _shadowDepthCameras.GetObjectAtIndex<Camera>(i);
						cam->SetFlags(cam->GetFlags()&(~Camera::Flags::NoRender));
						//TODO: Replace with something else
						//cam->SetClearMask(Camera::ClearMask::Depth);
					}
					else
					{
						Camera *cam = _shadowDepthCameras.GetObjectAtIndex<Camera>(i);
						cam->SetFlags(cam->GetFlags()|Camera::Flags::NoRender);
						//TODO: Replace with something else
						//cam->SetClearMask(0);
						continue;
					}*/
					
					if(_shadowParameter.splits[i].maxDistance < far)
					{
						float linear = _shadowTarget->GetClipNear() + (_shadowParameter.maxShadowDist*_shadowTarget->GetClipFar() - _shadowTarget->GetClipNear())*(i + 1.0f) / float(_shadowParameter.splits.size());
						float log = _shadowTarget->GetClipNear() * powf(_shadowParameter.maxShadowDist*_shadowTarget->GetClipFar() / _shadowTarget->GetClipNear(), (i + 1.0f) / float(_shadowParameter.splits.size()));
						far = linear*_shadowParameter.distanceBlendFactor + log*(1.0f - _shadowParameter.distanceBlendFactor);
					}
					else
					{
						far = _shadowParameter.splits[i].maxDistance;
					}
					
					Camera *tempcam = _shadowDepthCameras.GetObjectAtIndex<Camera>(i);
					tempcam->SetWorldRotation(GetWorldRotation());
					
					_shadowCameraMatrices[i] = tempcam->MakeShadowSplit(_shadowTarget, this, near, far);
					
					near = far;
				}
			}
		}
/*		else if(_lightType == Type::SpotLight)
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
		}*/
	}

	void Light::ReCalculateColor()
	{
		_finalColor = Vector3(_color.r, _color.g, _color.b);
		_finalColor *= (float)_intensity;
	}
}
