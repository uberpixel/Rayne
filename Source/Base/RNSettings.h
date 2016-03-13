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
		T *GetEntryForKey(const String *key) const
		{
			std::unique_lock<std::mutex> lock(_lock);
			Object *object = __GetObjectForKey(key);

			if(!object)
				return nullptr;

			object->Retain();
			lock.unlock();

			return object->Autorelease()->Downcast<T>();
		}

		bool GetBoolForKey(const String *key, bool defaultValue = false) const
		{
			Number *number = GetEntryForKey<Number>(key);
			return number ? number->GetBoolValue() : defaultValue;
		}
		float GetFloatForKey(const String *key, float defaultValue = 0.0f) const
		{
			Number *number = GetEntryForKey<Number>(key);
			return number ? number->GetFloatValue() : defaultValue;
		}
		int32 GetInt32ForKey(const String *key, int32 defaultValue = 0) const
		{
			Number *number = GetEntryForKey<Number>(key);
			return number ? number->GetInt32Value() : defaultValue;
		}
		uint32 GetUint32ForKey(const String *key, uint32 defaultValue = 0) const
		{
			Number *number = GetEntryForKey<Number>(key);
			return number ? number->GetUint32Value() : defaultValue;
		}

		RNAPI void SetEntryForKey(Object *object, const String *key);

		void SetBoolForKey(const String *key, bool value)
		{
			SetEntryForKey(Number::WithBool(value), key);
		}
		void SetFloatForKey(const String *key, float value)
		{
			SetEntryForKey(Number::WithFloat(value), key);
		}
		void SetInt32ForKey(const String *key, int32 value)
		{
			SetEntryForKey(Number::WithInt32(value), key);
		}
		void SetUint32ForKey(const String *key, uint32 value)
		{
			SetEntryForKey(Number::WithUint32(value), key);
		}

		RNAPI void RemoveEntryForKey(const String *key);

		RNAPI void Sync();

	private:
		Settings();
		~Settings();

		RNAPI Object *__GetObjectForKey(const String *key) const;
		RNAPI Object *__RetrieveObjectForLiteralKey(const String *key) const;

		mutable std::mutex _lock;
		bool _isDirty;
		Dictionary *_settings;
	};
}


#endif /* __RAYNE_SETTINGS_H_ */
