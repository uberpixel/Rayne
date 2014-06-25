//
//  RNTriggerZone.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TRIGGERZONE_H__
#define __RAYNE_TRIGGERZONE_H__

#include "RNBase.h"
#include "RNSceneNode.h"

namespace RN
{
	class TriggerZoneManager;
	class TriggerZone : public SceneNode
	{
	public:
		friend class TriggerZoneManager;
		
		enum
		{
			OnEnter = (1 << 0),
			OnLeave = (1 << 1),
			OnAllEnter = (1 << 2)
		};
		typedef uint32 Predicate;
		
		RNAPI TriggerZone();
		RNAPI ~TriggerZone() override;
		
		RNAPI virtual void Trigger(SceneNode *node, Predicate reason);
		
		RNAPI virtual void BeginTrackingSceneNode(SceneNode *node);
		RNAPI virtual void ContinueTrackingSceneNode(SceneNode *node);
		RNAPI virtual void EndTrackingSceneNode(SceneNode *node);
		RNAPI virtual bool ValidateSceneNode(SceneNode *node);
		
		RNAPI void AddWatchedNode(SceneNode *node);
		RNAPI void RemoveWatchedNode(SceneNode *node);
		
		RNAPI void SetPredicate(Predicate predicate);
		
	private:
		void ValidateNodeUpdate(SceneNode *node);
		
		Predicate _predicate;
		
		std::unordered_set<SceneNode *> _trackingNodes;
		std::unordered_set<SceneNode *> _watchedNodes;
		
		RNDeclareMeta(TriggerZone)
	};
	
	RNObjectClass(TriggerZone)
}

#endif /* __RAYNE_TRIGGERZONE_H__ */
