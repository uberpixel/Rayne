//
//  RNENetWorld.cpp
//  Rayne-ENet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNENetWorld.h"
#include "enet/enet.h"

namespace RN
{
	RNDefineMeta(ENetWorld, SceneAttachment)

	ENetWorld *ENetWorld::_instance = nullptr;

	ENetWorld* ENetWorld::GetInstance()
	{
		return _instance;
	}

	ENetWorld::ENetWorld() : _hosts(new Array())
	{
		RN_ASSERT(!_instance, "There already is an ENetWorld!");

		if(enet_initialize() != 0)
		{
			RNDebug("Failed initializing enet.");
			return;
		}

		_instance = this;
	}
		
	ENetWorld::~ENetWorld()
	{
		_hosts->Release();

		_instance = nullptr;
		enet_deinitialize();
	}

	void ENetWorld::Update(float delta)
	{
		_hosts->Enumerate<ENetHost>([&](ENetHost *host, size_t index, bool &stop) {
			host->Update(delta);
		});
	}

	void ENetWorld::AddHost(ENetHost *host)
	{
		_hosts->AddObject(host);
	}

	void ENetWorld::RemoveHost(ENetHost *host)
	{
		_hosts->RemoveObject(host);
	}
}
