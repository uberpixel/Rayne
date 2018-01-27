//
//  RNRecastAgent.cpp
//  Rayne-Recast
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRecastAgent.h"
#include "DetourCrowd.h"

#include "RNRecastWorld.h"

namespace RN
{
	RNDefineMeta(RecastAgent, SceneNodeAttachment)

	RecastAgent::RecastAgent() : _owner(nullptr), _didUpdatePosition(false), _agentIndex(-1)
	{
		_agentParams = new dtCrowdAgentParams();
		_agentParams->radius = 0.25f;
		_agentParams->height = 1.8f;
		_agentParams->maxAcceleration = 10.0f;
		_agentParams->maxSpeed = 10.0f;
		_agentParams->collisionQueryRange = 1.0f;
		_agentParams->pathOptimizationRange = 20.0f;
		_agentParams->separationWeight = 0.1f;
		_agentParams->updateFlags = DT_CROWD_ANTICIPATE_TURNS|DT_CROWD_OBSTACLE_AVOIDANCE;
		_agentParams->obstacleAvoidanceType = 0;
		_agentParams->queryFilterType = 0;
		_agentParams->userData = this;
	}
	
	RecastAgent::~RecastAgent()
	{
		delete _agentParams;
	}
	
	void RecastAgent::Update(float delta)
	{
		SceneNodeAttachment::Update(delta);
		
		const dtCrowdAgent *agent = RecastWorld::GetInstance()->GetCrowdManager()->getAgent(_agentIndex);
		Vector3 position(agent->npos[0], agent->npos[1], agent->npos[2]);
		
		_didUpdatePosition = true;
		SetWorldPosition(position);
	}
	
	void RecastAgent::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		SceneNodeAttachment::DidUpdate(changeSet);
		
		//TODO: Implement teleport
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			if(!_didUpdatePosition)
			{
				Vector3 position = GetWorldPosition();
				RecastWorld::GetInstance()->GetCrowdManager()->removeAgent(_agentIndex);
				_agentIndex = RecastWorld::GetInstance()->GetCrowdManager()->addAgent(&position.x, _agentParams);
			}
			_didUpdatePosition = false;
		}
		
		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent() && _agentIndex == -1)
			{
				Vector3 position = GetWorldPosition();
				_agentIndex = RecastWorld::GetInstance()->GetCrowdManager()->addAgent(&position.x, _agentParams);
			}
			else
			{
				if(!GetParent() && _agentIndex == -1)
				{
					RecastWorld::GetInstance()->GetCrowdManager()->removeAgent(_agentIndex);
					_agentIndex = -1;
				}
			}
			
			_owner = GetParent();
		}
	}
}
