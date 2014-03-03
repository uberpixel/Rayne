//
//  RNUIStyle.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIStyle.h"
#include "RNSettings.h"
#include "RNLogging.h"
#include "RNAutoreleasePool.h"
#include "RNJSONSerialization.h"

namespace RN
{
	namespace UI
	{
		RNDefineSingleton(Style)
		
		Style::Style()
		{
			try
			{
				String *path = Settings::GetSharedInstance()->GetManifestObjectForKey<String>(kRNManifestUIStyleKey);
				
				_data = JSONSerialization::JSONObjectFromData<Dictionary>(Data::WithContentsOfFile(path->GetUTF8String()));
				_data->Retain();
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
		// MARK: General
		// ---------------------
		
		Object *Style::__GetResourceWithKeyPath(String *keyPath)
		{
			RN_ASSERT(keyPath, "Key path mustn't be NULL");
			
			Array *keys = keyPath->GetComponentsSeparatedByString(RNCSTR("."));
			Object *resource = _data;
			
			size_t count = keys->GetCount();
			
			keys->Enumerate<String>([&](String *key, size_t index, bool &stop) {
				
				String *tkey = key->Copy();
				Object *temp = resource;
				
#if RN_PLATFORM_MAC_OS
				tkey->Append("~mac");
				resource = static_cast<Dictionary *>(resource)->GetObjectForKey(tkey);
#endif
#if RN_PLATFORM_WINDOWS
				tkey->Append("~win");
				resource = static_cast<Dictionary *>(resource)->GetObjectForKey(tkey);
#endif
#if RN_PLATFORM_LINUX
				tkey->Append("~linux");
				resource = static_cast<Dictionary *>(resource)->GetObjectForKey(tkey);
#endif
				
				tkey->Release();
				
				if(resource && (resource->IsKindOfClass(Dictionary::MetaClass()) || index == (count - 1)))
				   return;
				
				resource = temp; // Restore the resource
				resource = static_cast<Dictionary *>(resource)->GetObjectForKey(key);
				if(!resource)
				{
					stop = true;
					return;
				}
				
				if(!resource->IsKindOfClass(Dictionary::MetaClass()) && index < (count - 1))
				{
					stop = true;
					resource = nullptr;
					
					return;
				}
				
			});
			
			if(!resource)
				throw Exception(Exception::Type::InconsistencyException, std::string("Couldn't find resource for key path ") + keyPath->GetUTF8String());
				
			return resource;
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
					identifier = RNCSTR("fonts.RNDefaultFont");
					break;
					
				case FontStyle::DefaultFontBold:
					identifier = RNCSTR("fonts.RNDefaultFontBold");
					break;
					
				case FontStyle::DefaultFontItalics:
					identifier = RNCSTR("fonts.RNDefaultFontItalics");
					break;
					
				case FontStyle::DefaultFontBoldItalics:
					identifier = RNCSTR("fonts.RNDefaultFontBoldItalics");
					break;
			}
			
			return GetFontWithKeyPath(identifier);
		}
		
		Font *Style::GetFontWithKeyPath(String *identifier)
		{
			LockGuard<SpinLock> lock(_lock);
			
			Font *font = _fonts->GetObjectForKey<Font>(identifier);
			if(!font)
			{
				font = CreateFontFromDictionary(GetResourceWithKeyPath<Dictionary>(identifier));
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
					identifier = RNCSTR("colors.RNBackgroundColor");
					break;
					
				case ColorStyle::TextColor:
					identifier = RNCSTR("colors.RNTextColor");
					break;
					
				case ColorStyle::TitleColor:
					identifier = RNCSTR("colors.RNTitleColor");
					break;
					
				case ColorStyle::SelectionColor:
					identifier = RNCSTR("colors.RNSelectionColor");
					break;
			}
			
			return GetColorWithKeyPath(identifier);
		}
		
		Color *Style::GetColorWithKeyPath(String *keyPath)
		{
			LockGuard<SpinLock> lock(_lock);
			
			Array *color = GetResourceWithKeyPath<Array>(keyPath);
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
				Texture::Parameter parameter;
				parameter.maxMipMaps = 0;
				
				texture = Texture::WithFile(name->GetUTF8String(), parameter);
				_textures->SetObjectForKey(texture, name);
			}
			
			return texture;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Buttons
		// ---------------------
		
		Dictionary *Style::GetButtonStyleWithKeyPath(String *keyPath)
		{
			Dictionary *style = GetResourceWithKeyPath<Dictionary>(keyPath);
			return style;
		}
		
		Dictionary *Style::GetTextfieldStyleWithKeyPath(String *keyPath)
		{
			Dictionary *style = GetResourceWithKeyPath<Dictionary>(keyPath);
			return style;
		}
		
		Dictionary *Style::GetWindowStyleWithKeyPath(String *keyPath)
		{
			Dictionary *style = GetResourceWithKeyPath<Dictionary>(keyPath);
			return style;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Parsing
		// ---------------------
		
		Control::State Style::ParseState(String *string)
		{
			Control::State state = Control::Normal;
			
			if(string->GetRangeOfString(RNCSTR("selected")).origin != kRNNotFound)
				state |= Control::Selected;
			
			if(string->GetRangeOfString(RNCSTR("highlighted")).origin != kRNNotFound)
				state |= Control::Highlighted;
			
			if(string->GetRangeOfString(RNCSTR("disabled")).origin != kRNNotFound)
				state |= Control::Disabled;
			
			return state;
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
						return EdgeInsets(top->GetFloatValue(), bottom->GetFloatValue(), left->GetFloatValue(), right->GetFloatValue());
				}
				catch(Exception e)
				{
					RNWarning("Failed to parse edge insets dictionary: %s", e.GetReason().c_str());
				}
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
						return Atlas(x->GetFloatValue(), y->GetFloatValue(), width->GetFloatValue(), height->GetFloatValue());
				}
				catch(Exception e)
				{
					RNWarning("Failed to parse atlas dictionary: %s", e.GetReason().c_str());
				}
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
				try
				{
					RN_ASSERT(color->GetCount() == 3 || color->GetCount() == 4, "Array must have three or four components!");
					
					r = color->GetObjectAtIndex<Number>(0)->GetFloatValue();
					g = color->GetObjectAtIndex<Number>(1)->GetFloatValue();
					b = color->GetObjectAtIndex<Number>(2)->GetFloatValue();
					
					if(color->GetCount() == 4)
						a = color->GetObjectAtIndex<Number>(3)->GetFloatValue();
				}
				catch(Exception e)
				{
					RNWarning("Failed parse color array: %s", e.GetReason().c_str());
				}
			}
			
			return Color::WithRNColor(RN::Color(r, g, b, a));
		}
		
		Image *Style::ParseImage(Dictionary *style)
		{
			String *texture = style->GetObjectForKey<String>(RNCSTR("texture"));
			Dictionary *atlas  = style->GetObjectForKey<Dictionary>(RNCSTR("atlas"));
			Dictionary *insets = style->GetObjectForKey<Dictionary>(RNCSTR("insets"));
			
			Image *image = new Image(GetSharedInstance()->GetTextureWithName(texture));
			image->SetEdgeInsets(ParseEdgeInsets(insets));
			image->SetAtlas(ParseAtlas(atlas), false);
			
			return image;
		}
	}
}
