//
//  RNSettings.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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

#define kRNSettingsGammaCorrectionKey RNSTR("RNGammaCorrection")
#define KRNSettingsModulesKey         RNSTR("RNModules")
#define kRNSettingsGameModuleKey      RNSTR("RNGameModule")

namespace RN
{
	class Settings : public Singleton<Settings>
	{
	public:
		RNAPI Settings();
		RNAPI ~Settings();
		
		template<class T=Object>
		T *ObjectForKey(String *key)
		{
			_lock.Lock();
			T *object = _settings->ObjectForKey<T>(key);
			if(object)
				object->Retain()->Autorelease();
			_lock.Unlock();
			
			return object;
		}
		
		bool BoolForKey(String *key)
		{
			Number *number = ObjectForKey<Number>(key);
			return number ? number->BoolValue() : false;
		}
		
		RNAPI void SetObjectForKey(Object *object, String *key);
		RNAPI void RemoveObjectForKey(String *key);
		
	private:
		void Flush();
		std::string SettingsLocation() const;
		
		bool _mutated;
		SpinLock _lock;
		
		Dictionary *_settings;
	};
}

#endif /* __RAYNE_SETTINGS_H__ */
