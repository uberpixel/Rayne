//
//  RNRecastWorld.h
//  Rayne-Recast
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RECASTWORLD_H_
#define __RAYNE_RECASTWORLD_H_

#include "RNRecast.h"

#include "RNRecastMesh.h"
#include "RNRecastAgent.h"

class rcContext;
class dtCrowd;

namespace RN
{
	class RecastWorld : public SceneAttachment
	{
	public:
		RCAPI static RecastWorld *GetInstance();
		
		RCAPI rcContext *GetRecastContext() const { return _recastContext; }
		RCAPI dtCrowd *GetCrowdManager() const { return _crowdManager; }
		
		RCAPI void SetRecastMesh(RecastMesh *navMesh, uint8 maxAgents);
		RCAPI RN::Vector3 GetClosestPosition(Vector3 postion);

		RCAPI void SetPaused(bool paused);

		RCAPI RecastWorld();
		RCAPI ~RecastWorld() override;

	protected:
		void Update(float delta) override;
			
	private:
		static RecastWorld *_instance;
		
		RecastMesh *_navMesh;
		rcContext *_recastContext;
		dtCrowd *_crowdManager;

		bool _paused;
			
		RNDeclareMetaAPI(RecastWorld, RCAPI)
	};
}

#endif /* defined(__RAYNE_RECASTWORLD_H_) */
