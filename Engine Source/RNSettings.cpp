//
//  RNSettings.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSettings.h"
#include "RNData.h"
#include "RNPathManager.h"
#include "RNFileManager.h"
#include "RNJSONSerialization.h"

namespace RN
{
	Settings::Settings()
	{
		try
		{
			std::string path = FileManager::GetSharedInstance()->GetFilePathWithName("settings.json");
			
			Data *data = Data::WithContentsOfFile(path);
			_settings = static_cast<Dictionary *>(JSONSerialization::JSONObjectFromData(data));
			_settings->Retain();
		}
		catch(Exception e)
		{
			HandleException(e);
		}
	}
	
	Settings::~Settings()
	{
		Flush();
		_settings->Release();
	}
	
	
	std::string Settings::SettingsLocation() const
	{
		return PathManager::Join(PathManager::SaveDirectory(), "settings.json");
	}
	
	void Settings::Flush()
	{
		_lock.Lock();
		
		if(_mutated)
		{
			if(!PathManager::CreatePath(PathManager::Basepath(SettingsLocation()), true))
			{
				_lock.Unlock();
				throw Exception(Exception::Type::InconsistencyException, "Failed to flush Settings down to disk!");
			}
			
			Data *data = JSONSerialization::JSONDataFromObject(_settings, JSONSerialization::PrettyPrint);
			data->WriteToFile(SettingsLocation());
		
			_mutated = false;
		}
		
		_lock.Unlock();
	}
	
	
	
	void Settings::SetObjectForKey(Object *object, String *key)
	{
		_lock.Lock();
		_settings->SetObjectForKey(object, key);
		_mutated = true;
		_lock.Unlock();
	}
	
	void Settings::RemoveObjectForKey(String *key)
	{
		_lock.Lock();
		_settings->RemoveObjectForKey(key);
		_mutated = true;
		_lock.Unlock();
	}
}
