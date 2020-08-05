//
//  __TMP__World.cpp
//  __TMP_APPLICATION_NAME__
//
//  Copyright __TMP_YEAR__ by __TMP_COMPANY__. All rights reserved.
//

#include "__TMP__World.h"

namespace __TMP__
{
	World *World::_sharedInstance = nullptr;

	World *World::GetSharedInstance()
	{
		return _sharedInstance;
	}

	void World::Exit()
	{
		if(_sharedInstance)
		{
			_sharedInstance->RemoveAttachment(_sharedInstance->_physicsWorld);

#if RN_PLATFORM_ANDROID
			if(_sharedInstance->_vrWindow)
			{
				_sharedInstance->_vrWindow->Release(); //Reference from VRCamera
				_sharedInstance->_vrWindow->Release(); //Reference from World
				_sharedInstance->_vrWindow->Release(); //Reference from Application
				//_sharedInstance->_vrWindow->StopRendering();
			}
#endif
		}
		
		exit(0);
		//RN::Kernel::GetSharedInstance()->Exit();
	}

	World::World(RN::VRWindow *vrWindow) : _vrWindow(nullptr), _physicsWorld(nullptr), _isPaused(false), _isDash(false), _shaderLibrary(nullptr)
	{
		_sharedInstance = this;

		if (vrWindow)
			_vrWindow = vrWindow->Retain();
		
		_levelNodes = new RN::Array();
		
		_cameraManager.Setup(vrWindow);
	}

	World::~World()
	{
		
	}

	void World::WillBecomeActive()
	{
		RN::Scene::WillBecomeActive();

		if(!RN::Renderer::IsHeadless())
		{
			RN::Renderer *activeRenderer = RN::Renderer::GetActiveRenderer();
			_shaderLibrary = activeRenderer->CreateShaderLibraryWithFile(RNCSTR("shaders/Shaders.json"));
		}

		RN::String *pvdServerIP = nullptr;
#if RN_BUILD_DEBUG
	#if RN_PLATFORM_WINDOWS
		pvdServerIP = RNCSTR("localhost");
	#else
		pvdServerIP = RNCSTR("192.168.178.22");
	#endif
#endif
		_physicsWorld = new RN::PhysXWorld(RN::Vector3(0.0f, -9.81f, 0.0f), pvdServerIP);
		AddAttachment(_physicsWorld->Autorelease());

		LoadLevel();
	}

	void World::DidBecomeActive()
	{
		RN::SceneBasic::DidBecomeActive();
		_cameraManager.SetCameraAmbientColor(RN::Color::White(), 1.0f, nullptr);
	}

	void World::WillUpdate(float delta)
	{
		RN::Scene::WillUpdate(delta);

		_isPaused = false;
		_isDash = false;
		RN::VRHMDTrackingState::Mode headsetState = _cameraManager.Update(delta);
		if(headsetState == RN::VRHMDTrackingState::Mode::Paused)
		{
			_isPaused = true;
			_isDash = true;
		}
		else if(headsetState == RN::VRHMDTrackingState::Mode::Disconnected)
		{
			Exit();
		}

		if(RN::InputManager::GetSharedInstance()->IsControlToggling(RNCSTR("ESC")))
		{
			Exit();
		}
	}

	RN::Model *World::AssignShader(RN::Model *model, Types::MaterialType materialType) const
	{
		if(RN::Renderer::IsHeadless()) return model;
		
		World *world = World::GetSharedInstance();
		RN::ShaderLibrary *shaderLibrary = world->GetShaderLibrary();

		RN::Model::LODStage *lodStage = model->GetLODStage(0);
		for(int i = 0; i < lodStage->GetCount(); i++)
		{
			RN::Material *material = lodStage->GetMaterialAtIndex(i);
			
			switch(materialType)
			{
				case Types::MaterialDefault:
				{
					material->SetDepthWriteEnabled(true);
					material->SetDepthMode(RN::DepthMode::LessOrEqual);
					material->SetAlphaToCoverage(false);
					material->SetAmbientColor(RN::Color::White());
					RN::Shader::Options *shaderOptions = RN::Shader::Options::WithMesh(lodStage->GetMeshAtIndex(i));
					material->SetVertexShader(shaderLibrary->GetShaderWithName(RNCSTR("main_vertex"), shaderOptions));
					material->SetFragmentShader(shaderLibrary->GetShaderWithName(RNCSTR("main_fragment"), shaderOptions));
					break;
				}
			}
		}

		return model;
	}

