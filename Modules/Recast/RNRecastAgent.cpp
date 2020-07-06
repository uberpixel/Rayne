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

	RecastAgent::RecastAgent(Settings settings) : _owner(nullptr), _agentIndex(-1), _isEnabled(true)
	{
		_agentParams = new dtCrowdAgentParams();
		_agentParams->radius = settings.radius;
		_agentParams->height = settings.height;
		_agentParams->maxAcceleration = settings.maxAcceleration;
		_agentParams->maxSpeed = settings.maxSpeed;
		_agentParams->collisionQueryRange = settings.collisionQueryRange;
		_agentParams->pathOptimizationRange = settings.pathOptimizationRange;
		_agentParams->separationWeight = settings.separationWeight;
		_agentParams->updateFlags = settings.updateFlags;
        _agentParams->obstacleAvoidanceType = 0;//settings.obstacleAvoidanceType;
		_agentParams->queryFilterType = 0;
		_agentParams->userData = this;
	}
	
	RecastAgent::~RecastAgent()
	{
		delete _agentParams;
	}
	
	void RecastAgent::SetTarget(Vector3 target)
	{
        if(_agentIndex == -1) return;
        
		// Find nearest point on navmesh and set move request to that location.
		dtCrowd* crowd = RecastWorld::GetInstance()->GetCrowdManager();
		const dtNavMeshQuery* navquery = crowd->getNavMeshQuery();
		const dtQueryFilter* filter = crowd->getFilter(0);
		const float* ext = crowd->getQueryHalfExtents();
		Vector3 closestTarget;
		
		dtPolyRef targetPolyRef;
		navquery->findNearestPoly(&target.x, ext, filter, &targetPolyRef, &closestTarget.x);
		crowd->requestMoveTarget(_agentIndex, targetPolyRef,  &closestTarget.x);
	}
	
	void RecastAgent::Stop()
	{
        if(_agentIndex == -1) return;
        
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

    void RecastAgent::SetEnabled(bool enabled)
    {
        if(_isEnabled == enabled) return;
        _isEnabled = enabled;
        
        if(_agentIndex != -1 && !_isEnabled)
        {
            RecastWorld::GetInstance()->GetCrowdManager()->removeAgent(_agentIndex);
            _agentIndex = -1;
            _previousPosition = _currentPosition;
        }
        else if(_agentIndex == -1 && _isEnabled)
        {
            Vector3 position = GetWorldPosition() - _offset;
            position = RecastWorld::GetInstance()->GetClosestPosition(position);
            _agentIndex = RecastWorld::GetInstance()->GetCrowdManager()->addAgent(&position.x, _agentParams);
            _previousPosition = _currentPosition = position + _offset;
        }
    }
    
    void RecastAgent::SetPositionOffset(const Vector3 offset)
    {
        _offset = offset;
    }

    Vector3 RecastAgent::GetMoveDirection()
    {
        return _currentPosition - _previousPosition;
    }
	
	void RecastAgent::Update(float delta)
	{
		SceneNodeAttachment::Update(delta);
		
        if(_agentIndex != -1)
        {
            _previousPosition = _currentPosition;
            const dtCrowdAgent *agent = RecastWorld::GetInstance()->GetCrowdManager()->getAgent(_agentIndex);
            _currentPosition.x = agent->npos[0];
            _currentPosition.y = agent->npos[1];
            _currentPosition.z = agent->npos[2];
            
            _currentPosition += _offset;
		
            SetWorldPosition(_currentPosition);
        }
	}
	
	void RecastAgent::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		SceneNodeAttachment::DidUpdate(changeSet);
		
		//TODO: Implement teleport
		if(changeSet & SceneNode::ChangeSet::Position && _agentIndex != -1)
		{
			Vector3 position = GetWorldPosition();
            if(_currentPosition.GetSquaredDistance(position) > _agentParams->radius * _agentParams->radius * 0.25f) //if position differs more than half the agent radius, needed cause otherwise rotation changes will also trigger this breaking the pathfinding
            {
                position = RecastWorld::GetInstance()->GetClosestPosition(position) - _offset;
                RecastWorld::GetInstance()->GetCrowdManager()->removeAgent(_agentIndex);
                _agentIndex = RecastWorld::GetInstance()->GetCrowdManager()->addAgent(&position.x, _agentParams);
                _previousPosition = _currentPosition = position + _offset;
            }
		}
		
		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent() && _agentIndex == -1)
			{
                if(_isEnabled)
                {
                    Vector3 position = GetWorldPosition() - _offset;
                    position = RecastWorld::GetInstance()->GetClosestPosition(position);
                    _agentIndex = RecastWorld::GetInstance()->GetCrowdManager()->addAgent(&position.x, _agentParams);
                    _previousPosition = _currentPosition = position + _offset;
                }
			}
			else
			{
				if(!GetParent() && _agentIndex != -1)
				{
					RecastWorld::GetInstance()->GetCrowdManager()->removeAgent(_agentIndex);
					_agentIndex = -1;
                    _previousPosition = _currentPosition;
				}
			}
			
			_owner = GetParent();
		}
	}
}
