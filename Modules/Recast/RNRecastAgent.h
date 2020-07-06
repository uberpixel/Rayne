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
        enum UpdateFlags
        {
            UpdateFlagsAnticipateTurns = 1 << 0,
            UpdateFlagsObstacleAvoidance = 1 << 1,
            UpdateFlagsSeparation = 1 << 2,
            UpdateFlagsOptimizeVis = 1 << 3,
            UpdateFlagsOptimizeTopo = 1 << 4
        };
        
		class Settings
		{
		public:
            Settings() : radius(0.2), height(1.8f), maxAcceleration(5.0f), maxSpeed(2.0f), collisionQueryRange(1.5f), pathOptimizationRange(20.0f), separationWeight(0.1f), /*obstacleAvoidanceType(0),*/ updateFlags(UpdateFlagsAnticipateTurns | UpdateFlagsObstacleAvoidance)
			{
				
			}
			float radius;
			float height;
			float maxAcceleration;
			float maxSpeed;
            float collisionQueryRange;
            float pathOptimizationRange;
            float separationWeight;
            //uint8 obstacleAvoidanceType;
            uint8 updateFlags;
		};
		
		RCAPI RecastAgent(Settings settings);
		RCAPI ~RecastAgent();
		
		RCAPI void SetTarget(Vector3 target);
		RCAPI void Stop();
		RCAPI void UpdateSettings(Settings settings);
        RCAPI void SetEnabled(bool enabled);
        RCAPI Vector3 GetMoveDirection();
        RCAPI void SetPositionOffset(const Vector3 offset);
		
		RCAPI void Update(float delta) override;
		RCAPI void DidUpdate(SceneNode::ChangeSet changeSet) override;
		
	private:
		dtCrowdAgentParams *_agentParams;
		uint32 _agentIndex;
		SceneNode *_owner;
        bool _isEnabled;
        Vector3 _currentPosition;
        Vector3 _previousPosition;
        Vector3 _offset;
		
		RNDeclareMetaAPI(RecastAgent, RCAPI)
	};
}

#endif /* defined(__RAYNE_RECASTAGENT_H_) */
