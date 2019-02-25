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
		}
		
		Font *Font::WithFamilyName(const String *familyName, float size)
		{
			Font *font = new Font();
			
			sk_sp<SkFontMgr> mgr(SkFontMgr::RefDefault());
			sk_sp<SkTypeface> typeface = SkTypeface::MakeFromName(familyName->GetUTF8String(), SkFontStyle());
			font->_internals->font.setTypeface(typeface);
			font->_internals->font.setSize(size);
			//font->_internals->shaper = new SkShaper(font->_internals->typeface);
			
			return font->Autorelease();
		}

		float Font::GetSize() const
		{
			return _internals->font.getSize();
		}
	}
}
