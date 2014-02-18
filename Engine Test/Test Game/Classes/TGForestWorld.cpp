//
//  TGForestWorld.cpp
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "TGForestWorld.h"

#define TGForestFeatureTrees 500
#define TGForestFeatureGras  350000

namespace TG
{
	ForestWorld::ForestWorld()
	{
		RN::MessageCenter::GetSharedInstance()->AddObserver(kRNInputEventMessage, [&](RN::Message *message) {
			
			RN::Event *event = static_cast<RN::Event *>(message);
			HandleInputEvent(event);
			
			if(event->GetType() == RN::Event::Type::KeyDown)
			{
				switch(event->GetCharacter())
				{
					case 'y':
						SetCutScene("forest_cutscene.json");
						break;
						
					default:
						break;
				}
			}
			
		}, this);
	}
	
	void ForestWorld::LoadOnThread(RN::Thread *thread)
	{
		CreateCameras();
		LoadLevelJSON("forest.json");
		
		// Camera and sun
		_camera->SetPosition(RN::Vector3(0.0f, 2.0f, 0.0f));
		
		_sunLight = new Sun();
		_sunLight->SetRenderGroup(3);
		_sunLight->ActivateShadows(RN::ShadowParameter(_camera, 2048));
		
		// Create the ground
		RN::Model *ground = RN::Model::WithFile("models/UberPixel/ground.sgm");
		ground->GetMaterialAtIndex(0, 0)->SetShader(RN::Shader::WithFile("shader/rn_Blend"));
		ground->GetMaterialAtIndex(0, 0)->Define("RN_TEXTURE_TILING", 150);
		ground->GetMaterialAtIndex(0, 0)->SetCullMode(RN::Material::CullMode::None);
		//ground->GetMaterialAtIndex(0, 0)->SetPolygonMode(RN::Material::PolygonMode::Lines);
		ground->GetMaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/UberPixel/blend.png", true));
		ground->GetMaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/UberPixel/plaster2.png"));
		ground->GetMaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/UberPixel/plaster2_NRM.png", true));
		ground->GetMaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/UberPixel/plaster2_DISP.png", true));
		ground->GetMaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/UberPixel/sand.png"));
		ground->GetMaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/UberPixel/sand_NRM.png"));
		ground->GetMaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/UberPixel/rock.png"));
		ground->GetMaterialAtIndex(0, 0)->AddTexture(RN::Texture::WithFile("models/UberPixel/rock_NRM.png"));
		
		_ground = new RN::Entity(ground);
		_ground->SetScale(RN::Vector3(20.0f));
		
		RN::Water *water = new RN::Water(_camera, _refractCamera->GetStorage()->GetRenderTarget());
		water->SetWorldPosition(RN::Vector3(71.0f, -0.3f, -5.0f));
		
		
		LoadBlendAndHeightmap();
		LoadTreeModels();
		LoadGrassModels();
		
		RN::Random::MersenneTwister random;
		random.Seed(0x1024);
		
		
		// Trees
		RN::InstancingNode *treeNode = new RN::InstancingNode();
		treeNode->SetModels(RN::Array::WithObjects(_trees[0], _trees[1], _trees[2], _trees[3], _trees[4], _trees[5], _trees[6], _trees[7], _trees[8], _trees[9], nullptr));
		treeNode->SetPivot(_camera);
		
		for(int i = 0; i < TGForestFeatureTrees; i ++)
		{
			RN::Vector3 pos(random.RandomFloatRange(-200.0f, 200.0f), 0.0f, random.RandomFloatRange(-200.0f, 200.0f));
			
			if(PositionBlocked(pos + RN::Vector3(0.0f, 0.5f, 0.0f)))
			{
				i --;
				continue;
			}
			
			float sand  = _blendmap[IndexForPosition(pos)].b;
			if(sand > 0.05f)
				continue;
			
			
			pos.y = GetGroundHeight(pos);
			
			RN::Entity *entity = new RN::Entity(_trees[random.RandomInt32Range(0, TGForestTreeCount)], pos);
			entity->SetFlags(entity->GetFlags() | RN::SceneNode::Flags::Static);
			entity->SetScale(RN::Vector3(random.RandomFloatRange(0.89f, 1.12f)));
			entity->SetRotation(RN::Vector3(random.RandomFloatRange(0.0f, 360.0f), 0.0f, 0.0f));
			
			treeNode->AddChild(entity);
		}
		
		// Grass and reed
		/*RN::InstancingNode *grassNode = new RN::InstancingNode();
		grassNode->SetModels(RN::Array::WithObjects(_grass[0], _grass[1], _grass[2], _grass[3], nullptr));
		grassNode->SetRenderGroup(1);
		grassNode->SetPivot(_camera);
		grassNode->SetMode(RN::InstancingNode::Mode::Thinning | RN::InstancingNode::Mode::Clipping);
		grassNode->SetCellSize(32.0f);
		grassNode->SetClippingRange(16.0f);
		grassNode->SetThinningRange(128.0f);
		
		RN::InstancingNode *reedNode = new RN::InstancingNode();
		reedNode->SetModels(RN::Array::WithObjects(_reeds[0], nullptr));
		reedNode->SetPivot(_camera);
		reedNode->SetMode(RN::InstancingNode::Mode::Thinning | RN::InstancingNode::Mode::Clipping);
		reedNode->SetCellSize(32.0f);
		reedNode->SetClippingRange(16.0f);
		reedNode->SetThinningRange(128.0f);
		
		for(int i = 0; i < TGForestFeatureGras; i ++)
		{
			RN::Vector3 pos(random.RandomFloatRange(-200.0f, 200.0f), 0.2f, random.RandomFloatRange(-200.0f, 200.0f));
			
			if(PositionBlocked(pos + RN::Vector3(0.0f, 1.0f, 0.0f)))
			{
				i --;
				continue;
			}
			
			pos.y = GetGroundHeight(pos);
			
			int32 value = random.RandomInt32Range(1, 100);
			float sand  = _blendmap[IndexForPosition(pos)].b;
			
			// Reeds
			if(sand > 0.2f)
			{
				if(pos.y < -1.4f || value > 75)
					continue;
				
				// Leave a small chance for it to be a normal grass model inbetween the reeds
				if(value <= 60)
				{
					RN::Entity *entity = new RN::Entity(_reeds[0], pos);
					entity->SetFlags(entity->GetFlags() | RN::SceneNode::Flags::Static);
					entity->SetScale(RN::Vector3(random.RandomFloatRange(0.005f, 0.008f)));
					entity->SetRotation(RN::Vector3(random.RandomFloatRange(0, 360.0f), -90.0f, 90.0f));
					
					reedNode->AddChild(entity);
					continue;
				}
			}
			
			// Normal grass
			value = roundf(_flowermap[IndexForPosition(pos)].g * 40) + value;
			
			int index = 0;
			
			if(value > 98)
				index = 2;
			else if(value > 95)
				index = 3;
			else if(value > 70)
				value = 2;
			
			
			RN::Entity *entity = new RN::Entity(_grass[index], pos);
			entity->SetFlags(entity->GetFlags() | RN::SceneNode::Flags::Static);
			entity->SetScale(RN::Vector3(random.RandomFloatRange(0.9f, 1.3f)));
			entity->SetRotation(RN::Vector3(random.RandomFloatRange(0, 360.0f), 0.0f, 0.0f));
			
			grassNode->AddChild(entity);
		}*/
	}
	
