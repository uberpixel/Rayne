//
//  RNUIStyle.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UISTYLE_H__
#define __RAYNE_UISTYLE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNArray.h"
#include "RNDictionary.h"
#include "RNString.h"
#include "RNTexture.h"
#include "RNUIGeometry.h"
#include "RNUIControl.h"
#include "RNUIFont.h"
#include "RNUIColor.h"

namespace RN
{
	namespace UI
	{
		class Style : public Singleton<Style>
		{
		public:
			enum class FontStyle
			{
				DefaultFont,
				DefaultFontBold,
				DefaultFontItalics,
				DefaultFontBoldItalics
			};
			
			enum class ColorStyle
			{
				BackgroundColor,
				TextColor,
				TitleColor,
				SelectionColor
			};
			
			Style();
			~Style() override;
			
			Texture *GetTextureWithName(String *name);
			
			template<class T>
			T *GetResourceWithKeyPath(String *keyPath)
			{
				Object *resource = __GetResourceWithKeyPath(keyPath);
				
				if(!resource->IsKindOfClass(T::MetaClass()))
				   throw Exception(Exception::Type::InconsistencyException, "Failed to get resource for key path, unexpected object found");
				   
				return static_cast<T *>(resource);
			}
			
			Font *GetFont(FontStyle style);
			Font *GetFontWithKeyPath(String *identifier);
			
			Color *GetColor(ColorStyle style);
			Color *GetColorWithKeyPath(String *keyPath);
			
			Dictionary *GetButtonStyleWithKeyPath(String *keyPath);
			Dictionary *GetTextfieldStyleWithKeyPath(String *name);
			Dictionary *GetWindowStyleWithKeyPath(String *name);
			
			static EdgeInsets ParseEdgeInsets(Dictionary *insets);
			static Atlas ParseAtlas(Dictionary *atlas);
			static Control::State ParseState(String *string);
			static Color *ParseColor(Array *color);
			
		private:
			Font *CreateFontFromDictionary(Dictionary *info);
			Object *__GetResourceWithKeyPath(String *keyPath);
			
			SpinLock _lock;
			Dictionary *_data;
			Dictionary *_textures;
			Dictionary *_fonts;
		};
	}
}

#endif /* __RAYNE_UISTYLE_H__ */
