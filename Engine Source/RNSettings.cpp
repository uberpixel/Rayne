//
//  RNSettings.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSettings.h"
#include "RNData.h"
#include "RNJSONSerialization.h"

namespace RN
{
	Settings::Settings()
	{
		try
		{
			Data *data = Data::WithContentsOfFile("settings.json");
			_settings = static_cast<Dictionary *>(JSONSerialization::JSONObjectFromData(data));
			_settings->Retain();
		}
		catch(ErrorException e)
		{
			__HandleException(e);
		}
	}
	
	Settings::~Settings()
	{
		_settings->Release();
	}
}
