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
		_lightClass  = Light::MetaClass();
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
		if(!(camera->renderGroup & (1 << node->renderGroup)))
			return;
		
		if(node->IsVisibleInCamera(camera))
		{
			node->Render(_renderer, camera);
			
			machine_uint childs = node->Childs();
			
			for(machine_uint i=0; i<childs; i++)
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
	
	Hit GenericSceneManager::CastRay(const Vector3 &position, const Vector3 &direction, uint32 mask)
	{
		Hit hit;
		for(auto i=_nodes.begin(); i!=_nodes.end(); i++)
		{
			SceneNode *node = *i;
			if(!(mask & (1 << node->collisionGroup)))
				continue;
				
			Hit result = node->CastRay(position, direction);
			
			if(result.distance >= 0.0f)
			{
				if(hit.distance < 0.0f)
					hit = result;
				
				if(result.distance < hit.distance)
					hit = result;
			}
		}
		
		return hit;
	}
}
