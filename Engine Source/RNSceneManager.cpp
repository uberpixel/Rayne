//
//  RNSceneManager.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneManager.h"
#include "RNWorld.h"

namespace RN
{
	RNDefineMeta(SceneManager)
	RNDefineMeta(GenericSceneManager)
	
	SceneManager::SceneManager()
	{
		_renderer = Renderer::GetSharedInstance();
	}
	
	SceneManager::~SceneManager()
	{
	}
	
	
	
	
	GenericSceneManager::GenericSceneManager()
	{}
	
	GenericSceneManager::~GenericSceneManager()
	{}
	
	
	void GenericSceneManager::AddSceneNode(SceneNode *node)
	{
		Lock();
		
		if(!node->GetParent())
		{
			_nodes.push_back(node);
		}
		
		Unlock();
	}
	
	void GenericSceneManager::RemoveSceneNode(SceneNode *node)
	{
		Lock();
		
		_nodes.erase(std::remove(_nodes.begin(), _nodes.end(), node), _nodes.end());
		
		Unlock();
	}
	
	void GenericSceneManager::UpdateSceneNode(SceneNode *node, SceneNode::ChangeSet changes)
	{
		if(changes & SceneNode::ChangeSet::Parent)
		{
			Lock();
			
			bool hasParent = (node->GetParent());
			
			if(!hasParent)
			{
				Unlock();
				return;
			}
			
			_nodes.erase(std::remove(_nodes.begin(), _nodes.end(), node), _nodes.end());
			
			Unlock();
		}
	}

	
	void GenericSceneManager::RenderSceneNode(Camera *camera, SceneNode *node)
	{
		auto flags = node->GetFlags();
		
		if(!(camera->GetRenderGroups() & (1 << node-> GetRenderGroup())) || flags & SceneNode::Flags::Hidden)
			return;
		
		if(node->IsVisibleInCamera(camera))
		{
			node->Render(_renderer, camera);
			
			if(!(flags & SceneNode::Flags::HideChildren))
			{
				const Array *children = node->GetChildren();
				size_t count = children->GetCount();
				
				for(size_t i = 0; i < count; i++)
				{
					SceneNode *child = static_cast<SceneNode *>((*children)[i]);
					RenderSceneNode(camera, child);
				}
			}
		}
	}

	void GenericSceneManager::RenderScene(Camera *camera)
	{
		for(size_t i = 0; i < _nodes.size(); i ++)
		{
			SceneNode *node = _nodes[i];
			RenderSceneNode(camera, node);
		}
	}
	
	Hit GenericSceneManager::CastRay(const Vector3 &position, const Vector3 &direction, uint32 mask, Hit::HitMode mode)
	{
		World::GetActiveWorld()->ApplyNodes();
		
		Hit hit;
		for(auto i=_nodes.begin(); i!=_nodes.end(); i++)
		{
			SceneNode *node = *i;
			if(!(mask & (1 << node->GetCollisionGroup())))
				continue;
				
			Hit result = node->CastRay(position, direction, mode);
			
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
	
	std::vector<SceneNode *> GenericSceneManager::GetSceneNodes(const AABB &box)
	{
		std::vector<SceneNode *> nodes;
		
		World::GetActiveWorld()->ApplyNodes();
		
		for(SceneNode *node : _nodes)
		{
			if(node->GetBoundingBox().Intersects(box))
				nodes.push_back(node);
		}
		
		return nodes;
	}
}
