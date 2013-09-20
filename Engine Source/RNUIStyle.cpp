//
//  RNUIStyle.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIStyle.h"
#include "RNSettings.h"
#include "RNAutoreleasePool.h"
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
				String *path = Settings::GetSharedInstance()->GetObjectForKey<String>(kRNSettingsUIStyleKey);
				Object *data = JSONSerialization::JSONObjectFromData(Data::WithContentsOfFile(path->GetUTF8String()));
				
				_data = data->Downcast<Dictionary>()->Retain();
			}
			catch(Exception e)
			{
				__HandleException(e);
			}
			
			_textures = new Dictionary();
			_fonts = new Dictionary();
		}
		
		Style::~Style()
		{
			_data->Release();
			_textures->Release();
			_fonts->Release();
		}
		
		
		EdgeInsets Style::ParseEdgeInsets(Dictionary *insets)
		{
			if(insets)
			{
				try
				{
					Number *top    = insets->GetObjectForKey<Number>(RNCSTR("top"));
					Number *bottom = insets->GetObjectForKey<Number>(RNCSTR("bottom"));
					Number *left   = insets->GetObjectForKey<Number>(RNCSTR("left"));
					Number *right  = insets->GetObjectForKey<Number>(RNCSTR("right"));
					
					if(top && bottom && left && right)
					{
						return EdgeInsets(top->GetFloatValue(), bottom->GetFloatValue(), left->GetFloatValue(), right->GetFloatValue());
					}
				}
				catch(Exception e)
				{}
			}
			
			return EdgeInsets();
		}
		
		Atlas Style::ParseAtlas(Dictionary *atlas)
		{
			if(atlas)
			{
				try
				{
					Number *x = atlas->GetObjectForKey<Number>(RNCSTR("x"));
					Number *y = atlas->GetObjectForKey<Number>(RNCSTR("y"));
					Number *width  = atlas->GetObjectForKey<Number>(RNCSTR("width"));
					Number *height = atlas->GetObjectForKey<Number>(RNCSTR("height"));
					
					if(x && y && width && height)
					{
						return Atlas(x->GetFloatValue(), y->GetFloatValue(), width->GetFloatValue(), height->GetFloatValue());
					}
				}
				catch(Exception e)
				{}
			}
			
			return Atlas(0.0f, 0.0f, 1.0f, 1.0f);
		}
		
		Font *Style::CreateFontFromDictionary(Dictionary *info)
		{
			AutoreleasePool pool;
			
			String *name = info->GetObjectForKey<String>(RNCSTR("name"));
			Number *size = info->GetObjectForKey<Number>(RNCSTR("size"));
			String *traits = info->GetObjectForKey<String>(RNCSTR("traits"));
			
			RN_ASSERT(name, "malformed font dictionary!");
			
			FontDescriptor descriptor;
			
			if(traits)
			{
				if(traits->GetRangeOfString(RNCSTR("bold")).origin != k::NotFound)
					descriptor.style |= FontDescriptor::FontStyleBold;
				
				if(traits->GetRangeOfString(RNCSTR("italics")).origin != k::NotFound)
					descriptor.style |= FontDescriptor::FontStyleItalic;
			}
			
			float fontSize = size ? size->GetDoubleValue() : 12.0f;
			Font *font = new Font(name->GetUTF8String(), fontSize, descriptor);
			
			return font;
		}
		
		Font *Style::GetFont(FontStyle style)
		{
			String *identifier = nullptr;
			
			switch(style)
			{
				case FontStyle::DefaultFont:
					identifier = RNCSTR("RNDefaultFont");
					break;
					
				case FontStyle::DefaultFontBold:
					identifier = RNCSTR("RNDefaultFontBold");
					break;
					
				case FontStyle::DefaultFontItalics:
					identifier = RNCSTR("RNDefaultFontItalics");
					
				case FontStyle::DefaultFontBoldItalics:
					identifier = RNCSTR("RNDefaultFontBoldItalics");
					break;
			}
			
			RN_ASSERT(identifier, "");
			
			LockGuard<SpinLock> lock(_lock);
			
			Font *font = _fonts->GetObjectForKey<Font>(identifier);
			if(!font)
			{
				Dictionary *fontDict = _data->GetObjectForKey<Dictionary>(RNCSTR("fonts"));
				font = CreateFontFromDictionary(fontDict->GetObjectForKey<Dictionary>(identifier));
				
				_fonts->SetObjectForKey(font->Autorelease(), identifier);
			}
			
			return font;
		}
		
		
		
		
		
		Texture *Style::GetTextureWithName(String *name)
		{
			Texture *texture = _textures->GetObjectForKey<Texture>(name);
			if(!texture)
			{
				TextureParameter parameter;
				parameter.generateMipMaps = false;
				parameter.mipMaps = 0;
				
				texture = new Texture(name->GetUTF8String(), parameter);
				_textures->SetObjectForKey(texture->Autorelease(), name);
			}
			
			return texture;
		}
		
		
		
		Dictionary *Style::GetButtonStyle(String *name)
		{
			Array *buttons = _data->GetObjectForKey<Array>(kRNUIStyleButtonsKey);
			Dictionary *style = nullptr;
			
			buttons->Enumerate([&](Object *object, size_t index, bool *stop) {
				Dictionary *dict = object->Downcast<Dictionary>();
				String *tname = dict->GetObjectForKey<String>(kRNUIStyleNameKey);
				
				if(tname->IsEqual(name))
				{
					style = dict;
					*stop = true;
				}
			});
			
			return style;
		}
		
		Dictionary *Style::GetTextfieldStyle(String *name)
		{
			Array *textfields = _data->GetObjectForKey<Array>(kRNUIStyleTextfieldKey);
			Dictionary *style = nullptr;
			
			textfields->Enumerate([&](Object *object, size_t index, bool *stop) {
				Dictionary *dict = object->Downcast<Dictionary>();
				String *tname = dict->GetObjectForKey<String>(kRNUIStyleNameKey);
				
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
