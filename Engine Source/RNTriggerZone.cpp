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
			LockGuard<TriggerZoneManager *> lock(this);
			_zones.push_back(zone);
		}
		
		void RemoveZone(TriggerZone *zone)
		{
			LockGuard<TriggerZoneManager *> lock(this);
			_zones.erase(std::find(_zones.begin(), _zones.end(), zone));
		}
		
		void SceneNodeDidUpdate(SceneNode *node) override
		{
			LockGuard<TriggerZoneManager *> lock(this);
			
			for(size_t i = 0; i < _zones.size(); i ++)
			{
				try
				{
					_zones[i]->ValidateNodeUpdate(node);
				}
				catch(Exception e)
				{}
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
	
	void TriggerZone::Trigger(SceneNode *node, Predicate reason)
	{}
	
	
	
	void TriggerZone::BeginTrackingSceneNode(SceneNode *node)
	{
		if((_predicate & OnEnter))
		{
			Trigger(node, OnEnter);
		}
		
		if((_predicate & OnAllEnter) && _watchedNodes.size() == _trackingNodes.size())
		{
			Trigger(node, OnAllEnter);
		}
	}
	
	void TriggerZone::ContinueTrackingSceneNode(SceneNode *node)
	{}
	
	void TriggerZone::EndTrackingSceneNode(SceneNode *node)
	{
		if((_predicate & OnLeave))
		{
			Trigger(node, OnLeave);
		}
	}
	
	
	
	bool TriggerZone::ValidateSceneNode(SceneNode *node)
	{
		return GetBoundingBox().Intersects(node->GetBoundingBox());
	}
	
	
	
	void TriggerZone::AddWatchedNode(SceneNode *node)
	{
		Lock();
		
		node->Retain();
		
		_watchedNodes.insert(node);
		ValidateSceneNode(node);
		
		Unlock();
	}
	
	void TriggerZone::RemoveWatchedNode(SceneNode *node)
	{
		Lock();
		
		_trackingNodes.erase(node);
		_watchedNodes.erase(node);
		
		node->Release();
		
		Unlock();
	}
	
	void TriggerZone::ValidateNodeUpdate(SceneNode *node)
	{
		if(_watchedNodes.find(node) != _watchedNodes.end())
		{
			bool result = ValidateSceneNode(node);
			if(result)
			{
				LockGuard<TriggerZone *> lock(this);
				
				if(_trackingNodes.find(node) != _trackingNodes.end())
				{
					lock.Unlock();
					
					ContinueTrackingSceneNode(node);
					return;
				}
				
				_trackingNodes.insert(node);
				lock.Unlock();
				
				BeginTrackingSceneNode(node);
			}
			else if(_trackingNodes.find(node) != _trackingNodes.end())
			{
				LockGuard<TriggerZone *> lock(this);
				_trackingNodes.erase(node);
				lock.Unlock();
				
				EndTrackingSceneNode(node);
			}
		}
	}
}
