//
//  TGPlayer.cpp
//  Game-osx
//
//  Created by Sidney Just on 06.04.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGPlayer.h"

namespace TG
{
	RNDeclareMeta(Player)
	
	Player::Player(RN::Model *model) :
		RN::bullet::KinematicController(RN::bullet::CapsuleShape::WithRadius(model->BoundingBox().Width() * 0.5f, model->BoundingBox().Height()), 0.45f)
	{
		SetModel(model);
		_camera = 0;
	}
	
	Player::~Player()
	{
	}
	
	void Player::Update(float delta)
	{
		RN::Input *input = RN::Input::SharedInstance();
		RN::Vector3 translation;
		RN::Vector3 rotation;
		
		const RN::Vector3& mouseDelta = input->MouseDelta() * -0.2f;
		rotation.x = mouseDelta.x;
		
		if(_camera)
		{
			float pitch = _camera->Pitch() + (mouseDelta.y * 0.8f);
			pitch = MIN(25.0f, MAX(-40.0f, pitch));
			
			_camera->SetPitch(pitch);
		}
		
		translation.x = (input->KeyPressed('d') - input->KeyPressed('a')) * 0.10f;
		translation.z = (input->KeyPressed('s') - input->KeyPressed('w')) * 0.16f;
		
		float movement = translation.Length();
		if(movement > 0.0f)
		{
			RN::Skeleton *skeleton = Skeleton();
			
			if(skeleton)
				skeleton->Update(24.0f * delta);
		}
		
		Rotate(rotation);
		SetWalkDirection(WorldRotation().RotateVector(translation));
		
		RN::bullet::KinematicController::Update(delta);
	}
}
