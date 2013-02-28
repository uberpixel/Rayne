//
//  RNSettings.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSettings.h"
#include "RNFile.h"
#include <jansson.h>

namespace RN
{
	Settings::Settings()
	{
		File *file = (new File("game/settings.json"))->Autorelease<File>();
		
		json_error_t error;
		json_t *root = json_loadf(file->FilePointer(), 0, &error);
		
		RN_ASSERT(json_is_object(root), "The root settings must be object type!");
		
		json_t *gammaCorrection = json_object_get(root, "RNGammaCorrection");
		json_t *gameModule = json_object_get(root, "RNGameModule");
		json_t *modules = json_object_get(root, "RNModules");
		
		_gammaCorrection = (gammaCorrection && json_is_boolean(gammaCorrection)) ? (gammaCorrection == json_true()) : true;
		_gameModule = (gameModule && json_is_string(gameModule)) ? json_string_value(gameModule) : "";
		
		if(modules && json_is_array(modules))
		{
			for(size_t i=0; i<json_array_size(modules); i++)
			{
				json_t *module = json_array_get(modules, i);
				if(json_is_string(module))
				{
					std::string name = json_string_value(module);
					_modules.push_back(name);
				}
			}
		}
		
		json_decref(root);
	}
	
	Settings::~Settings()
	{
	}
}
