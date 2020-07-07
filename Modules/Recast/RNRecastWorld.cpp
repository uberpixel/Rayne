//
//  RNRecastWorld.cpp
//  Rayne-Recast
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRecastWorld.h"
#include "Recast.h"
#include "DetourCrowd.h"

namespace RN
{
	RNDefineMeta(RecastWorld, SceneAttachment)
    RNDefineMeta(RecastPath, Object)

	RecastWorld *RecastWorld::_instance = nullptr;

	RecastWorld* RecastWorld::GetInstance()
	{
		return _instance;
	}

	RecastWorld::RecastWorld() : _navMesh(nullptr), _paused(false)
	{
		RN_ASSERT(!_instance, "There already is a RecastWorld!");

		_recastContext = new rcContext();
		_crowdManager = new dtCrowd();
		
/*		dtObstacleAvoidanceParams params;
		memcpy(&params, _crowdManager->getObstacleAvoidanceParams(0), sizeof(dtObstacleAvoidanceParams));
		
		// Good (45)
		params.velBias = 0.5f;
		params.adaptiveDivs = 7;
		params.adaptiveRings = 2;
		params.adaptiveDepth = 3;
		_crowdManager->setObstacleAvoidanceParams(0, &params);*/

		_instance = this;
	}
		
	RecastWorld::~RecastWorld()
	{
		delete _recastContext;
		delete _crowdManager;
		
		_instance = nullptr;
	}
	
	void RecastWorld::SetRecastMesh(RecastMesh *navMesh, uint8 maxAgents)
	{
		SafeRelease(_navMesh);
		_navMesh = navMesh;
		SafeRetain(_navMesh);
		if(_navMesh)
		{
			_crowdManager->init(maxAgents, 0.3f, _navMesh->GetDetourNavMesh());
		}
	}
	
	RN::Vector3 RecastWorld::GetClosestPosition(Vector3 postion)
	{
		// Find nearest point on navmesh
		const dtNavMeshQuery* navquery = _crowdManager->getNavMeshQuery();
		const dtQueryFilter* filter = _crowdManager->getFilter(0);
		const float *ext = _crowdManager->getQueryHalfExtents();
		
		Vector3 closestPosition;
		dtPolyRef targetPolyRef;
		navquery->findNearestPoly(&postion.x, ext, filter, &targetPolyRef, &closestPosition.x);
		
		return closestPosition;
	}

    RecastPath *RecastWorld::FindPath(const RN::Vector3 &from, const RN::Vector3 &to)
    {
        const dtNavMeshQuery* navquery = _crowdManager->getNavMeshQuery();
        const dtQueryFilter* filter = _crowdManager->getFilter(0);
        const float *ext = _crowdManager->getQueryHalfExtents();
        
        dtPolyRef startPoly;
        dtPolyRef targetPoly;
        navquery->findNearestPoly(&from.x, ext, filter, &startPoly, nullptr);
        navquery->findNearestPoly(&to.x, ext, filter, &targetPoly, nullptr);
        
        RecastPath *finalPath = new RecastPath();
        
        if(startPoly && targetPoly)
        {
            int outCount;
            std::vector<dtPolyRef> path(2048);
            
            navquery->findPath(startPoly, targetPoly, &from.x, &to.x, filter, path.data(), &outCount, static_cast<int>(path.capacity()));
            
            path.resize(outCount);
            
            if(outCount)
            {
                finalPath->corners.resize(2048);
                
                navquery->findStraightPath(&from.x, &to.x, path.data(), static_cast<int>(path.size()), reinterpret_cast<float *>(finalPath->corners.data()), nullptr, nullptr, &outCount, static_cast<int>(finalPath->corners.capacity()));
                finalPath->corners.resize(outCount);
                
                std::reverse(finalPath->corners.begin(), finalPath->corners.end());
            }
        }
        
        return finalPath->Autorelease();
    }

	void RecastWorld::SetPaused(bool paused)
	{
		_paused = paused;
	}

	void RecastWorld::Update(float delta)
	{
		if(_navMesh && !_paused)
			_crowdManager->update(delta, nullptr);
	}
}
