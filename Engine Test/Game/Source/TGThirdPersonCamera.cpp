//
//  TGThirdPersonCamera.cpp
//  Game-osx
//
//  Created by Sidney Just on 07.04.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGThirdPersonCamera.h"

namespace TG
{
	RNDeclareMeta(ThirdPersonCamera)
	
	ThirdPersonCamera::ThirdPersonCamera(RN::RenderStorage *storage) :
		RN::Camera(RN::Vector2(), storage, RN::Camera::FlagDefaults)
	{
		_target = 0;
		_distance = 9.0f;
	}
	
	ThirdPersonCamera::~ThirdPersonCamera()
	{
		if(_target)
			_target->Release();
	}
	
	void ThirdPersonCamera::SetTarget(RN::Entity *target)
	{
		if(_target)
			_target->Release();
		
		_target = target ? target->Retain() : 0;
	}
	
	void ThirdPersonCamera::Update(float delta)
	{
		if(_target)
		{
			const RN::Quaternion& rotation = _target->WorldRotation();
			const RN::Vector3& position = _target->WorldPosition();
			
			RN::Vector3 cameraPosition = rotation.RotateVector(RN::Vector3(0.0f, 3.8f, _distance));
			cameraPosition += position;
			
			SetWorldPosition(cameraPosition);
			SetWorldRotation(rotation);
		}
		
		RN::Camera::Update(delta);
	}
	
	bool ThirdPersonCamera::CanUpdate(FrameID frame)
	{
		if(!RN::Camera::CanUpdate(frame))
			return false;
		
		if(_target)
			return (_target->LastFrame() == frame);
		
		return true;
	}
}
