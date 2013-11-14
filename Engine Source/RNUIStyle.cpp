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
				HandleException(e);
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
		
		// ---------------------
		// MARK: -
		// MARK: Fonts
		// ---------------------
		
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
					break;
					
				case FontStyle::DefaultFontBoldItalics:
					identifier = RNCSTR("RNDefaultFontBoldItalics");
					break;
			}
			
			return GetFontWithIdentifier(identifier);
		}
		
		Font *Style::GetFontWithIdentifier(String *identifier)
		{
			RN_ASSERT(identifier, "Identifier mustn't be NULL");
			
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
		
		// ---------------------
		// MARK: -
		// MARK: Color
		// ---------------------
		
		Color *Style::GetColor(ColorStyle style)
		{
			String *identifier = nullptr;
			
			switch(style)
			{
				case ColorStyle::BackgroundColor:
					identifier = RNCSTR("RNBackgroundColor");
					break;
					
				case ColorStyle::TextColor:
					identifier = RNCSTR("RNTextColor");
					break;
					
				case ColorStyle::SelectionColor:
					identifier = RNCSTR("RNSelectionColor");
					break;
			}
			
			return GetColorWithIdentifier(identifier);
		}
		
		Color *Style::GetColorWithIdentifier(String *identifier)
		{
			RN_ASSERT(identifier, "Identifier mustn't be NULL");
			
			LockGuard<SpinLock> lock(_lock);
			
			Dictionary *colorDict = _data->GetObjectForKey<Dictionary>(RNCSTR("colors"));
			Array *color = colorDict->GetObjectForKey<Array>(identifier);
			
			return ParseColor(color);
		}
		
		// ---------------------
		// MARK: -
		// MARK: Textures
		// ---------------------
		
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
		
		// ---------------------
		// MARK: -
		// MARK: Buttons
		// ---------------------
		
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
		
		// ---------------------
		// MARK: -
		// MARK: Textfields
		// ---------------------
		
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
		
		// ---------------------
		// MARK: -
		// MARK: Parsing
		// ---------------------
		
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
		
		Color *Style::ParseColor(Array *color)
		{
			float r = 1.0f;
			float g = 1.0f;
			float b = 1.0f;
			float a = 1.0f;
			
			if(color)
			{
				RN_ASSERT(color->GetCount() == 3 || color->GetCount() == 4, "Array must have three or four components!");
				try
				{
					r = color->GetObjectAtIndex<Number>(0)->GetFloatValue();
					g = color->GetObjectAtIndex<Number>(1)->GetFloatValue();
					b = color->GetObjectAtIndex<Number>(2)->GetFloatValue();
					
					if(color->GetCount() == 4)
						a = color->GetObjectAtIndex<Number>(3)->GetFloatValue();
				}
				catch(Exception e)
				{}
			}
			
			return Color::WithRNColor(RN::Color(r, g, b, a));
		}
	}
}
