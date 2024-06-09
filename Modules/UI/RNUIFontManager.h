//
//  RNUIFontManager.h
//  Rayne
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_UIFONTMANAGER_H_
#define __RAYNE_UIFONTMANAGER_H_

#include "RNUIConfig.h"

struct stbtt_fontinfo;

namespace RN
{
	namespace UI
	{
		class FontManager;

		class Font : public Object
		{
		friend FontManager;
		public:
			UIAPI ~Font();
			
			UIAPI Mesh *GetMeshForCharacter(int codepoint);
			
			UIAPI float GetOffsetForNextCharacter(int currentCodepoint, int nextCodepoint);
			UIAPI float GetHeight();
			UIAPI float GetAscent();
			UIAPI float GetDescent();
			UIAPI float GetLineOffset();
			UIAPI Texture *GetFontTexture();
			UIAPI bool IsSDFFont();
			
		private:
			Font(String *filepath, bool preloadASCII = true);
			
			void *_arFont;
			Texture *_fontTexture;
			Vector2 _textureResolution;
			std::map<size_t, size_t> _codePointToIndex;
			
			stbtt_fontinfo *_fontInfo;
			Data *_fontData;
			
			Dictionary *_meshes;
			
			RNDeclareMetaAPI(Font, UIAPI)
		};

		class FontManager : public Object
		{
		public:
			UIAPI static FontManager *GetSharedInstance();
			
			UIAPI FontManager();
			UIAPI ~FontManager();
			
			UIAPI Font *GetFontForFilepath(String *filepath, bool preloadASCII = true);
			
		private:
			Dictionary *_fonts;
			
			static FontManager *_sharedInstance;
			
			RNDeclareMetaAPI(FontManager, UIAPI)
		};
	}
}


#endif /* __RAYNE_UIFONTMANAGER_H_ */
