//
//  RNWorldCoordinator.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorldCoordinator.h"
#include "RNAutoreleasePool.h"
#include "RNMessage.h"

namespace RN
{
	RNDefineSingleton(WorldCoordinator)
	
	WorldCoordinator::WorldCoordinator() :
		_world(nullptr),
		_loading(false)
	{}
	
	WorldCoordinator::~WorldCoordinator()
	{
		if(IsLoading())
		{
			try
			{
				_loadFuture.wait();
				AwaitFinishLoading();
			}
			catch(Exception e)
			{}
		}
		
		SafeRelease(_world);
	}
	
	
	void WorldCoordinator::StepWorld(FrameID frame, float delta)
	{
		if(_loading)
		{
			bool result = AwaitFinishLoading();
			if(!result)
				return;
		}
		
		if(_world)
			_world->StepWorld(frame, delta);
	}
	
	void WorldCoordinator::RenderWorld(Renderer *renderer)
	{
		if(_loading)
			return;
		
		if(_world)
			_world->RenderWorld(renderer);
	}
	
	

	
	Progress *WorldCoordinator::LoadWorld(World *world)
	{
		_lock.Lock();
		
		SafeRelease(_world);
		_world = SafeRetain(world);
		_loading.store(true);
		
		std::packaged_task<bool ()> task(std::bind(&WorldCoordinator::BeginLoading, this));
		_loadFuture = task.get_future();
		_loadingProgress = new Progress(0);
		
		if(_world->SupportsBackgroundLoading())
		{
			_loadThread = new Thread(std::move(task), false);
			_loadThread->SetName("WorldCoordinator");
			_loadThread->Detach();
		}
		else
		{
			_loadThread = nullptr;
			task();
		}
		
		return _loadingProgress;
	}
	
	bool WorldCoordinator::BeginLoading()
	{
		AutoreleasePool pool;
		
		MessageCenter::GetSharedInstance()->PostMessage(kRNWorldCoordinatorWillBeginLoading, _world, nullptr);
		
		_loadingProgress->MakeActive();
		_world->LoadOnThread(Thread::GetCurrentThread());
		
		return true;
	}
	
	bool WorldCoordinator::AwaitFinishLoading()
	{
		std::future_status status = _loadFuture.wait_for(std::chrono::milliseconds(5));
		if(status == std::future_status::ready)
		{
			try
			{
				FinishLoading(_loadFuture.get());
				return true;
			}
			catch(Exception e)
			{
				FinishLoading(false);
				throw e;
			}
		}
		
		return false;
	}
	
	void WorldCoordinator::FinishLoading(bool state)
	{
		if(_loadThread)
			_loadThread->Release();
		
		_loading.store(false);
		
		if(!state)
			SafeRelease(_world);
		else
			_world->FinishLoading();
		
		MessageCenter::GetSharedInstance()->PostMessage(kRNWorldCoordinatorDidFinishLoading, _world, nullptr);
		
		_loadingProgress->Release();
		_loadingProgress = nullptr;
		_lock.Unlock();
	}
}
