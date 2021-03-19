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

		class Font : public RN::Object
		{
		friend FontManager;
		public:
			UIAPI ~Font();
			
			UIAPI RN::Mesh *GetMeshForCharacter(int codepoint);
			
			UIAPI float GetOffsetForNextCharacter(int currentCodepoint, int nextCodepoint);
			UIAPI float GetHeight();
			UIAPI float GetAscent();
			UIAPI float GetDescent();
			UIAPI float GetLineOffset();
			
		private:
			Font(RN::String *filepath, bool preloadASCII = true);
			
			stbtt_fontinfo *_fontInfo;
			RN::Data *_fontData;
			
			RN::Dictionary *_meshes;
			
			RNDeclareMetaAPI(Font, UIAPI)
		};

		class FontManager : public RN::Object
		{
		public:
			UIAPI static FontManager *GetSharedInstance();
			
			UIAPI FontManager();
			UIAPI ~FontManager();
			
			UIAPI Font *GetFontForFilepath(RN::String *filepath, bool preloadASCII = true);
			
		private:
			RN::Dictionary *_fonts;
			
			static FontManager *_sharedInstance;
			
			RNDeclareMetaAPI(FontManager, UIAPI)
		};
	}
}


#endif /* __RAYNE_UIFONTMANAGER_H_ */
