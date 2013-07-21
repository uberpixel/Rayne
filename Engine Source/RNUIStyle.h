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

namespace RN
{
	namespace UI
	{
		class Style : public Singleton<Style>
		{
		public:
			Style();
			~Style() override;
			
			Texture *TextureWithName(String *name);
			
			Dictionary *ButtonStyle(String *name);
			Dictionary *TextfieldStyle(String *name);
			
			static EdgeInsets ParseEdgeInsets(Dictionary *insets);
			static Atlas ParseAtlas(Dictionary *atlas);
			static Control::State ParseState(String *string);
			
		private:
			Dictionary *_data;
			Dictionary *_textures;
		};
	}
}

#endif /* __RAYNE_UISTYLE_H__ */
