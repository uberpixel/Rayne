//
//  RNUIStyle.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIStyle.h"
#include "RNSettings.h"
#include "RNJSONSerialization.h"

#define kRNUIStyleButtonsKey RNCSTR("buttons")
#define kRNUIStyleNameKey    RNCSTR("name")

namespace RN
{
	namespace UI
	{
		Style::Style()
		{
			String *path = Settings::SharedInstance()->ObjectForKey<String>(kRNSettingsUIStyleKey);
			Object *data = JSONSerialization::JSONObjectFromData(Data::WithContentsOfFile(path->UTF8String()));
			
			_data = data->Downcast<Dictionary>()->Retain();
			_textures = new Dictionary();
		}
		
		Style::~Style()
		{
			_data->Release();
			_textures->Release();
		}
		
		
		Texture *Style::TextureWithName(String *name)
		{
			Texture *texture = _textures->ObjectForKey<Texture>(name);
			if(!texture)
			{
				texture = new Texture(name->UTF8String());
				_textures->SetObjectForKey(texture->Autorelease(), name);
			}
			
			return texture;
		}
		
		Dictionary *Style::ButtonStyle(String *name)
		{
			Array *buttons = _data->ObjectForKey<Array>(kRNUIStyleButtonsKey);
			Dictionary *style = nullptr;
			
			buttons->Enumerate([&](Object *object, size_t index, bool *stop) {
				Dictionary *dict = object->Downcast<Dictionary>();
				String *tname = dict->ObjectForKey<String>(kRNUIStyleNameKey);
				
				if(tname->IsEqual(name))
				{
					style = dict;
					*stop = true;
				}
			});
			
			return style;
		}
	}
}
