//
//  RNSettings.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../System/RNFileManager.h"
#include "../Objects/RNJSONSerialization.h"
#include "../Debug/RNLogger.h"
#include "RNSettings.h"

namespace RN
{
	Settings *__sharedInstance = nullptr;

	Settings::Settings() :
		_isDirty(false),
		_settings(nullptr)
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
					Data *orgData = Data::WithContentsOfFile(original);
					if(orgData)
					{
						try
						{
							Dictionary *orgSettings = JSONSerialization::ObjectFromData<Dictionary>(orgData, 0);

							orgSettings->Enumerate<Object, String>([&](Object *object, const String *key, bool &stop) {

								if(!_settings->GetObjectForKey(key))
								{
									_settings->SetObjectForKey(object, key);
									_isDirty = true;
								}

							});
						}
						catch(Exception &)
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
		//data->WriteToFile(location);

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
		String *location = FileManager::GetSharedInstance()->GetPathForLocation(FileManager::Location::InternalSaveDirectory);
		location->AppendPathComponent(RNCSTR("settings.json"));

		return location;
	}

	void Settings::SetEntryForKey(Object *object, const String *tkey)
	{
		if(!JSONSerialization::IsValidJSONObject(object))
			throw InvalidArgumentException("Object must be serializable to JSON!");

		object = object->Copy();
		String *key = tkey->Copy();

		LockGuard<Lockable> lock(_lock);

		_settings->SetObjectForKey(object->Autorelease(), key->Autorelease());
		_isDirty = true;
	}

	void Settings::RemoveEntryForKey(const String *key)
	{
		LockGuard<Lockable> lock(_lock);
		_settings->RemoveObjectForKey(key);
		_isDirty = true;
	}

	void Settings::Sync()
	{
		LockGuard<Lockable> lock(_lock);
		if(!_isDirty)
			return;

		try
		{
			Data *data = JSONSerialization::JSONDataFromObject(_settings, JSONSerialization::Options::PrettyPrint);
			data->WriteToFile(GetSettingsLocation());
			_isDirty = false;
		}
		catch(Exception &e)
		{
			RNWarning("Encountered the following exception while trying to sync the settings.json: " << e);
		}
	}

	Object *Settings::__RetrieveObjectForLiteralKey(const String *key) const
	{
		return _settings->GetObjectForKey(key);
	}

	Object *Settings::__GetObjectForKey(const String *key) const
	{
		Object *result;

#if RN_PLATFORM_MAC_OS
		if((result = _settings->GetObjectForKey(key->StringByAppendingString(RNCSTR("~macos")))))
			return result;
#endif
#if RN_PLATFORM_WINDOWS
		if((result = _settings->GetObjectForKey(key->StringByAppendingString(RNCSTR("~windows")))))
			return result;
#endif
#if RN_PLATFORM_LINUX
		if((result = _settings->GetObjectForKey(key->StringByAppendingString(RNCSTR("~linux")))))
			return result;
#endif
#if RN_PLATFORM_ANDROID
		if((result = _settings->GetObjectForKey(key->StringByAppendingString(RNCSTR("~android")))))
			return result;
#endif

		return _settings->GetObjectForKey(key);
	}
}
