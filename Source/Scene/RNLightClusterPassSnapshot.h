//
//  RNLightClusterPassSnapshot.h
//  Rayne
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LIGHT_CLUSTER_PASS_SNAPSHOT_H__
#define __RAYNE_LIGHT_CLUSTER_PASS_SNAPSHOT_H__

#include "RNLightManager.h"

namespace RN
{
	class LightClusterPassSnapshot : public Object
	{
	public:
		LightClusterPassSnapshot(LightManager *lightManager, LightManager::BuildInput &&input) :
			_lightManager(lightManager),
			_input(std::move(input))
		{}

		// A camera snapshot may be shared by several passes. Build and retain its buffers once.
		const LightManager::DrawSnapshot &PrepareDrawSnapshot()
		{
			if(!_snapshot.IsValid())
				_snapshot = _lightManager->BuildDrawSnapshot(std::move(_input));
			return _snapshot;
		}

	private:
		StrongRef<LightManager> _lightManager;
		LightManager::BuildInput _input;
		LightManager::DrawSnapshot _snapshot;

		__RNDeclareMetaInternal(LightClusterPassSnapshot)
	};
} // namespace RN

#endif
