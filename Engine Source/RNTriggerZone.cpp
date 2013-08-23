//
//  RNTriggerZone.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTriggerZone.h"
#include "RNWorld.h"
#include "RNWorldAttachment.h"

namespace RN
{
	class TriggerZoneManager : public WorldAttachment, public Singleton<TriggerZoneManager>
	{
	public:
		TriggerZoneManager()
		{
			World::GetSharedInstance()->AddAttachment(this);
		}
		
		~TriggerZoneManager()
		{}
		
		void AddZone(TriggerZone *zone)
		{
			_zones.push_back(zone);
		}
		
		void RemoveZone(TriggerZone *zone)
		{
			_zones.erase(std::find(_zones.begin(), _zones.end(), zone));
		}
		
		void SceneNodeDidUpdate(SceneNode *node) override
		{
			for(size_t i = 0; i < _zones.size(); i ++)
			{
				_zones[i]->ValidateNodeUpdate(node);
			}
		}
		
	private:
		std::vector<TriggerZone *> _zones;
		
		RNDefineMeta(TriggerZoneManager, WorldAttachment);
	};
	
	
	RNDeclareMeta(TriggerZoneManager)
	RNDeclareMeta(TriggerZone)
	
	
	TriggerZone::TriggerZone()
	{
		TriggerZoneManager::GetSharedInstance()->AddZone(this);
		
		_predicate = 0;
	}
	
	TriggerZone::~TriggerZone()
	{
		TriggerZoneManager::GetSharedInstance()->RemoveZone(this);
		
		for(auto i = _watchedNodes.begin(); i != _watchedNodes.end(); i ++)
		{
			SceneNode *node = *i;
			node->Release();
		}
	}
	
	
	void TriggerZone::SetPredicate(Predicate predicate)
	{
		_predicate = predicate;
	}
	
	void TriggerZone::Trigger(SceneNode *node)
	{
	}
	
	
	
	void TriggerZone::BeginTrackingSceneNode(SceneNode *node)
	{
		if((_predicate & OnEnter))
		{
			Trigger(node);
		}
	}
	
	void TriggerZone::ContinueTrackingSceneNode(SceneNode *node)
	{
	}
	
	void TriggerZone::EndTrackingSceneNode(SceneNode *node)
	{
		if((_predicate & OnLeave))
		{
			Trigger(node);
		}
	}
	
	
	void TriggerZone::ValidateSceneNode(SceneNode *node)
	{
		if(GetBoundingBox().Intersects(node->GetBoundingBox()))
		{
			if(_trackingNodes.find(node) != _trackingNodes.end())
			{
				ContinueTrackingSceneNode(node);
				return;
			}
			
			BeginTrackingSceneNode(node);
			_watchedNodes.insert(node);
		}
		else
		{
			if(_trackingNodes.find(node) != _trackingNodes.end())
			{
				EndTrackingSceneNode(node);
				_trackingNodes.erase(node);
			}
		}
	}
	
	
	void TriggerZone::AddWatchedNode(SceneNode *node)
	{
		node->Retain();
		
		_watchedNodes.insert(node);
		ValidateSceneNode(node);
	}
	
	void TriggerZone::RemoveWatchedNode(SceneNode *node)
	{
		_trackingNodes.erase(node);
		_watchedNodes.erase(node);
		
		node->Release();
	}
	
	void TriggerZone::ValidateNodeUpdate(SceneNode *node)
	{
		if(_watchedNodes.find(node) != _watchedNodes.end())
		{
			ValidateSceneNode(node);
		}
	}
}
