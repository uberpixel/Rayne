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

#define kRNWorldCoordinatorWillBeginLoadingMessage RNCSTR("kRNWorldCoordinatorWillBeginLoadingMessage")
#define kRNWorldCoordinatorDidFinishLoadingMessage RNCSTR("kRNWorldCoordinatorDidFinishLoadingMessage")
#define kRNWorldCoordinatorDidStepWorldMessage     RNCSTR("kRNWorldCoordinatorDidStepWorldMessage")

namespace RN
{
	class Kernel;
	class WorldCoordinator : public ISingleton<WorldCoordinator>
	{
	public:
		friend class Kernel;
		
		RNAPI WorldCoordinator();
		RNAPI ~WorldCoordinator();
		
		RNAPI Progress *LoadWorld(World *world);
		RNAPI Progress *LoadWorld(const std::string &file);
		RNAPI Progress *LoadWorld(Deserializer *deserializer);
		
		RNAPI void SaveWorld(const std::string &file);
		RNAPI void SaveWorld(Serializer *serializer);
		
		RNAPI bool IsLoading() const { return _loading.load(); }
		RNAPI World *GetWorld() const { return _world; }
		RNAPI std::string GetWorldFile() const { return _worldFile; }
		
	private:
		void StepWorld(FrameID frame, float delta);
		void RenderWorld(Renderer *renderer);
		
		bool AwaitFinishLoading();
		bool BeginLoading(Deserializer *deserializer);
		void FinishLoading(bool state);
		Progress *__LoadWorld(Deserializer *deserializer);
		
		void BeginSaving(Serializer *serializer);
		
		void __AwaitLoadingForExit();
		
		Thread *_loadThread;
		World *_world;
		Mutex _lock;
		
		std::atomic<bool> _loading;
		std::future<bool> _loadFuture;
		Progress *_loadingProgress;
		uint32 _loadState;
		
		std::string _worldFile;
		
		Deserializer *_deserializer;
		
		RNDeclareSingleton(WorldCoordinator)
	};
}

#endif /* __RAYNE_WORLDCOORDINATOR_H__ */
