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

namespace RN
{
	class Settings : public Singleton<Settings>
	{
	public:
		RNAPI Settings();
		RNAPI ~Settings();
		
		bool GammaCorrection() const { return _gammaCorrection; }
		const std::string& GameModule() const { return _gameModule; }
		const std::vector<std::string>& Modules() const { return _modules; }
		
	private:
		bool _gammaCorrection;
		std::string _gameModule;
		std::vector<std::string> _modules;
	};
}

#endif /* __RAYNE_SETTINGS_H__ */
