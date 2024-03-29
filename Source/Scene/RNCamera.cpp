//
//  RNCamera.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCamera.h"
#include "RNLight.h"
#include "../Rendering/RNRenderer.h"
#include "../Rendering/RNWindow.h"

namespace RN
{
	RNDefineMeta(Camera, SceneNode)

	Camera::Camera(RenderPass *renderPass) :
		_cameraSceneEntry(this),
		_flags(Camera::Flags::Defaults)
	{
		if(!renderPass)
		{
			_renderPass = new RenderPass();
		}
		else
		{
			_renderPass = renderPass->Retain();
		}

		Initialize();
	}

	Camera::Camera(const Vector2 &size) :
		_cameraSceneEntry(this),
		_flags(Camera::Flags::Defaults)
	{
		//TODO: This implementaiton is probably not what a user would expect when creating a camera with a size tbh... It just renders it to the screens top left corner with the given size. Instead it should probably create a render target of that size and render into it instead.
		_renderPass = new RenderPass();
		_renderPass->SetFrame(RN::Rect(0, 0, size.x, size.y));

		Initialize();
	}

	Camera::~Camera()
	{
		SafeRelease(_renderPass);
		SafeRelease(_material);
		
		SafeRelease(_multiviewCameras);

/*		if(_lightManager)
		{
			_lightManager->camera = nullptr;
			_lightManager->Unlock();
		}*/
	}


	void Camera::Initialize()
	{
		_fov      = 70.0f;
		_aspect   = 0.0f;

		_clipNear = 0.1f;
		_clipFar  = 500.0f;

		_orthoLeft   = -100.0f;
		_orthoRight  = 100.0f;
		_orthoBottom = -100.0f;
		_orthoTop    = 100.0f;

		_fogNear   = 100.0f;
		_fogFar    = 500.0f;
		_ambient   = Color::White();
		_clipPlane = Plane();

		_dirtyProjection = true;
		_dirtyPosition = true;
		_dirtyFrustum = true;

		_shaderHint = Shader::UsageHint::Default;
		_prefersLightManager = false;

		_material   = nullptr;

		_priority  = 0;
		_lodCamera = nullptr;
		
		_multiviewCameras = nullptr;
		_isMultiviewCamera = false;

		_prefersLightManager = true;
		
		_frustumPlaneOffsets[0] = 0.0f;
		_frustumPlaneOffsets[1] = 0.0f;
		_frustumPlaneOffsets[2] = 0.0f;
		_frustumPlaneOffsets[3] = 0.0f;
	}

	// Setter
	void Camera::SetRenderPass(RenderPass *renderPass)
	{
		SafeRelease(_renderPass);
		_renderPass = renderPass->Retain();
	}

	void Camera::SetFlags(Flags flags)
	{
		_flags = flags;
	}

	void Camera::SetShaderHint(Shader::UsageHint hint)
	{
		_shaderHint = hint;
	}

	void Camera::SetMaterial(Material *material)
	{
		SafeRelease(_material);
		_material = SafeRetain(material);
	}

	void Camera::SetLODCamera(Camera *camera)
	{
		_lodCamera = camera;
	}

	void Camera::SetPriority(int32 priority)
	{
		_priority = priority;
	}

	void Camera::SetFOV(float fov)
	{
		_fov = fov;
		_dirtyProjection = true;
	}
	void Camera::SetAspectRatio(float ratio)
	{
		_aspect = ratio;
		_dirtyProjection = true;
	}

	void Camera::SetClipNear(float near)
	{
		_clipNear = near;
		_dirtyProjection = true;
	}
	void Camera::SetClipFar(float far)
	{
		_clipFar = far;
		_dirtyProjection = true;
	}

	void Camera::SetFogColor0(Color color)
	{
		_fogColor0 = color;
	}
	void Camera::SetFogColor1(Color color)
	{
		_fogColor1 = color;
	}
	void Camera::SetFogNear(float near)
	{
		_fogNear = near;
	}
	void Camera::SetFogFar(float far)
	{
		_fogFar = far;
	}

