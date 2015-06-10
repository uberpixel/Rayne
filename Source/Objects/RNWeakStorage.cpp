//
//  RNWeakStorage.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWeakStorage.h"

namespace RN
{
	RNDefineMeta(WeakStorage, Object)

	WeakStorage::WeakStorage()
	{}

	WeakStorage::~WeakStorage()
	{}


	void WeakStorage::AddObject(Object *object)
	{
		_storage.emplace_back(object);
	}
	void WeakStorage::RemoveObject(Object *object)
	{
		CleanZombies();

		std::remove_if(_storage.begin(), _storage.end(), [](Entry &entry) -> bool {

			Object *temp = entry.object.Load();
			return (temp && temp->IsEqual(object));

		});
	}
	void WeakStorage::RemoveObjectIdenticalTo(Object *object)
	{
		CleanZombies();

		std::remove_if(_storage.begin(), _storage.end(), [](Entry &entry) -> bool {

			Object *temp = entry.object.Load();
			return (temp == object)

		});
	}
	void WeakStorage::RemoveAllObjects()
	{
		_storage.clear();
	}

	Array *WeakStorage::GetAllObjects() const
	{
		CleanZombies();

		Array *objects = new Array();

		for(Entry &entry : _storage)
		{
			Object *object = entry.object.Load();
			if(!object)
				continue;

			objects->AddObject(object);
		}

		return objects->Autorelease();
	}





	void WeakStorage::CleanZombies() const
	{
		std::remove_if(_storage.begin(), _storage.end(), [](Entry &entry) -> bool {

			Object *object = entry.object.Load();
			return (object != nullptr);

		});
	}
}