	RN::Model *World::MakeDeepCopy(RN::Model *model) const
	{
		RN::Model *result = model->Copy();
		
		RN::Model::LODStage *lodStage = result->GetLODStage(0);
		for(int i = 0; i < lodStage->GetCount(); i++)
		{
			RN::Material *material = lodStage->GetMaterialAtIndex(i)->Copy();
			lodStage->ReplaceMaterial(material->Autorelease(), i);
		}
		
		return result->Autorelease();
	}

	void World::AddLevelNode(RN::SceneNode *node)
	{
		_levelNodes->AddObject(node);
		AddNode(node);
	}

	void World::RemoveLevelNode(RN::SceneNode *node)
	{
		_levelNodes->RemoveObject(node);
		RemoveNode(node);
	}

	void World::RemoveAllLevelNodes()
	{
		_levelNodes->Enumerate<RN::SceneNode>([&](RN::SceneNode *node, size_t index, bool &stop){
			RemoveNode(node);
		});
		
		_levelNodes->RemoveAllObjects();
	}

	void World::LoadLevel()
	{
		RemoveAllLevelNodes();
		
		RN::Model *levelModel = AssignShader(RN::Model::WithName(RNCSTR("models/template/level.sgm")), Types::MaterialDefault);
		RN::Entity *levelEntity = new RN::Entity(levelModel);
		AddLevelNode(levelEntity->Autorelease());

		RN::PhysXMaterial *levelPhysicsMaterial = new RN::PhysXMaterial();
		RN::PhysXCompoundShape *levelShape = RN::PhysXCompoundShape::WithModel(levelModel, levelPhysicsMaterial->Autorelease(), true);
		RN::PhysXStaticBody *levelBody = RN::PhysXStaticBody::WithShape(levelShape);
		levelBody->SetCollisionFilter(Types::CollisionLevel, Types::CollisionAll);
		levelEntity->AddAttachment(levelBody);
		
		if(!RN::Renderer::IsHeadless())
		{
			RN::Model *skyModel = RN::Model::WithName(RNCSTR("models/template/sky.sgm"));
			RN::Material *skyMaterial = RN::Material::WithShaders(nullptr, nullptr);
			skyMaterial->SetDepthMode(RN::DepthMode::LessOrEqual);

			RN::Shader::Options *skyShaderOptions = RN::Shader::Options::WithMesh(skyModel->GetLODStage(0)->GetMeshAtIndex(0));
			skyShaderOptions->AddDefine(RNCSTR("RN_SKY"), RNCSTR("1"));
			skyMaterial->SetVertexShader(RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, skyShaderOptions, RN::Shader::UsageHint::Default));
			skyMaterial->SetFragmentShader(RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, skyShaderOptions, RN::Shader::UsageHint::Default));
			skyMaterial->SetVertexShader(RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Vertex, skyShaderOptions, RN::Shader::UsageHint::Depth), RN::Shader::UsageHint::Depth);
			skyMaterial->SetFragmentShader(RN::Renderer::GetActiveRenderer()->GetDefaultShader(RN::Shader::Type::Fragment, skyShaderOptions, RN::Shader::UsageHint::Depth), RN::Shader::UsageHint::Depth);
			skyModel->GetLODStage(0)->ReplaceMaterial(skyMaterial, 0);
			
			RN::Entity *skyEntity = new RN::Entity(skyModel);
			skyEntity->SetScale(RN::Vector3(10.0f));
			skyEntity->AddFlags(RN::Entity::Flags::DrawLate);
			AddLevelNode(skyEntity->Autorelease());
		}
	}
}
