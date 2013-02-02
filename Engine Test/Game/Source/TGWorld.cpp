//
//  TGWorld.cpp
//  Game
//
//  Created by Sidney Just on 27.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "TGWorld.h"

namespace TG
{
	World::World()
	{
		CreateWorld();
		
		_delta = 0.0f;
		_camera = new RN::Camera(RN::Vector2(), RN::Camera::FlagFullscreen | RN::Camera::FlagUpdateAspect);
		
		RN::Input::SharedInstance()->Activate();
		RN::MessageCenter::SharedInstance()->AddObserver(this, RN::Message::MessageGroupInput, RN::InputMessage::InputMessageTypeKeyDown | RN::InputMessage::InputMessageTypeKeyPressed);
	}
	
	World::~World()
	{
		RN::Input::SharedInstance()->Deactivate();
		RN::MessageCenter::SharedInstance()->RemoveObserver(this);
		
		_camera->Release();		
	}
	
	void World::Update(float delta)
	{
		static float time = 0.0f;
		
		_delta = delta;
		
		const RN::Vector3& mouseDelta = RN::Input::SharedInstance()->MouseDelta();
		
		float yaw = mouseDelta.x;
		float pitch = mouseDelta.y;
		
		_camera->Rotate(RN::Vector3(yaw, 0.0f, pitch));
		
		time += delta;
		if(time >= 5.0f)
		{
			_block1->ApplyImpulse(RN::Vector3(0.0f, 100.0f, 0.0f));
			_block1->ApplyTorqueImpulse(RN::Vector3(0.0f, 10.0f, 0.0f));
			
			time = 0.0f;
		}
	}
	
	void World::HandleMessage(RN::Message *message)
	{
		if(message->Group() == RN::Message::MessageGroupInput)
		{
			RN::InputMessage *input = (RN::InputMessage *)message;
			RN::Vector3 translation;
			
			switch(input->character)
			{
				case 'w':
					translation.z += 5.0f;
					break;
					
				case 's':
					translation.z -= 5.0f;
					break;
					
				case 'a':
					translation.x += 5.0f;
					break;
					
				case 'd':
					translation.x -= 5.0f;
					break;
					
				default:
					break;
			}
			
			_camera->Translate(translation * _delta);
		}
	}
	
	void World::CreateWorld()
	{
		// Blocks
		RN::Texture *blockTexture0 = RN::Texture::WithFile("textures/brick.png", RN::Texture::FormatRGB565);
		RN::Texture *blockTexture1 = RN::Texture::WithFile("textures/testpng.png", RN::Texture::FormatRGB565);
		
		RN::Material *blockMaterial = new RN::Material(0);
		blockMaterial->AddTexture(blockTexture0);
		blockMaterial->AddTexture(blockTexture1);
		
		RN::Mesh  *blockMesh = RN::Mesh::CubeMesh(RN::Vector3(0.5f, 0.5f, 0.5f));
		RN::Model *blockModel = RN::Model::WithMesh(blockMesh->Autorelease<RN::Mesh>(), blockMaterial->Autorelease<RN::Material>());
		
		_block1 = new RN::RigidBodyEntity();
		_block1->SetModel(blockModel);
		_block1->SetPosition(RN::Vector3(-1.0f, 8.0f, -8.0f));
		_block1->SetSize(RN::Vector3(0.5f));
		_block1->SetMass(10.0f);
		_block1->SetRestitution(0.8);
		_block1->SetFriction(1.8f);
		_block1->ApplyImpulse(RN::Vector3(0.0f, -100.0f, 0.0f));
		
		_block2 = new RN::RigidBodyEntity();
		_block2->SetModel(blockModel);
		_block2->SetRotation(RN::Quaternion(RN::Vector3(45.0f)));
		_block2->SetPosition(RN::Vector3(1.0f, 8.0f, -8.0f));
		_block2->SetSize(RN::Vector3(0.5f));
		_block2->SetMass(10.0f);
		
		_block3 = new RN::RigidBodyEntity();
		_block3->SetModel(blockModel);
		_block3->SetRotation(RN::Quaternion(RN::Vector3(45.0f)));
		_block3->SetPosition(RN::Vector3(0.0f, 8.0f, -10.0f));
		_block3->SetSize(RN::Vector3(0.5f));
		_block3->SetMass(10.0f);
		_block3->ApplyImpulse(RN::Vector3(0.0f, 0.0f, -15.0f));
		
		// Floor
		RN::Shader *floorShader = RN::Shader::WithFile("shader/Ground");
		RN::Texture *floorTexture = RN::Texture::WithFile("textures/tiles.png", RN::Texture::FormatRGB565);
		
		RN::Material *floorMaterial = new RN::Material(floorShader);
		floorMaterial->AddTexture(floorTexture);
		
		RN::Mesh *floorMesh = RN::Mesh::CubeMesh(RN::Vector3(5.0f, 0.5f, 5.0f));
		RN::Model *floorModel = new RN::Model(floorMesh->Autorelease<RN::Mesh>(), floorMaterial->Autorelease<RN::Material>());
		
		_floor = new RN::RigidBodyEntity();
		_floor->SetModel(floorModel);
		_floor->SetPosition(RN::Vector3(0.0f, -5.0f, -8.0f));
		_floor->SetMass(0.0f);
		_floor->SetSize(RN::Vector3(5.0f, 0.5f, 5.0f));
		_floor->SetRestitution(0.5f);
	}
}
