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

#define kRNUIStyleButtonsKey   RNCSTR("buttons")
#define kRNUIStyleTextfieldKey RNCSTR("textfields")
#define kRNUIStyleNameKey      RNCSTR("name")

namespace RN
{
	namespace UI
	{
		Style::Style()
		{
			try
			{
				String *path = Settings::SharedInstance()->ObjectForKey<String>(kRNSettingsUIStyleKey);
				Object *data = JSONSerialization::JSONObjectFromData(Data::WithContentsOfFile(path->UTF8String()));
				
				_data = data->Downcast<Dictionary>()->Retain();
			}
			catch(Exception e)
			{
				__HandleException(e);
			}
			
			
			_textures = new Dictionary();
		}
		
		Style::~Style()
		{
			_data->Release();
			_textures->Release();
		}
		
		
		EdgeInsets Style::ParseEdgeInsets(Dictionary *insets)
		{
			if(insets)
			{
				Number *top    = insets->ObjectForKey<Number>(RNCSTR("top"));
				Number *bottom = insets->ObjectForKey<Number>(RNCSTR("bottom"));
				Number *left   = insets->ObjectForKey<Number>(RNCSTR("left"));
				Number *right  = insets->ObjectForKey<Number>(RNCSTR("right"));
				
				if(top && bottom && left && right)
				{
					return EdgeInsets(top->FloatValue(), bottom->FloatValue(), left->FloatValue(), right->FloatValue());
				}
			}
			
			return EdgeInsets();
		}
		
		Atlas Style::ParseAtlas(Dictionary *atlas)
		{
			if(atlas)
			{
				Number *x = atlas->ObjectForKey<Number>(RNCSTR("x"));
				Number *y = atlas->ObjectForKey<Number>(RNCSTR("y"));
				Number *width  = atlas->ObjectForKey<Number>(RNCSTR("width"));
				Number *height = atlas->ObjectForKey<Number>(RNCSTR("height"));
				
				if(x && y && width && height)
				{
					return Atlas(x->FloatValue(), y->FloatValue(), width->FloatValue(), height->FloatValue());
				}
			}
			
			return Atlas(0.0f, 0.0f, 1.0f, 1.0f);
		}
		
		
		
		Texture *Style::TextureWithName(String *name)
		{
			Texture *texture = _textures->ObjectForKey<Texture>(name);
			if(!texture)
			{
				TextureParameter parameter;
				parameter.generateMipMaps = false;
				parameter.mipMaps = 0;
				
				texture = new Texture(name->UTF8String(), parameter);
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
		
		Dictionary *Style::TextfieldStyle(String *name)
		{
			Array *textfields = _data->ObjectForKey<Array>(kRNUIStyleTextfieldKey);
			Dictionary *style = nullptr;
			
			textfields->Enumerate([&](Object *object, size_t index, bool *stop) {
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
		
		Control::State Style::ParseState(String *string)
		{
			if(string->IsEqual(RNCSTR("selected")))
				return Control::Selected;
			
			if(string->IsEqual(RNCSTR("highlighted")))
				return Control::Highlighted;
			
			if(string->IsEqual(RNCSTR("disabled")))
				return Control::Disabled;
			
			return Control::Normal;
		}
	}
}
