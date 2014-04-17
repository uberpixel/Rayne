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
		{
			_world->StepWorld(frame, delta);
			MessageCenter::GetSharedInstance()->PostMessage(kRNWorldCoordinatorDidStepWorldMessage, _world, nullptr);
		}
	}
	
	void WorldCoordinator::RenderWorld(Renderer *renderer)
	{
		if(_loading)
			return;
		
		if(_world)
			_world->RenderWorld(renderer);
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Loading
	// ---------------------
	
	Progress *WorldCoordinator::LoadWorld(World *world)
	{
		_lock.Lock();
		
		if(_loading)
		{
			_lock.Unlock();
			throw Exception(Exception::Type::InconsistencyException, "Tried to load a World while another one is already loading");
		}
		
		SafeRelease(_world);
		_world = SafeRetain(world);
		_loading.store(true);
		
		_deserializer = nullptr;
		_worldFile = std::string();
		
		std::packaged_task<bool ()> task(std::bind(&WorldCoordinator::BeginLoading, this, nullptr));
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
	
	Progress *WorldCoordinator::LoadWorld(const std::string &file)
	{
		_lock.Lock();
		
		if(_loading)
		{
			_lock.Unlock();
			throw Exception(Exception::Type::InconsistencyException, "Tried to load a World while another one is already loading");
		}
		
		
		_deserializer = new FlatDeserializer(Data::WithContentsOfFile(file));
		_worldFile = file;
		
		return __LoadWorld(_deserializer);
	}
	
	Progress *WorldCoordinator::LoadWorld(Deserializer *deserializer)
	{
		_lock.Lock();
		
		if(_loading)
		{
			_lock.Unlock();
			throw Exception(Exception::Type::InconsistencyException, "Tried to load a World while another one is already loading");
		}
		
		
		_deserializer = SafeRetain(deserializer);
		_worldFile = std::string();
		
		return __LoadWorld(_deserializer);
	}
	
	Progress *WorldCoordinator::__LoadWorld(Deserializer *deserializer)
	{
		int32 magic   = deserializer->DecodeInt32();
		int32 version = deserializer->DecodeInt32();
		
		if(magic != 0xdeadf00d)
		{
			_lock.Unlock();
			throw Exception(Exception::Type::InconsistencyException, "Invalid magic for World");
		}
		
		if(version != 0)
		{
			_lock.Unlock();
			throw Exception(Exception::Type::InconsistencyException, "Invalid version for World");
		}
		
		
		std::string mname = deserializer->DecodeString();
		MetaClass *meta = Catalogue::GetSharedInstance()->GetClassWithName(mname);
		
		
		SafeRelease(_world);
		_world = static_cast<World *>(meta->Construct());
		_loading.store(true);
		
		std::packaged_task<bool ()> task(std::bind(&WorldCoordinator::BeginLoading, this, _deserializer));
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
	
	bool WorldCoordinator::BeginLoading(Deserializer *deserializer)
	{
		AutoreleasePool pool;
		
		MessageCenter::GetSharedInstance()->PostMessage(kRNWorldCoordinatorWillBeginLoadingMessage, _world, nullptr);
		
		_loadState = 0;
		_loadingProgress->MakeActive();
		_world->LoadOnThread(Thread::GetCurrentThread(), deserializer);
		
		return true;
	}
	
	bool WorldCoordinator::AwaitFinishLoading()
	{
		if(!_loading.load())
			return true;
		
		if(_loadState >= 1)
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
		
		std::future_status status = _loadFuture.wait_for(std::chrono::milliseconds(5));
		if(status == std::future_status::ready)
		{
			_loadState ++;
			return false;
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
			_world->FinishLoading(_deserializer);
		
		MessageCenter::GetSharedInstance()->PostMessage(kRNWorldCoordinatorDidFinishLoadingMessage, _world, nullptr);
		
		RN::SafeRelease(_deserializer);
		
		_loadingProgress->SetCompletedUnits(_loadingProgress->GetTotalUnits());
		_loadingProgress->Release();
		_loadingProgress = nullptr;
		_lock.Unlock();
	}
	
	void WorldCoordinator::__AwaitLoadingForExit()
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
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Saving
	// ---------------------
	
	void WorldCoordinator::SaveWorld(const std::string &file)
	{
		FlatSerializer *serializer = new FlatSerializer();
		
		LockGuard<Mutex> lock(_lock);
		BeginSaving(serializer);
		lock.Unlock();
		
		// Write to file
		Data *data = serializer->GetSerializedData();
		data->WriteToFile(file);
		
		serializer->Release();
		_worldFile = file;
	}
	
	void WorldCoordinator::SaveWorld(Serializer *serializer)
	{
		LockGuard<Mutex> lock(_lock);
		BeginSaving(serializer);
		lock.Unlock();
	}
	
	void WorldCoordinator::BeginSaving(Serializer *serializer)
	{
		serializer->EncodeInt32(0xdeadf00d);
		serializer->EncodeInt32(0);
		
		serializer->EncodeString(_world->GetClass()->GetFullname().c_str());
		
		_world->SaveOnThread(Thread::GetCurrentThread(), serializer);
		_world->FinishSaving(serializer);
	}
}
