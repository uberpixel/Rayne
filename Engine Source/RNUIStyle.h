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
#include "RNUIImage.h"

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
			
			RNAPI Style();
			RNAPI ~Style() override;
			
			RNAPI Texture *GetTextureWithName(String *name);
			
			template<class T>
			T *GetResourceWithKeyPath(String *keyPath)
			{
				Object *resource = __GetResourceWithKeyPath(keyPath);
				
				if(!resource->IsKindOfClass(T::MetaClass()))
				   throw Exception(Exception::Type::InconsistencyException, "Failed to get resource for key path, unexpected object found");
				   
				return static_cast<T *>(resource);
			}
			
			RNAPI Font *GetFont(FontStyle style);
			RNAPI Font *GetFontWithKeyPath(String *identifier);
			
			RNAPI Color *GetColor(ColorStyle style);
			RNAPI Color *GetColorWithKeyPath(String *keyPath);
			
			RNAPI Dictionary *GetButtonStyleWithKeyPath(String *keyPath);
			RNAPI Dictionary *GetTextfieldStyleWithKeyPath(String *name);
			RNAPI Dictionary *GetWindowStyleWithKeyPath(String *name);
			
			RNAPI static EdgeInsets ParseEdgeInsets(Dictionary *insets);
			RNAPI static Atlas ParseAtlas(Dictionary *atlas);
			RNAPI static Control::State ParseState(String *string);
			RNAPI static Color *ParseColor(Array *color);
			RNAPI static Image *ParseImage(Dictionary *image);
			
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
