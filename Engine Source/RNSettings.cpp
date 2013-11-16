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
	Settings::Settings() :
		_manifest(nullptr),
		_settings(nullptr)
	{
		LoadManifest();
	}
	
	Settings::~Settings()
	{
		Sync(true);
		
		_settings->Release();
		_manifest->Release();
	}
	
	void Settings::LoadManifest()
	{
		try
		{
			std::string path = FileManager::GetSharedInstance()->GetFilePathWithName("manifest.json");
			
			Data *data = Data::WithContentsOfFile(path);
			_manifest = static_cast<Dictionary *>(JSONSerialization::JSONObjectFromData(data));
			_manifest->Retain();
		}
		catch(Exception e)
		{
			throw Exception(Exception::Type::InconsistencyException, "manifest.json not found!");
		}
		
#define ValidateManifest(Key, OType) \
		try { \
			if(!_manifest->GetObjectForKey<OType>(Key)) \
			{ throw Exception(Exception::Type::InconsistencyException, "manifest.json malformed!"); } \
		} \
		catch(Exception e) \
		{ throw Exception(Exception::Type::InconsistencyException, "manifest.json malformed!"); }
		
		
		ValidateManifest(kRNManifestApplicationKey, String)
		ValidateManifest(kRNManifestGameModuleKey, String)
	}
	
	void Settings::LoadSettings()
	{
		try
		{
			bool isVirgin = false;
			std::string path = std::move(SettingsLocation());
			
			if(!PathManager::PathExists(path))
			{
				path = std::move(FileManager::GetSharedInstance()->GetFilePathWithName("settings.json"));
				isVirgin = true;
			}
			
			Data *data = Data::WithContentsOfFile(path);
			_settings = static_cast<Dictionary *>(JSONSerialization::JSONObjectFromData(data));
			_settings->Retain();
			
			if(isVirgin)
				data->WriteToFile(SettingsLocation());
		}
		catch(Exception e)
		{
			_settings = new Dictionary();
		}
	}
	
	void Settings::Sync(bool force)
	{
		_lock.Lock();
		
		if(_mutated || force)
		{
			try
			{
				Data *data = JSONSerialization::JSONDataFromObject(_settings, JSONSerialization::PrettyPrint);
				data->WriteToFile(SettingsLocation());
			}
			catch(Exception e)
			{}
		
			_mutated = false;
		}
		
		_lock.Unlock();
	}
	
	
	
	std::string Settings::SettingsLocation() const
	{
		return PathManager::Join(PathManager::SaveDirectory(), "settings.json");
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
