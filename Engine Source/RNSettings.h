//
//  RNSettings.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SETTINGS_H__
#define __RAYNE_SETTINGS_H__

#include "RNBase.h"

namespace RN
{
	class Settings : public Singleton<Settings>
	{
	public:
		Settings();
		~Settings();
		
		bool GammaCorrection() const { return _gammaCorrection; }
		const std::string& GameModule() const { return _gameModule; }
		
	private:
		bool _gammaCorrection;
		std::string _gameModule;
	};
}

#endif /* __RAYNE_SETTINGS_H__ */
