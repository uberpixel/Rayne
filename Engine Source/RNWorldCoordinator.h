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

#define kRNWorldCoordinatorDidFinishLoading RNCSTR("kRNWorldCoordinatorDidFinishLoading")
#define kRNWorldCoordinatorWillBeginLoading RNCSTR("kRNWorldCoordinatorWillBeginLoading")

namespace RN
{
	class WorldCoordinator : public ISingleton<WorldCoordinator>
	{
	public:
		WorldCoordinator();
		~WorldCoordinator();
		
		void StepWorld(FrameID frame, float delta);
		void RenderWorld(Renderer *renderer);
		
		void LoadWorld(const std::string &name);
		void LoadWorld(World *world);
		
		bool IsLoading() const { return _loading.load(); }
		
		World *GetWorld() const { return _world; }
		
	private:
		bool AwaitFinishLoading();
		bool BeginLoading();
		void FinishLoading(bool state);
		
		Thread *_loadThread;
		World *_world;
		Mutex _lock;
		
		std::atomic<bool> _loading;
		std::future<bool> _loadFuture;
		
		RNDefineSingleton(WorldCoordinator)
	};
}

#endif /* __RAYNE_WORLDCOORDINATOR_H__ */
