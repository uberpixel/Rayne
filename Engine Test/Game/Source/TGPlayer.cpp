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
		RN::bullet::KinematicController(RN::bullet::CapsuleShape::WithRadius((model->BoundingBox().maxExtend-model->BoundingBox().minExtend).Length() * 0.5f*0.4f, (model->BoundingBox().maxExtend-model->BoundingBox().minExtend).y*0.5f*0.4f), 0.45f)
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
		
		const RN::Vector2& mouseDelta = input->MouseDelta();
		rotation.x = mouseDelta.x;
		
		if(_camera)
		{
			float pitch = _camera->Pitch() + (mouseDelta.y * 0.8f);
			pitch = MIN(25.0f, MAX(-40.0f, pitch));
			
			_camera->SetPitch(pitch);
		}
		
		translation.x = (input->KeyPressed('d') - input->KeyPressed('a')) * 0.10f;
		translation.z = (input->KeyPressed('s') - input->KeyPressed('w')) * 0.16f*(1.0+input->KeyPressed('c'));
		
		float movement = translation.Length();
		if(movement > 0.0f)
		{
			RN::Skeleton *skeleton = Skeleton();
			
			if(skeleton)
				skeleton->Update(200.0f * delta * fabs(translation.z));
		}
		
		Rotate(rotation);
		SetWalkDirection(WorldRotation().RotateVector(translation));
		
		RN::bullet::KinematicController::Update(delta);
	}
}
