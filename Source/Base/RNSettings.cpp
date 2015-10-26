//
//  RNSettings.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../System/RNFileCoordinator.h"
#include "../Objects/RNJSONSerialization.h"
#include "../Debug/RNLogger.h"
#include "RNSettings.h"

namespace RN
{
	Settings *__sharedInstance = nullptr;

	Settings::Settings() :
		_settings(nullptr),
		_isDirty(false)
	{
		FileCoordinator *manager = FileCoordinator::GetSharedInstance();

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
					Data *orgData = Data::WithContentsOfFile(original);
					if(orgData)
					{
						try
						{
							Dictionary *orgSettings = JSONSerialization::ObjectFromData<Dictionary>(orgData, 0);

							orgSettings->Enumerate<Object, String>([&](Object *object, String *key, bool &stop) {

								if(!_settings->GetObjectForKey(key))
								{
									_settings->SetObjectForKey(object, key);
									_isDirty = true;
								}

							});
						}
						catch(Exception &e)
						{}
					}


					__sharedInstance = this;
					return;
				}
			}
			catch(Exception &e)
			{
				RNWarning("Found stored settings.json, but encountered the following exception while trying to read it: " << e);
			}
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
		String *location = FileCoordinator::GetSharedInstance()->GetPathForLocation(FileCoordinator::Location::SaveDirectory);
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
			bool saved = data->WriteToFile(GetSettingsLocation());

			if(saved)
			{
				_isDirty = false;
				return;
			}

			RNWarning("Failed to sync settings.json");
		}
		catch(Exception &e)
		{
			RNWarning("Encountered the following exception while trying to sync the settings.json: " << e);
		}
	}
}
