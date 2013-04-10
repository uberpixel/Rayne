//
//  RNSceneManager.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneManager.h"
#include "RNLight.h"

namespace RN
{
	RNDeclareMeta(SceneManager)
	RNDeclareMeta(GenericSceneManager)
	
	SceneManager::SceneManager()
	{
		_renderer = Renderer::SharedInstance();
	}
	
	SceneManager::~SceneManager()
	{
	}
	
	
	
	
	GenericSceneManager::GenericSceneManager()
	{
		_entityClass = Entity::MetaClass();
		_lightClass = Light::MetaClass();
	}
	
	GenericSceneManager::~GenericSceneManager()
	{
	}
	
	
	void GenericSceneManager::AddSceneNode(SceneNode *node)
	{
		_nodes.insert(node);
	}
	
	void GenericSceneManager::RemoveSceneNode(SceneNode *node)
	{
		_nodes.erase(node);
	}
	
	void GenericSceneManager::UpdateSceneNode(SceneNode *node)
	{
	}
	
	
	
	void GenericSceneManager::RenderSceneNode(Camera *camera, SceneNode *node)
	{
		if(node->IsVisibleInCamera(camera))
		{
			if(node->IsKindOfClass(_entityClass))
			{
				Entity *entity = static_cast<Entity *>(node);
				if(entity->Model())
				{
					float distance = entity->WorldPosition().Distance(camera->WorldPosition());
					distance /= camera->clipfar;
					
					Model *model = entity->Model();
					uint32 lodStage = model->LODStageForDistance(distance);
					
					RenderingObject object;
					
					object.transform = (Matrix *)&entity->WorldTransform();
					object.skeleton  = entity->Skeleton();
					
					uint32 count = model->Meshes(lodStage);
					for(uint32 i=0; i<count; i++)
					{
						object.mesh = model->MeshAtIndex(lodStage, i);
						object.material = model->MaterialAtIndex(lodStage, i);
						
						_renderer->RenderObject(object);
					}
				}
			}
			
			if(node->IsKindOfClass(_lightClass))
			{
				_renderer->RenderLight(static_cast<Light *>(node));
			}
			
			for(machine_uint i=0; i<node->Childs(); i++)
			{
				SceneNode *child = node->ChildAtIndex(i);
				RenderSceneNode(camera, child);
			}
		}
	}

	void GenericSceneManager::RenderScene(Camera *camera)
	{
		for(auto i=_nodes.begin(); i!=_nodes.end(); i++)
		{
			SceneNode *node = *i;
			if(!node->Parent())
			{
				RenderSceneNode(camera, node);
			}
		}
	}
}