	void Camera::SetAmbientColor(Color color)
	{
		_ambient = color;
	}
	void Camera::SetClipPlane(const Plane &clipPlane)
	{
		_clipPlane = clipPlane;
	}

/*	void Camera::SetLightManager(LightManager *lightManager)
	{
		RN_ASSERT(!lightManager || !lightManager->camera, "The LightManager can't be attached to another camera!");

		if(_lightManager)
		{
			_lightManager->camera = nullptr;
			_lightManager->Unlock();
		}

		_prefersLightManager = false;

		_lightManager = SafeRetain(lightManager);

		if(_lightManager)
			_lightManager->camera = this;
	}*/

	void Camera::SetOrthogonalFrustum(float top, float bottom, float left, float right)
	{
		RN_ASSERT((_flags & Flags::Orthogonal), "SetOrthogonalFrustum() called, but the camera is not an orthogonal camera");

		_orthoLeft   = left;
		_orthoRight  = right;
		_orthoTop    = top;
		_orthoBottom = bottom;

		_dirtyProjection = true;
	}

	void Camera::SetFrustumPlaneOffset(float topOffset, float bottomOffset, float leftOffset, float rightOffset)
	{
		_frustumPlaneOffsets[0] = topOffset;
		_frustumPlaneOffsets[1] = bottomOffset;
		_frustumPlaneOffsets[2] = leftOffset;
		_frustumPlaneOffsets[3] = rightOffset;
		_dirtyFrustum = true;
	}

	void Camera::SetProjectionMatrix(const Matrix &projectionMatrix)
	{
		_dirtyProjection = false;
		_dirtyFrustum = true;
		_projectionMatrix = projectionMatrix;
		_inverseProjectionMatrix = _projectionMatrix.GetInverse();
		UpdateFrustum();
	}

	void Camera::DidUpdate(ChangeSet changeSet)
	{
		SceneNode::DidUpdate(changeSet);

		if(changeSet & ChangeSet::Position)
		{
			_dirtyPosition = true;
		}
	}

	Matrix Camera::MakeShadowSplit(Camera *camera, Light *light, float cameraDistanceToCenter, float near, float far)
	{
		Rect frame = _renderPass->GetFrame();

		//Get camera frustum extends to be covered by the split
		//far plane is at z=0, near plane z=1 for reverse-z!
		Vector3 nearcenter = camera->ToWorld(Vector3(0.0f, 0.0f, near));
		Vector3 farcorner1 = camera->ToWorld(Vector3(1.0f, 1.0f, far));
		Vector3 farcorner2 = camera->ToWorld(Vector3(-1.0f, -1.0f, far));
		Vector3 farcenter = (farcorner1+farcorner2)*0.5f;
		Vector3 center = (nearcenter+farcenter)*0.5f;

		//Calculate the size of a pixel in world units
		float dist = center.GetDistance(farcorner1);
		Vector3 pixelsize = Vector3(Vector2(dist*2.0f), 1.0f)/Vector3(frame.width, frame.height, 1.0f);

		//Place the light camera above the splits center
		Vector3 pos = center-light->GetForward()*cameraDistanceToCenter;

		//Transform the position to light space
		Matrix rot = light->GetWorldRotation().GetRotationMatrix();
		pos = rot.GetInverse()*pos;

		//Snap to the pixel grid
		pos /= pixelsize;
		pos.x = floorf(pos.x);
		pos.y = floorf(pos.y);
		pos.z = floorf(pos.z);
		pos *= pixelsize;

		//Transform back and place the camera there
		pos = rot*pos;
		SetWorldPosition(pos);

		//Set the light camera frustum
		_orthoLeft = -dist;
		_orthoRight = dist;
		_orthoBottom = -dist;
		_orthoTop = dist;

		//Update the projection matrix
		_dirtyProjection = true;
		UpdateProjection(); //Because the target is always a valid framebuffer, we don't need to pass the renderer as parameter here

		//Return the resulting matrix
		Matrix projview = _projectionMatrix * GetWorldTransform().GetInverse();
		return projview;
	}

	// Helper
	void Camera::Update(float delta)
	{
		SceneNode::Update(delta);
		_dirtyPosition = true;
	}

