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
		_camera = new RN::Camera(RN::Vector2(), RN::Texture::FormatRGBA8888, RN::Camera::FlagFullscreen | RN::Camera::FlagUpdateAspect);
		CreateWorld();
		
#if RN_PLATFORM_MAC_OS
//		CreateSSAOStage();
#endif
		
		RN::Input::SharedInstance()->Activate();
	}
	
	World::~World()
	{
		RN::Input::SharedInstance()->Deactivate();
		
		_camera->Release();		
	}
	
	void World::Update(float delta)
	{
		RN::Input *input = RN::Input::SharedInstance();
		RN::Vector3 translation;
		RN::Vector3 rotation;
		
#if RN_PLATFORM_MAC_OS || RN_PLATFORM_WINDOWS
		  const RN::Vector3& mouseDelta = input->MouseDelta() * -0.2f;
		
		rotation.x = mouseDelta.x;
		rotation.z = mouseDelta.y;
		
		translation.x = (input->KeyPressed('a') - input->KeyPressed('d')) * 18.0f;
		translation.z = (input->KeyPressed('w') - input->KeyPressed('s')) * 18.0f;
#endif
		
#if RN_PLATFORM_IOS
		const std::vector<RN::Touch>& touches = input->Touches();
		
		for(auto i=touches.begin(); i!=touches.end(); i++)
		{
			if(i->phase == RN::Touch::TouchPhaseBegan)
			{
				if(i->location.x > _camera->Frame().width*0.5f)
				{
					_touchRight = i->uniqueID;
				}
				else
				{
					_touchLeft = i->uniqueID;
				}
			}
			
			if(i->phase == RN::Touch::TouchPhaseMoved)
			{
				if(i->uniqueID == _touchRight)
				{
					rotation.x = i->initialLocation.x - i->location.x;
					rotation.z = i->initialLocation.y - i->location.y;
				}
						
				if(i->uniqueID == _touchLeft)
				{
					translation.x = i->initialLocation.x - i->location.x;
					translation.z = i->initialLocation.y - i->location.y;
				}
			}
		}
		
		rotation *= delta;
		translation *= 0.2f;
#endif
		
		_camera->Rotate(rotation);
		
		RN::Matrix rot;
		rot.MakeRotate(_camera->Rotation());
		rot.Transpose();
		_camera->Translate(rot.Transform(translation * -delta));
	}
	
	
	void World::CreateSSAOStage()
	{
		// Surface normals + depth stage
		RN::Shader *surfaceShader = RN::Shader::WithFile("shader/SurfaceNormals");
		RN::Material *surfaceMaterial = new RN::Material(surfaceShader);
		
		_camera->SetMaterial(surfaceMaterial);
		
		// SSAO stage
		RN::Texture *noise = RN::Texture::WithFile("textures/SSAO_noise.png", RN::Texture::FormatRGB888, RN::Texture::WrapModeRepeat, RN::Texture::FilterLinear, true);
		RN::Shader *ssao = RN::Shader::WithFile("shader/SSAO");
		
		RN::Material *ssaoMaterial = new RN::Material(ssao);
		ssaoMaterial->AddTexture(noise);
		
		RN::Camera *ssaoStage  = new RN::Camera(RN::Vector2(), RN::Texture::FormatR8, RN::Camera::FlagInherit | RN::Camera::FlagDrawTarget, RN::RenderStorage::BufferFormatColor);
		ssaoStage->SetMaterial(ssaoMaterial);
		ssaoMaterial->Release();
		
		_camera->AddStage(ssaoStage);
		
		// Blur
		RN::Shader *blurVertical = new RN::Shader();
		RN::Shader *blurHorizontal = new RN::Shader();
		
		blurVertical->SetVertexShader("shader/GaussianVertical.vsh");
		blurVertical->SetFragmentShader("shader/Gaussian.fsh");
		blurVertical->Link();
		
		blurHorizontal->SetVertexShader("shader/GaussianHorizontal.vsh");
		blurHorizontal->SetFragmentShader("shader/Gaussian.fsh");
		blurHorizontal->Link();
		
		RN::Material *verticalMaterial = new RN::Material(blurVertical->Autorelease<RN::Shader>());
		RN::Material *horizontalMateral = new RN::Material(blurHorizontal->Autorelease<RN::Shader>());
		
		RN::Camera *verticalStage  = new RN::Camera(RN::Vector2(), RN::Texture::FormatR8, RN::Camera::FlagInherit | RN::Camera::FlagDrawTarget);
		RN::Camera *horizontalStage  = new RN::Camera(RN::Vector2(), RN::Texture::FormatR8, RN::Camera::FlagInherit | RN::Camera::FlagDrawTarget);
		
		verticalStage->SetMaterial(verticalMaterial);		
		horizontalStage->SetMaterial(horizontalMateral);
		 
		_camera->AddStage(verticalStage);
		_camera->AddStage(horizontalStage);
		
		// World stage
		RN::Camera *sceneCamera = new RN::Camera(RN::Vector2(), RN::Texture::FormatRGB888, RN::Camera::FlagInherit);
		_camera->AddStage(sceneCamera);
		
		// SSAO post stage
		RN::Shader *ssaoPost = new RN::Shader();
		ssaoPost->SetVertexShader("shader/SSAO.vsh");
		ssaoPost->SetFragmentShader("shader/SSAOPost.fsh");
		ssaoPost->Link();
		
		RN::Material *ssaoPostMaterial = new RN::Material(ssaoPost);
		ssaoPostMaterial->AddTexture(horizontalStage->RenderTarget());
		
		RN::Camera *ssaoPostStage = new RN::Camera(RN::Vector2(), RN::Texture::FormatRGB888, RN::Camera::FlagInherit | RN::Camera::FlagDrawTarget);
		ssaoPostStage->SetMaterial(ssaoPostMaterial);
		
		_camera->AddStage(ssaoPostStage);
	}
	
	void World::CreateWorld()
	{
		// Blocks
		/*RN::Texture *blockTexture0 = RN::Texture::WithFile("textures/brick.png", RN::Texture::FormatRGB888);
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
		RN::Texture *floorTexture = RN::Texture::WithFile("textures/tiles.png", RN::Texture::FormatRGB888);
		
		RN::Material *floorMaterial = new RN::Material(floorShader);
		floorMaterial->AddTexture(floorTexture);
		
		RN::Mesh *floorMesh = RN::Mesh::CubeMesh(RN::Vector3(5.0f, 0.5f, 5.0f));
		RN::Model *floorModel = new RN::Model(floorMesh->Autorelease<RN::Mesh>(), floorMaterial->Autorelease<RN::Material>());
		
		_floor = new RN::RigidBodyEntity();
		_floor->SetModel(floorModel);
		_floor->SetPosition(RN::Vector3(0.0f, -5.0f, -8.0f));
		_floor->SetMass(0.0f);
		_floor->SetSize(RN::Vector3(5.0f, 0.5f, 5.0f));
		_floor->SetRestitution(0.5f);*/
		
		RN::Shader *shader = RN::Shader::WithFile("shader/rn_Texture1Discard");
		
		RN::Model *model = RN::Model::WithFile("models/sponza/sponza.sgm");
		model->MaterialForMesh(model->MeshAtIndex(4))->SetShader(shader);
		model->MaterialForMesh(model->MeshAtIndex(4))->culling = false;
		model->MaterialForMesh(model->MeshAtIndex(10))->SetShader(shader);
		model->MaterialForMesh(model->MeshAtIndex(10))->culling = false;
		model->MaterialForMesh(model->MeshAtIndex(19))->SetShader(shader);
		model->MaterialForMesh(model->MeshAtIndex(19))->culling = false;
		
		RN::Entity *sponza = new RN::Entity();
		sponza->SetModel(model);
		sponza->SetScale(RN::Vector3(0.1, 0.1, 0.1));
		sponza->Rotate(RN::Vector3(0.0, 0.0, -90.0));
		sponza->SetPosition(RN::Vector3(0.0f, -5.0f, 0.0f));
		
#if RN_TARGET_OPENGL
		RN::Shader *instancedShader = RN::Shader::WithFile("shader/rn_Texture1Discard_instanced");
		RN::Model *foliage[4];
		
		foliage[0] = RN::Model::WithFile("models/nobiax/fern_01.sgm");
		foliage[0]->MaterialForMesh(foliage[0]->MeshAtIndex(0))->SetShader(instancedShader);
		foliage[0]->MaterialForMesh(foliage[0]->MeshAtIndex(0))->culling = false;
		
		foliage[1] = RN::Model::WithFile("models/nobiax/grass_05.sgm");
		foliage[1]->MaterialForMesh(foliage[1]->MeshAtIndex(0))->SetShader(instancedShader);
		foliage[1]->MaterialForMesh(foliage[1]->MeshAtIndex(0))->culling = false;
		
		foliage[2] = RN::Model::WithFile("models/nobiax/grass_19.sgm");
		foliage[2]->MaterialForMesh(foliage[2]->MeshAtIndex(0))->SetShader(instancedShader);
		foliage[2]->MaterialForMesh(foliage[2]->MeshAtIndex(0))->culling = false;
		
		foliage[3] = RN::Model::WithFile("models/nobiax/grass_04.sgm");
		foliage[3]->MaterialForMesh(foliage[3]->MeshAtIndex(0))->SetShader(instancedShader);
		foliage[3]->MaterialForMesh(foliage[3]->MeshAtIndex(0))->culling = false;
		
		uint32 index = 0;
		
		for(float x = -100.0f; x < 200.0f; x += 1.0f)
		{
			for(float y = -100.0f; y < 100.0f; y += 1.0f)
			{
				index = (index + 1) % 4;
				
				RN::Entity *fern = new RN::Entity();
				fern->SetModel(foliage[index]);
				fern->Rotate(RN::Vector3(0.0, 0.0, -90.0));
				fern->SetPosition(RN::Vector3(x, -5.3, y));
			}
		}
#endif
	}
}
