//
//  RNOctreeSceneManager.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOctreeSceneManager.h"

namespace RN
{
	RNDeclareMeta(OctreeSceneManager)

	OctreeSceneManager::OctreeSceneManager()
	{}
	
	OctreeSceneManager::~OctreeSceneManager()
	{}
	
	
	void OctreeSceneManager::AddSceneNode(SceneNode *node)
	{
		Lock();
		
		if(!node->GetParent())
		{
			_nodes.push_back(node);
		}
		
		Unlock();
	}
	
	void OctreeSceneManager::RemoveSceneNode(SceneNode *node)
	{
		Lock();
		
		_nodes.erase(std::remove(_nodes.begin(), _nodes.end(), node), _nodes.end());
		
		Unlock();
	}
	
	void OctreeSceneManager::UpdateSceneNode(SceneNode *node, uint32 changes)
	{
		if(changes & SceneNode::ChangedParent)
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

	
	void OctreeSceneManager::RenderSceneNode(Camera *camera, SceneNode *node)
	{
		if(!(camera->renderGroup & (1 << node->renderGroup)) || node->GetFlags() & SceneNode::FlagHidden)
			return;
		
		if(node->IsVisibleInCamera(camera))
		{
			node->Render(_renderer, camera);
			
			const Array *children = node->GetChildren();
			size_t count = children->GetCount();
			
			for(size_t i = 0; i < count; i++)
			{
				SceneNode *child = children->GetObjectAtIndex<SceneNode>(i);
				RenderSceneNode(camera, child);
			}
		}
	}

	void OctreeSceneManager::RenderScene(Camera *camera)
	{
		for(size_t i = 0; i < _nodes.size(); i ++)
		{
			SceneNode *node = _nodes[i];
			RenderSceneNode(camera, node);
		}
	}
	
	Hit OctreeSceneManager::CastRay(const Vector3 &position, const Vector3 &direction, uint32 mask, Hit::HitMode mode)
	{
		Hit hit;
		for(auto i=_nodes.begin(); i!=_nodes.end(); i++)
		{
			SceneNode *node = *i;
			if(!(mask & (1 << node->collisionGroup)))
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
}