	void Camera::PostUpdate()
	{
		if(_dirtyPosition)
		{
			_dirtyPosition = false;
			_dirtyFrustum = true;

			_inverseViewMatrix = Matrix::WithTranslation(GetWorldPosition());
			_inverseViewMatrix.Rotate(GetWorldRotation());
			_inverseViewMatrix.Scale(GetScale());

			_viewMatrix = _inverseViewMatrix.GetInverse();
		}

		UpdateProjection();
		UpdateFrustum();
	}

	void Camera::UpdateProjection()
	{
		if(!_dirtyProjection)
			return;

		if(_flags & Flags::Orthogonal)
		{
			_projectionMatrix = Matrix::WithProjectionOrthogonal(_orthoLeft, _orthoRight, _orthoBottom, _orthoTop, _clipNear, _clipFar);
			_inverseProjectionMatrix = _projectionMatrix.GetInverse();
			return;
		}

		float tempAspect = _aspect;
		if(std::abs(tempAspect) <= 0.0001)
		{
			Vector2 size = _renderPass->GetFrame().GetSize();
			tempAspect = size.x / size.y;
		}

		_projectionMatrix = Matrix::WithProjectionPerspective(_fov, tempAspect, _clipNear, _clipFar);
		_inverseProjectionMatrix = _projectionMatrix.GetInverse();

		_dirtyProjection = false;

		_dirtyFrustum = true;
		UpdateFrustum();
	}

	void Camera::UpdateFrustum()
	{
		if(!_dirtyFrustum) return;
		_dirtyFrustum = false;

		if(_flags & Flags::UseSimpleCulling)
		{
			_frustumCenter = Vector3(0.0f, 0.0f, _clipFar*0.5f);
            _frustumRadius = _clipFar * 1.5;
			return;
		}
		
		//far plane is at z=0, near plane z=1 for reverse-z!
		Vector3 pos1 = __ToWorld(Vector3(-1.0f, 1.0f, 1.0f));
		Vector3 pos2 = __ToWorld(Vector3(-1.0f, 1.0f, 0.0));
		Vector3 pos3 = __ToWorld(Vector3(-1.0f, -1.0f, 0.0));
		Vector3 pos4 = __ToWorld(Vector3(1.0f, -1.0f, 1.0));
		Vector3 pos5 = __ToWorld(Vector3(1.0f, 1.0f, 0.0));
		Vector3 pos6 = __ToWorld(Vector3(1.0f, -1.0f, 0.0));

		const Vector3 &position = GetWorldPosition();
		Vector3 direction = GetWorldRotation().GetRotatedVector(Vector3(0.0, 0.0, -1.0));

		Vector3 vmax;
		Vector3 vmin;
		vmax.x = std::max(pos1.x, std::max(pos4.x, std::max(pos2.x, std::max(pos3.x, std::max(pos5.x, pos6.x)))));
		vmax.y = std::max(pos1.y, std::max(pos4.y, std::max(pos2.y, std::max(pos3.y, std::max(pos5.y, pos6.y)))));
		vmax.z = std::max(pos1.z, std::max(pos4.z, std::max(pos2.z, std::max(pos3.z, std::max(pos5.z, pos6.z)))));
		vmin.x = std::min(pos1.x, std::min(pos4.x, std::min(pos2.x, std::min(pos3.x, std::min(pos5.x, pos6.x)))));
		vmin.y = std::min(pos1.y, std::min(pos4.y, std::min(pos2.y, std::min(pos3.y, std::min(pos5.y, pos6.y)))));
		vmin.z = std::min(pos1.z, std::min(pos4.z, std::min(pos2.z, std::min(pos3.z, std::min(pos5.z, pos6.z)))));

		_frustumCenter = vmax + vmin;
		_frustumCenter = _frustumCenter * 0.5f;
		_frustumRadius = _frustumCenter.GetDistance(vmax);

		frustums._frustumLeft = Plane::WithTriangle(pos1, pos2, pos3, 1.0f, -_frustumPlaneOffsets[2]);
		frustums._frustumRight = Plane::WithTriangle(pos4, pos5, pos6, -1.0f, _frustumPlaneOffsets[3]);
		frustums._frustumTop =  Plane::WithTriangle(pos1, pos2, pos5, -1.0f, _frustumPlaneOffsets[0]);
		frustums._frustumBottom = Plane::WithTriangle(pos4, pos3, pos6, 1.0f, -_frustumPlaneOffsets[1]);
		frustums._frustumNear = Plane::WithPositionNormal(position + direction * std::min(_clipNear, _clipFar), -direction);
		frustums._frustumFar = Plane::WithPositionNormal(position + direction * std::max(_clipNear, _clipFar), direction);
	}