	void ForestWorld::LoadBlendAndHeightmap()
	{
		{
			RN::Texture2D *texture = static_cast<RN::Texture2D *>(RN::Texture::WithFile("models/UberPixel/heightmap.png"));
			float *color = new float[texture->GetWidth() * texture->GetHeight()];
			
			RN::Texture::PixelData data;
			data.alignment = 1;
			data.format    = RN::Texture::Format::R32F;
			data.data      = color;
			
			texture->GetData(data);
			
			float *temp = color;
			
			for(size_t i = 0; i < texture->GetWidth() * texture->GetHeight(); i ++)
			{
				_heightmap.push_back(*temp);
				temp ++;
			}
			
			_heightExtent = (_ground->GetBoundingBox().maxExtend.y - _ground->GetBoundingBox().minExtend.y);
			_heightBase   = _ground->GetBoundingBox().position.y   + _ground->GetBoundingBox().minExtend.y;
			
			delete [] color;
		}
		{
			RN::Texture2D *texture = static_cast<RN::Texture2D *>(RN::Texture::WithFile("models/UberPixel/blend.png"));
			float *color = new float[texture->GetWidth() * texture->GetHeight() * 4];
			
			RN::Texture::PixelData data;
			data.alignment = 1;
			data.format    = RN::Texture::Format::RGBA32F;
			data.data      = color;
			
			texture->GetData(data);
			
			float *temp = color;
			
			for(size_t i = 0; i < texture->GetWidth() * texture->GetHeight(); i ++)
			{
				_blendmap.emplace_back(temp[0], temp[1], temp[2], temp[3]);
				temp += 4;
			}
			
			delete [] color;
		}
		{
			RN::Texture2D *texture = static_cast<RN::Texture2D *>(RN::Texture::WithFile("models/UberPixel/flowermap.png"));
			float *color = new float[texture->GetWidth() * texture->GetHeight() * 4];
			
			RN::Texture::PixelData data;
			data.alignment = 1;
			data.format    = RN::Texture::Format::RGBA32F;
			data.data      = color;
			
			texture->GetData(data);
			
			float *temp = color;
			
			for(size_t i = 0; i < texture->GetWidth() * texture->GetHeight(); i ++)
			{
				_flowermap.emplace_back(temp[0], temp[1], temp[2], temp[3]);
				temp += 4;
			}
			
			delete [] color;
		}
	}
	
