//
//  RNSettings.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../System/RNFileManager.h"
#include "../Objects/RNJSONSerialization.h"
#include "RNSettings.h"

namespace RN
{
	Settings *__sharedInstance = nullptr;

	Settings::Settings() :
		_settings(nullptr),
		_isDirty(false)
	{
		FileManager *manager = FileManager::GetSharedInstance();

		String *location = GetSettingsLocation();
		String *original = manager->ResolveFullPath(RNCSTR("settings.json"), 0);

		if(manager->PathExists(location))
		{
			try
			{
				Data *data = Data::WithContentsOfFile(location);
				_settings = SafeRetain(JSONSerialization::ObjectFromData<Dictionary>(data, 0));

				if(_settings)
				{
					__sharedInstance = this;
					return;
				}
			}
			catch(Exception &e)
			{}
		}


		if(!original)
		{
			_settings = new Dictionary();
			__sharedInstance = this;
			return;
		}

		Data *data = Data::WithContentsOfFile(original);
		data->WriteToFile(location);

		_settings = SafeRetain(JSONSerialization::ObjectFromData<Dictionary>(data, 0));
		__sharedInstance = this;
	}

	Settings::~Settings()
	{
		Sync();
		SafeRelease(_settings);

		__sharedInstance = nullptr;
	}

	Settings *Settings::GetSharedInstance()
	{
		return __sharedInstance;
	}

	String *Settings::GetSettingsLocation()
	{
		String *location = FileManager::GetSharedInstance()->GetPathForLocation(FileManager::Location::SaveDirectory);
		location->AppendPathComponent(RNCSTR("settings.json"));

		return location;
	}

	void Settings::SetEntryForKey(Object *object, String *key)
	{
		if(!JSONSerialization::IsValidJSONObject(object))
			throw InvalidArgumentException("Object must be serializable to JSON!");

		object = object->Copy();
		key = key->Copy();

		std::lock_guard<std::mutex> lock(_lock);

		_settings->SetObjectForKey(object->Autorelease(), key->Autorelease());
		_isDirty = true;
	}

	void Settings::RemoveEntryForKey(String *key)
	{
		std::lock_guard<std::mutex> lock(_lock);
		_settings->RemoveObjectForKey(key);
		_isDirty = true;
	}

	void Settings::Sync()
	{
		std::lock_guard<std::mutex> lock(_lock);
		if(!_isDirty)
			return;

		try
		{
			Data *data = JSONSerialization::JSONDataFromObject(_settings, JSONSerialization::Options::PrettyPrint);
			data->WriteToFile(GetSettingsLocation());

			_isDirty = false;
		}
		catch(Exception &e)
		{}
	}
}
