//
//  RNTriggerZone.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		
		TriggerZone();
		~TriggerZone() override;
		
		virtual void Trigger(SceneNode *node, Predicate reason);
		
		virtual void BeginTrackingSceneNode(SceneNode *node);
		virtual void ContinueTrackingSceneNode(SceneNode *node);
		virtual void EndTrackingSceneNode(SceneNode *node);
		virtual bool ValidateSceneNode(SceneNode *node);
		
		void AddWatchedNode(SceneNode *node);
		void RemoveWatchedNode(SceneNode *node);
		
		void SetPredicate(Predicate predicate);
		
	private:
		void ValidateNodeUpdate(SceneNode *node);
		
		Predicate _predicate;
		
		std::unordered_set<SceneNode *> _trackingNodes;
		std::unordered_set<SceneNode *> _watchedNodes;
		
		RNDefineMeta(TriggerZone, SceneNode)
	};
}

#endif /* __RAYNE_TRIGGERZONE_H__ */