	void ForestWorld::LoadTreeModels()
	{
		_trees[0] = RN::Model::WithFile("models/pure3d/BirchTrees/birch2m.sgm");
		_trees[0]->GetMaterialAtIndex(0, 0)->SetCullMode(RN::Material::CullMode::None);
		_trees[0]->GetMaterialAtIndex(0, 0)->SetDiscard(true);
		_trees[0]->GetMaterialAtIndex(0, 0)->SetDiscardThreshold(0.1f);
		_trees[0]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_trees[0]->GetMaterialAtIndex(0, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[0]->GetMaterialAtIndex(0, 1)->SetDiscard(true);
		_trees[0]->GetMaterialAtIndex(0, 1)->SetDiscardThreshold(0.1f);
		_trees[0]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		_trees[0]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		_trees[0]->GetMaterialAtIndex(1, 0)->SetCullMode(RN::Material::CullMode::None);
		_trees[0]->GetMaterialAtIndex(1, 0)->SetDiscard(true);
		_trees[0]->GetMaterialAtIndex(1, 0)->SetDiscardThreshold(0.1f);
		_trees[0]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		_trees[0]->GetMaterialAtIndex(1, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[0]->GetMaterialAtIndex(1, 1)->SetDiscard(true);
		_trees[0]->GetMaterialAtIndex(1, 1)->SetDiscardThreshold(0.1f);
		_trees[0]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		_trees[0]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		
		_trees[1] = RN::Model::WithFile("models/pure3d/BirchTrees/birch6m.sgm");
		_trees[1]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_trees[1]->GetMaterialAtIndex(0, 0)->SetCullMode(RN::Material::CullMode::None);
		_trees[1]->GetMaterialAtIndex(0, 0)->SetDiscard(true);
		_trees[1]->GetMaterialAtIndex(0, 0)->SetDiscardThreshold(0.1f);
		_trees[1]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		_trees[1]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		_trees[1]->GetMaterialAtIndex(1, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[1]->GetMaterialAtIndex(1, 1)->SetDiscard(true);
		_trees[1]->GetMaterialAtIndex(1, 1)->SetDiscardThreshold(0.1f);
		_trees[1]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		
		_trees[2] = RN::Model::WithFile("models/pure3d/BirchTrees/birch11m.sgm");
		_trees[2]->GetMaterialAtIndex(0, 0)->SetCullMode(RN::Material::CullMode::None);
		_trees[2]->GetMaterialAtIndex(0, 0)->SetDiscard(true);
		_trees[2]->GetMaterialAtIndex(0, 0)->SetDiscardThreshold(0.1f);
		_trees[2]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_trees[2]->GetMaterialAtIndex(0, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[2]->GetMaterialAtIndex(0, 1)->SetDiscard(true);
		_trees[2]->GetMaterialAtIndex(0, 1)->SetDiscardThreshold(0.1f);
		_trees[2]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		_trees[2]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		_trees[2]->GetMaterialAtIndex(1, 0)->SetCullMode(RN::Material::CullMode::None);
		_trees[2]->GetMaterialAtIndex(1, 0)->SetDiscard(true);
		_trees[2]->GetMaterialAtIndex(1, 0)->SetDiscardThreshold(0.1f);
		_trees[2]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		_trees[2]->GetMaterialAtIndex(1, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[2]->GetMaterialAtIndex(1, 1)->SetDiscard(true);
		_trees[2]->GetMaterialAtIndex(1, 1)->SetDiscardThreshold(0.1f);
		_trees[2]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		_trees[2]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		
		_trees[3] = RN::Model::WithFile("models/pure3d/BirchTrees/birch13m.sgm");
		_trees[3]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_trees[3]->GetMaterialAtIndex(0, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[3]->GetMaterialAtIndex(0, 1)->SetDiscard(true);
		_trees[3]->GetMaterialAtIndex(0, 1)->SetDiscardThreshold(0.1f);
		_trees[3]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		_trees[3]->GetMaterialAtIndex(0, 2)->SetCullMode(RN::Material::CullMode::None);
		_trees[3]->GetMaterialAtIndex(0, 2)->SetDiscard(true);
		_trees[3]->GetMaterialAtIndex(0, 2)->SetDiscardThreshold(0.1f);
		_trees[3]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		_trees[3]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		_trees[3]->GetMaterialAtIndex(1, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[3]->GetMaterialAtIndex(1, 1)->SetDiscard(true);
		_trees[3]->GetMaterialAtIndex(1, 1)->SetDiscardThreshold(0.1f);
		_trees[3]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		_trees[3]->GetMaterialAtIndex(1, 2)->SetCullMode(RN::Material::CullMode::None);
		_trees[3]->GetMaterialAtIndex(1, 2)->SetDiscard(true);
		_trees[3]->GetMaterialAtIndex(1, 2)->SetDiscardThreshold(0.1f);
		_trees[3]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		
		_trees[4] = RN::Model::WithFile("models/pure3d/BirchTrees/birch18m.sgm");
		_trees[4]->GetMaterialAtIndex(0, 0)->SetCullMode(RN::Material::CullMode::None);
		_trees[4]->GetMaterialAtIndex(0, 0)->SetDiscard(true);
		_trees[4]->GetMaterialAtIndex(0, 0)->SetDiscardThreshold(0.1f);
		_trees[4]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_trees[4]->GetMaterialAtIndex(0, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[4]->GetMaterialAtIndex(0, 1)->SetDiscard(true);
		_trees[4]->GetMaterialAtIndex(0, 1)->SetDiscardThreshold(0.1f);
		_trees[4]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		_trees[4]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		_trees[4]->GetMaterialAtIndex(1, 0)->SetCullMode(RN::Material::CullMode::None);
		_trees[4]->GetMaterialAtIndex(1, 0)->SetDiscard(true);
		_trees[4]->GetMaterialAtIndex(1, 0)->SetDiscardThreshold(0.1f);
		_trees[4]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		_trees[4]->GetMaterialAtIndex(1, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[4]->GetMaterialAtIndex(1, 1)->SetDiscard(true);
		_trees[4]->GetMaterialAtIndex(1, 1)->SetDiscardThreshold(0.1f);
		_trees[4]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		_trees[4]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		
		_trees[5] = RN::Model::WithFile("models/pure3d/BirchTrees/birch20m.sgm");
		_trees[5]->GetMaterialAtIndex(0, 0)->SetCullMode(RN::Material::CullMode::None);
		_trees[5]->GetMaterialAtIndex(0, 0)->SetDiscard(true);
		_trees[5]->GetMaterialAtIndex(0, 0)->SetDiscardThreshold(0.1f);
		_trees[5]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_trees[5]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		_trees[5]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		_trees[5]->GetMaterialAtIndex(1, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[5]->GetMaterialAtIndex(1, 1)->SetDiscard(true);
		_trees[5]->GetMaterialAtIndex(1, 1)->SetDiscardThreshold(0.1f);
		_trees[5]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		
		_trees[6] = RN::Model::WithFile("models/pure3d/PineTrees/pine4m.sgm");
		_trees[6]->GetMaterialAtIndex(0, 0)->SetCullMode(RN::Material::CullMode::None);
		_trees[6]->GetMaterialAtIndex(0, 0)->SetDiscard(true);
		_trees[6]->GetMaterialAtIndex(0, 0)->SetDiscardThreshold(0.5f);
		_trees[6]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_trees[6]->GetMaterialAtIndex(0, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[6]->GetMaterialAtIndex(0, 1)->SetDiscard(true);
		_trees[6]->GetMaterialAtIndex(0, 1)->SetDiscardThreshold(0.5f);
		_trees[6]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		_trees[6]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		_trees[6]->GetMaterialAtIndex(1, 0)->SetCullMode(RN::Material::CullMode::None);
		_trees[6]->GetMaterialAtIndex(1, 0)->SetDiscard(true);
		_trees[6]->GetMaterialAtIndex(1, 0)->SetDiscardThreshold(0.5f);
		_trees[6]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		_trees[6]->GetMaterialAtIndex(1, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[6]->GetMaterialAtIndex(1, 1)->SetDiscard(true);
		_trees[6]->GetMaterialAtIndex(1, 1)->SetDiscardThreshold(0.5f);
		_trees[6]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		_trees[6]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		_trees[6]->GetMaterialAtIndex(2, 0)->SetCullMode(RN::Material::CullMode::None);
		_trees[6]->GetMaterialAtIndex(2, 0)->SetDiscard(true);
		_trees[6]->GetMaterialAtIndex(2, 0)->SetDiscardThreshold(0.5f);
		_trees[6]->GetMaterialAtIndex(2, 0)->Define("RN_VEGETATION");
		_trees[6]->GetMaterialAtIndex(2, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[6]->GetMaterialAtIndex(2, 1)->SetDiscard(true);
		_trees[6]->GetMaterialAtIndex(2, 1)->SetDiscardThreshold(0.5f);
		_trees[6]->GetMaterialAtIndex(2, 1)->Define("RN_VEGETATION");
		
		_trees[7] = RN::Model::WithFile("models/pure3d/PineTrees/pine7m.sgm");
		_trees[7]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_trees[7]->GetMaterialAtIndex(0, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[7]->GetMaterialAtIndex(0, 1)->SetDiscard(true);
		_trees[7]->GetMaterialAtIndex(0, 1)->SetDiscardThreshold(0.5f);
		_trees[7]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		_trees[7]->GetMaterialAtIndex(0, 2)->SetCullMode(RN::Material::CullMode::None);
		_trees[7]->GetMaterialAtIndex(0, 2)->SetDiscard(true);
		_trees[7]->GetMaterialAtIndex(0, 2)->SetDiscardThreshold(0.5f);
		_trees[7]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		_trees[7]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		_trees[7]->GetMaterialAtIndex(1, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[7]->GetMaterialAtIndex(1, 1)->SetDiscard(true);
		_trees[7]->GetMaterialAtIndex(1, 1)->SetDiscardThreshold(0.5f);
		_trees[7]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		_trees[7]->GetMaterialAtIndex(1, 2)->SetCullMode(RN::Material::CullMode::None);
		_trees[7]->GetMaterialAtIndex(1, 2)->SetDiscard(true);
		_trees[7]->GetMaterialAtIndex(1, 2)->SetDiscardThreshold(0.5f);
		_trees[7]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		_trees[7]->GetMaterialAtIndex(2, 0)->Define("RN_VEGETATION");
		_trees[7]->GetMaterialAtIndex(2, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[7]->GetMaterialAtIndex(2, 1)->SetDiscard(true);
		_trees[7]->GetMaterialAtIndex(2, 1)->SetDiscardThreshold(0.5f);
		_trees[7]->GetMaterialAtIndex(2, 1)->Define("RN_VEGETATION");
		_trees[7]->GetMaterialAtIndex(2, 2)->SetCullMode(RN::Material::CullMode::None);
		_trees[7]->GetMaterialAtIndex(2, 2)->SetDiscard(true);
		_trees[7]->GetMaterialAtIndex(2, 2)->SetDiscardThreshold(0.5f);
		_trees[7]->GetMaterialAtIndex(2, 2)->Define("RN_VEGETATION");
		
		_trees[8] = RN::Model::WithFile("models/pure3d/PineTrees/pine9m.sgm");
		_trees[8]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_trees[8]->GetMaterialAtIndex(0, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[8]->GetMaterialAtIndex(0, 1)->SetDiscard(true);
		_trees[8]->GetMaterialAtIndex(0, 1)->SetDiscardThreshold(0.5f);
		_trees[8]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		_trees[8]->GetMaterialAtIndex(0, 2)->SetCullMode(RN::Material::CullMode::None);
		_trees[8]->GetMaterialAtIndex(0, 2)->SetDiscard(true);
		_trees[8]->GetMaterialAtIndex(0, 2)->SetDiscardThreshold(0.5f);
		_trees[8]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		_trees[8]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		_trees[8]->GetMaterialAtIndex(1, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[8]->GetMaterialAtIndex(1, 1)->SetDiscard(true);
		_trees[8]->GetMaterialAtIndex(1, 1)->SetDiscardThreshold(0.5f);
		_trees[8]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		_trees[8]->GetMaterialAtIndex(1, 2)->SetCullMode(RN::Material::CullMode::None);
		_trees[8]->GetMaterialAtIndex(1, 2)->SetDiscard(true);
		_trees[8]->GetMaterialAtIndex(1, 2)->SetDiscardThreshold(0.5f);
		_trees[8]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		_trees[8]->GetMaterialAtIndex(2, 0)->Define("RN_VEGETATION");
		_trees[8]->GetMaterialAtIndex(2, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[8]->GetMaterialAtIndex(2, 1)->SetDiscard(true);
		_trees[8]->GetMaterialAtIndex(2, 1)->SetDiscardThreshold(0.5f);
		_trees[8]->GetMaterialAtIndex(2, 1)->Define("RN_VEGETATION");
		
		_trees[9] = RN::Model::WithFile("models/pure3d/PineTrees/pine16m.sgm");
		_trees[9]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_trees[9]->GetMaterialAtIndex(0, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[9]->GetMaterialAtIndex(0, 1)->SetDiscard(true);
		_trees[9]->GetMaterialAtIndex(0, 1)->SetDiscardThreshold(0.5f);
		_trees[9]->GetMaterialAtIndex(0, 1)->Define("RN_VEGETATION");
		_trees[9]->GetMaterialAtIndex(0, 2)->SetCullMode(RN::Material::CullMode::None);
		_trees[9]->GetMaterialAtIndex(0, 2)->SetDiscard(true);
		_trees[9]->GetMaterialAtIndex(0, 2)->SetDiscardThreshold(0.5f);
		_trees[9]->GetMaterialAtIndex(0, 2)->Define("RN_VEGETATION");
		_trees[9]->GetMaterialAtIndex(1, 0)->Define("RN_VEGETATION");
		_trees[9]->GetMaterialAtIndex(1, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[9]->GetMaterialAtIndex(1, 1)->SetDiscard(true);
		_trees[9]->GetMaterialAtIndex(1, 1)->SetDiscardThreshold(0.5f);
		_trees[9]->GetMaterialAtIndex(1, 1)->Define("RN_VEGETATION");
		_trees[9]->GetMaterialAtIndex(1, 2)->SetCullMode(RN::Material::CullMode::None);
		_trees[9]->GetMaterialAtIndex(1, 2)->SetDiscard(true);
		_trees[9]->GetMaterialAtIndex(1, 2)->SetDiscardThreshold(0.5f);
		_trees[9]->GetMaterialAtIndex(1, 2)->Define("RN_VEGETATION");
		_trees[9]->GetMaterialAtIndex(2, 0)->Define("RN_VEGETATION");
		_trees[9]->GetMaterialAtIndex(2, 1)->SetCullMode(RN::Material::CullMode::None);
		_trees[9]->GetMaterialAtIndex(2, 1)->SetDiscard(true);
		_trees[9]->GetMaterialAtIndex(2, 1)->SetDiscardThreshold(0.5f);
		_trees[9]->GetMaterialAtIndex(2, 1)->Define("RN_VEGETATION");
	}
	
	void ForestWorld::LoadGrassModels()
	{
		_grass[0] = RN::Model::WithFile("models/dexsoft/grass2/grass1.sgm");
		_grass[0]->GetMaterialAtIndex(0, 0)->SetCullMode(RN::Material::CullMode::None);
		_grass[0]->GetMaterialAtIndex(0, 0)->SetDiscard(true);
		_grass[0]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_grass[0]->GetMaterialAtIndex(0, 0)->Define("RN_GRASS");
		
		_grass[1] = RN::Model::WithFile("models/dexsoft/grass2/grass2.sgm");
		_grass[1]->GetMaterialAtIndex(0, 0)->SetCullMode(RN::Material::CullMode::None);
		_grass[1]->GetMaterialAtIndex(0, 0)->SetDiscard(true);
		_grass[1]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_grass[1]->GetMaterialAtIndex(0, 0)->Define("RN_GRASS");
		
		_grass[2] = RN::Model::WithFile("models/dexsoft/grass2/grass3.sgm");
		_grass[2]->GetMaterialAtIndex(0, 0)->SetCullMode(RN::Material::CullMode::None);
		_grass[2]->GetMaterialAtIndex(0, 0)->SetDiscard(true);
		_grass[2]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_grass[2]->GetMaterialAtIndex(0, 0)->Define("RN_GRASS");
		
		_grass[3] = RN::Model::WithFile("models/UberPixel/rock/rock.sgm");
		
		_reeds[0] = RN::Model::WithFile("models/UberPixel/Schilf.mdl");
		_reeds[0]->GetMaterialAtIndex(0, 0)->SetCullMode(RN::Material::CullMode::None);
		_reeds[0]->GetMaterialAtIndex(0, 0)->SetDiscard(true);
		_reeds[0]->GetMaterialAtIndex(0, 0)->Define("RN_VEGETATION");
		_reeds[0]->GetMaterialAtIndex(0, 0)->Define("RN_GRASS");
	}
	
	
	
	bool ForestWorld::PositionBlocked(const RN::Vector3 &position)
	{
		for(RN::AABB &obstacle : _obstacles)
		{
			if(obstacle.Contains(position))
				return true;
		}
		
		if(_blendmap[IndexForPosition(position)].r > 0.2f)
			return true;
		
		return false;
	}
	
	float ForestWorld::GetGroundHeight(const RN::Vector3 &position)
	{
		float factor = _heightmap[IndexForPosition(position)];
		
		float height = _heightBase + (_heightExtent * factor);
		return height;
	}
	
	size_t ForestWorld::IndexForPosition(const RN::Vector3 &position)
	{
		RN_ASSERT(position.x >= -200.0f && position.x <= 200.0f, "position must be [-200, 200]");
		RN_ASSERT(position.z >= -200.0f && position.z <= 200.0f, "position must be [-200, 200]");
		
		RN::Vector3 pixel = ((position + 200.0f) / 400.0f) * 1023.0f;
		pixel.x = roundf(1023 - pixel.x);
		pixel.z = roundf(1023 - pixel.z);
		
		size_t index = static_cast<size_t>((pixel.z * 1024) + pixel.x);
		return index;
	}
}
