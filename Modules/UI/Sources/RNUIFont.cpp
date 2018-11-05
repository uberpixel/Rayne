//
//  RNUIFont.cpp
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIFont.h"
#include "RNUIInternals.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Font, Object)

		Font::Font()
		{
			
		}
		
		Font::~Font()
		{
			//delete _internals->shaper;
			_internals->typeface = nullptr;
		}
		
		Font *Font::WithFamilyName(const String *familyName, float size)
		{
			Font *font = new Font();
			
			font->_size = size;
			
			sk_sp<SkFontMgr> mgr(SkFontMgr::RefDefault());
			font->_internals->typeface = SkTypeface::MakeFromName(familyName->GetUTF8String(), SkFontStyle());
			//font->_internals->shaper = new SkShaper(font->_internals->typeface);
			
			return font->Autorelease();
		}
	}
}
