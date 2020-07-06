//
//  RNRecastAgent.h
//  Rayne-Recast
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RECASTAGENT_H_
#define __RAYNE_RECASTAGENT_H_

#include "RNRecast.h"
#include "RNRecastMesh.h"

struct dtCrowdAgentParams;

namespace RN
{
	class RecastAgent : public SceneNodeAttachment
	{
	public:
		class Settings
		{
		public:
			Settings() : radius(0.2), height(1.8f), maxAcceleration(5.0f), maxSpeed(2.0f), collisionQueryRange(1.5f), pathOptimizationRange(20.0f), separationWeight(0.1f)
			{
				
			}
			float radius;
			float height;
			float maxAcceleration;
			float maxSpeed;
            float collisionQueryRange;
            float pathOptimizationRange;
            float separationWeight;
		};
		
		RCAPI RecastAgent(Settings settings);
		RCAPI ~RecastAgent();
		
		RCAPI void SetTarget(Vector3 target);
		RCAPI void Stop();
		RCAPI void UpdateSettings(Settings settings);
		
		RCAPI void Update(float delta) override;
		RCAPI void DidUpdate(SceneNode::ChangeSet changeSet) override;
		
	private:
		dtCrowdAgentParams *_agentParams;
		uint32 _agentIndex;
		SceneNode *_owner;
		
		RNDeclareMetaAPI(RecastAgent, RCAPI)
	};
}

#endif /* defined(__RAYNE_RECASTAGENT_H_) */
