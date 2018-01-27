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

	RecastWorld *RecastWorld::_instance = nullptr;

	RecastWorld* RecastWorld::GetInstance()
	{
		return _instance;
	}

	RecastWorld::RecastWorld() : _navMesh(nullptr)
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

	void RecastWorld::Update(float delta)
	{
		if(_navMesh)
			_crowdManager->update(delta, nullptr);
	}
}
