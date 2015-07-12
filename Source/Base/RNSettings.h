//
//  RNSettings.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_SETTINGS_H_
#define __RAYNE_SETTINGS_H_

#include "RNBase.h"
#include "../Objects/RNString.h"
#include "../Objects/RNDictionary.h"

#include "../Objects/RNNumber.h"

#define kRNSettingsLoggersKey RNCSTR("RNLoggers")

namespace RN
{
	class Kernel;
	class Settings
	{
	public:
		friend class Kernel;

		RNAPI static Settings *GetSharedInstance();
		RNAPI static String *GetSettingsLocation();

		template<class T=Object>
		T *GetEntryForKey(String *key) const
		{
			std::unique_lock<std::mutex> lock(_lock);
			T *object = _settings->GetObjectForKey<T>(key);

			if(!object)
				return nullptr;

			object->Retain();
			lock.unlock();

			return object->Autorelease();
		}

		bool GetBoolForKey(String *key, bool defaultValue = false) const
		{
			Number *number = GetEntryForKey<Number>(key);
			return number ? number->GetBoolValue() : defaultValue;
		}
		float GetFloatForKey(String *key, float defaultValue = 0.0f) const
		{
			Number *number = GetEntryForKey<Number>(key);
			return number ? number->GetFloatValue() : defaultValue;
		}
		int32 GetInt32ForKey(String *key, int32 defaultValue = 0) const
		{
			Number *number = GetEntryForKey<Number>(key);
			return number ? number->GetInt32Value() : defaultValue;
		}
		uint32 GetUint32ForKey(String *key, uint32 defaultValue = 0) const
		{
			Number *number = GetEntryForKey<Number>(key);
			return number ? number->GetUint32Value() : defaultValue;
		}

		RNAPI void SetEntryForKey(Object *object, String *key);

		void SetBoolForKey(String *key, bool value)
		{
			SetEntryForKey(Number::WithBool(value), key);
		}
		void SetFloatForKey(String *key, float value)
		{
			SetEntryForKey(Number::WithFloat(value), key);
		}
		void SetInt32ForKey(String *key, int32 value)
		{
			SetEntryForKey(Number::WithInt32(value), key);
		}
		void SetUint32ForKey(String *key, uint32 value)
		{
			SetEntryForKey(Number::WithUint32(value), key);
		}

		RNAPI void RemoveEntryForKey(String *key);

		RNAPI void Sync();

	private:
		Settings();
		~Settings();

		mutable std::mutex _lock;
		bool _isDirty;
		Dictionary *_settings;
	};
}


#endif /* __RAYNE_SETTINGS_H_ */