	Vector3 Camera::__ToWorld(const Vector3 &dir)
	{
		PostUpdate();

		Vector4 ndcPos(dir.x, dir.y, dir.z, 1.0f);
		
		Vector4 vec = _inverseViewMatrix * _inverseProjectionMatrix * ndcPos;
		vec /= vec.w;
		
		return Vector3(vec);
	}

	// There should be a much better solution, but at least this works for now
	Vector3 Camera::ToWorld(const Vector3 &dir)
	{
		PostUpdate();
		Vector4 ndcPos(dir.x, dir.y, 0.0f, 1.0f);
		
		Vector4 vec = _inverseProjectionMatrix * ndcPos;
		vec /= vec.w;
		
		float sign = vec.z < 0.0f? -1.0f : 1.0f; //Otherwise the division would lose the sign
		vec /= vec.z * sign;
		vec *= dir.z;
		vec.w = 1.0;
		
		vec = _inverseViewMatrix * vec;
		
		return Vector3(vec);
	}

/*	LightManager *Camera::GetLightManager()
	{
		if(!_lightManager && _prefersLightManager)
		{
			SetLightManager(LightManager::CreateDefaultLightManager());
			_lightManager->Unlock(); // SetLightManager() retains the light manager, and CreateDefaultLightManager() delegates the ownership to the caller
		}

		return _lightManager;
	}*/

	const Vector3 &Camera::GetFrustumCenter()
	{
		UpdateFrustum();
		return _frustumCenter;
	}

	float Camera::GetFrustumRadius()
	{
		UpdateFrustum();
		return _frustumRadius;
	}

	bool Camera::InFrustum(const Vector3 &position, float radius)
	{
		if(_frustumCenter.GetDistance(position) > _frustumRadius + radius)
			return false;

		if(_flags & Flags::UseSimpleCulling)
			return true;

		if(frustums._frustumLeft.GetDistance(position) > radius)
			return false;

		if(frustums._frustumRight.GetDistance(position) > radius)
			return false;

		if(frustums._frustumTop.GetDistance(position) > radius)
			return false;

		if(frustums._frustumBottom.GetDistance(position) > radius)
			return false;

		if(frustums._frustumNear.GetDistance(position) > radius)
			return false;

		if(frustums._frustumFar.GetDistance(position) > radius)
			return false;

		return true;
	}

	bool Camera::InFrustum(const Sphere &sphere)
	{
		UpdateFrustum();
		return InFrustum(sphere.position+sphere.offset, sphere.radius);
	}

	bool Camera::InFrustum(const AABB &aabb)
	{
		UpdateFrustum();

		/*		Plane *planes = &_frustumLeft;
				Plane *absPlanes = &_absFrustumLeft;

				Vector3 position = aabb.Position();

				for(int i=0; i<6; i++)
				{
					const Plane &plane = planes[i];
					const Plane &absPlane = absPlanes[i];

					float d = position.Dot(plane.normal);
					float r = aabb.width.Dot(absPlane.normal);

					float dpr = d + r + plane.d;

					if(Math::IsNegative(dpr))
						return false;
				}*/

		return true;
	}

	void Camera::AddMultiviewCamera(RN::Camera *camera)
	{
		RN_ASSERT(camera, "Camera cannot be empty");
		
		if(!_multiviewCameras)
		{
			_multiviewCameras = new RN::Array();
		}
		
		_multiviewCameras->AddObject(camera);
		camera->_isMultiviewCamera = true;
	}

	void Camera::RemoveMultiviewCamera(RN::Camera *camera)
	{
		_multiviewCameras->RemoveObject(camera);
		camera->_isMultiviewCamera = false;
	}
}
