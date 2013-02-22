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
		
		_gammaCorrection = (gammaCorrection && json_is_boolean(gammaCorrection)) ? (gammaCorrection == json_true()) : true;
		_gameModule = (gameModule && json_is_string(gameModule)) ? json_string_value(gameModule) : "";
		
		json_decref(root);
	}
	
	Settings::~Settings()
	{
	}
}
