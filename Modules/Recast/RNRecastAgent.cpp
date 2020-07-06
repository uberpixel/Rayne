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

	RecastAgent::RecastAgent(Settings settings) : _owner(nullptr), _agentIndex(-1)
	{
		_agentParams = new dtCrowdAgentParams();
		_agentParams->radius = settings.radius;
		_agentParams->height = settings.height;
		_agentParams->maxAcceleration = settings.maxAcceleration;
		_agentParams->maxSpeed = settings.maxSpeed;
		_agentParams->collisionQueryRange = settings.collisionQueryRange;
		_agentParams->pathOptimizationRange = settings.pathOptimizationRange;
		_agentParams->separationWeight = settings.separationWeight;
		_agentParams->updateFlags = DT_CROWD_ANTICIPATE_TURNS|DT_CROWD_OBSTACLE_AVOIDANCE;
		_agentParams->obstacleAvoidanceType = 0;
		_agentParams->queryFilterType = 0;
		_agentParams->userData = this;
	}
	
	RecastAgent::~RecastAgent()
	{
		delete _agentParams;
	}
	
	void RecastAgent::SetTarget(Vector3 target)
	{
/*		const dtCrowdAgent* ag = crowd->getAgent(i);
		if (!ag->active) continue;
		calcVel(vel, ag->npos, p, ag->params.maxSpeed);
		crowd->requestMoveVelocity(i, vel);*/
		
		// Find nearest point on navmesh and set move request to that location.
		dtCrowd* crowd = RecastWorld::GetInstance()->GetCrowdManager();
		const dtNavMeshQuery* navquery = crowd->getNavMeshQuery();
		const dtQueryFilter* filter = crowd->getFilter(0);
		const float* ext = crowd->getQueryExtents();
		Vector3 closestTarget;
		
		dtPolyRef targetPolyRef;
		navquery->findNearestPoly(&target.x, ext, filter, &targetPolyRef, &closestTarget.x);
		crowd->requestMoveTarget(_agentIndex, targetPolyRef,  &closestTarget.x);
//		crowd->requestMoveVelocity(_agentIndex, &velocity.x);
		
/*		if (adjust)
		{
			float vel[3];
			// Request velocity
			if (m_agentDebug.idx != -1)
			{
				const dtCrowdAgent* ag = crowd->getAgent(m_agentDebug.idx);
				if (ag && ag->active)
				{
					calcVel(vel, ag->npos, p, ag->params.maxSpeed);
					crowd->requestMoveVelocity(m_agentDebug.idx, vel);
				}
			}
			else
			{
				for (int i = 0; i < crowd->getAgentCount(); ++i)
				{
					const dtCrowdAgent* ag = crowd->getAgent(i);
					if (!ag->active) continue;
					calcVel(vel, ag->npos, p, ag->params.maxSpeed);
					crowd->requestMoveVelocity(i, vel);
				}
			}
		}*/
	}
	
	void RecastAgent::Stop()
	{
		RecastWorld::GetInstance()->GetCrowdManager()->resetMoveTarget(_agentIndex);
	}
	
	void RecastAgent::UpdateSettings(Settings settings)
	{
		_agentParams->radius = settings.radius;
		_agentParams->height = settings.height;
		_agentParams->maxAcceleration = settings.maxAcceleration;
		_agentParams->maxSpeed = settings.maxSpeed;
		
		if(_agentIndex != -1)
		{
			RecastWorld::GetInstance()->GetCrowdManager()->updateAgentParameters(_agentIndex, _agentParams);
		}
	}
	
	void RecastAgent::Update(float delta)
	{
		SceneNodeAttachment::Update(delta);
		
		const dtCrowdAgent *agent = RecastWorld::GetInstance()->GetCrowdManager()->getAgent(_agentIndex);
		Vector3 position(agent->npos[0], agent->npos[1], agent->npos[2]);
		
		SetWorldPosition(position);
	}
	
	void RecastAgent::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		SceneNodeAttachment::DidUpdate(changeSet);
		
		//TODO: Implement teleport
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			Vector3 position = GetWorldPosition();
			position = RecastWorld::GetInstance()->GetClosestPosition(position);
			RecastWorld::GetInstance()->GetCrowdManager()->removeAgent(_agentIndex);
			_agentIndex = RecastWorld::GetInstance()->GetCrowdManager()->addAgent(&position.x, _agentParams);
		}
		
		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent() && _agentIndex == -1)
			{
				Vector3 position = GetWorldPosition();
				position = RecastWorld::GetInstance()->GetClosestPosition(position);
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
