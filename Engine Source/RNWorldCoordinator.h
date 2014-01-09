//
//  RNWorldCoordinator.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WORLDCOORDINATOR_H__
#define __RAYNE_WORLDCOORDINATOR_H__

#include "RNBase.h"
#include "RNMutex.h"
#include "RNWorld.h"
#include "RNProgress.h"

#define kRNWorldCoordinatorDidFinishLoading RNCSTR("kRNWorldCoordinatorDidFinishLoading")
#define kRNWorldCoordinatorWillBeginLoading RNCSTR("kRNWorldCoordinatorWillBeginLoading")

namespace RN
{
	class WorldCoordinator : public ISingleton<WorldCoordinator>
	{
	public:
		RNAPI WorldCoordinator();
		RNAPI ~WorldCoordinator();
		
		RNAPI void StepWorld(FrameID frame, float delta);
		RNAPI void RenderWorld(Renderer *renderer);
		
		RNAPI Progress *LoadWorld(World *world);
		
		RNAPI bool IsLoading() const { return _loading.load(); }
		RNAPI World *GetWorld() const { return _world; }
		
	private:
		bool AwaitFinishLoading();
		bool BeginLoading();
		void FinishLoading(bool state);
		
		Thread *_loadThread;
		World *_world;
		Mutex _lock;
		
		std::atomic<bool> _loading;
		std::future<bool> _loadFuture;
		Progress *_loadingProgress;
		
		RNDefineSingleton(WorldCoordinator)
	};
}

#endif /* __RAYNE_WORLDCOORDINATOR_H__ */
