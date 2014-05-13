//
//  RNSettings.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SETTINGS_H__
#define __RAYNE_SETTINGS_H__

#include "RNBase.h"
#include "RNSpinLock.h"
#include "RNDictionary.h"
#include "RNString.h"
#include "RNNumber.h"
#include "RNArray.h"

#define kRNSettingsGammaCorrectionKey RNCSTR("RNGammaCorrection")
#define kRNSettingsScreenKey          RNCSTR("RNScreen")
#define kRNSettingsRendererKey        RNCSTR("RNOpenGLRenderer")
#define kRNSettingsDebugContextKey    RNCSTR("RNDebugContext")

#define kRNManifestApplicationKey  RNCSTR("RNApplication")
#define KRNManifestModulesKey      RNCSTR("RNModules")
#define kRNManifestUIStyleKey      RNCSTR("RNUIStyle")

namespace RN
{
	class Kernel;
	class Settings : public ISingleton<Settings>
	{
	public:
		friend class Kernel;
		
		RNAPI Settings();
		RNAPI ~Settings() override;
		
		template<class T=Object>
		T *GetObjectForKey(String *key)
		{
			_lock.Lock();
			T *object = _settings->GetObjectForKey<T>(key);
			
			if(object)
				object->Retain()->Autorelease();
			
			_lock.Unlock();
			
			return object;
		}
		
		bool GetBoolForKey(String *key, bool defaultValue = false)
		{
			Number *number = GetObjectForKey<Number>(key);
			return number ? number->GetBoolValue() : defaultValue;
		}
		float GetFloatForKey(String *key, float defaultValue = 0.0f)
		{
			Number *number = GetObjectForKey<Number>(key);
			return number ? number->GetFloatValue() : defaultValue;
		}
		int32 GetInt32ForKey(String *key, int32 defaultValue = 0)
		{
			Number *number = GetObjectForKey<Number>(key);
			return number ? number->GetInt32Value() : defaultValue;
		}
		uint32 GetUint32ForKey(String *key, uint32 defaultValue = 0)
		{
			Number *number = GetObjectForKey<Number>(key);
			return number ? number->GetUint32Value() : defaultValue;
		}
		
		void SetBoolForKey(String *key, bool value)
		{
			SetObjectForKey(Number::WithBool(value), key);
		}
		void SetFloatForKey(String *key, float value)
		{
			SetObjectForKey(Number::WithFloat(value), key);
		}
		void SetInt32ForKey(String *key, int32 value)
		{
			SetObjectForKey(Number::WithInt32(value), key);
		}
		void SetUint32ForKey(String *key, uint32 value)
		{
			SetObjectForKey(Number::WithUint32(value), key);
		}
		
		template<class T=Object>
		T *GetManifestObjectForKey(String *key)
		{
			T *object = _manifest->GetObjectForKey<T>(key);
			return object;
		}
		
		RNAPI void SetObjectForKey(Object *object, String *key);
		RNAPI void RemoveObjectForKey(String *key);
		
	private:
		void LoadManifest();
		void LoadSettings();
		
		void Sync(bool force);
		std::string SettingsLocation() const;
		
		bool _mutated;
		SpinLock _lock;
		
		Dictionary *_settings;
		Dictionary *_manifest;
		
		RNDeclareSingleton(Settings)
	};
}

#endif /* __RAYNE_SETTINGS_H__ */
