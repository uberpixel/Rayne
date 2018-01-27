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
		RCAPI RecastAgent();
		RCAPI ~RecastAgent();
		
		RCAPI void SetTarget(Vector3 target, Vector3 velocity);
		RCAPI void Stop();
		
		RCAPI void Update(float delta) override;
		RCAPI void DidUpdate(SceneNode::ChangeSet changeSet) override;
		
	private:
		dtCrowdAgentParams *_agentParams;
		uint32 _agentIndex;
		SceneNode *_owner;
		bool _didUpdatePosition;
		
		RNDeclareMetaAPI(RecastAgent, RCAPI)
	};
}

#endif /* defined(__RAYNE_RECASTAGENT_H_) */
