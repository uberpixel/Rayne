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
#include "RNDictionary.h"
#include "RNString.h"
#include "RNTexture.h"
#include "RNUIGeometry.h"
#include "RNUIControl.h"
#include "RNUIFont.h"

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
			
			Style();
			~Style() override;
			
			Texture *GetTextureWithName(String *name);
			
			Font *GetFont(FontStyle style);
			Dictionary *GetButtonStyle(String *name);
			Dictionary *GetTextfieldStyle(String *name);
			
			static EdgeInsets ParseEdgeInsets(Dictionary *insets);
			static Atlas ParseAtlas(Dictionary *atlas);
			static Control::State ParseState(String *string);
			
		private:
			Font *CreateFontFromDictionary(Dictionary *info);
			
			SpinLock _lock;
			Dictionary *_data;
			Dictionary *_textures;
			Dictionary *_fonts;
		};
	}
}

#endif /* __RAYNE_UISTYLE_H__ */
