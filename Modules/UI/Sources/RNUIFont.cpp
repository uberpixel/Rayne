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
	
		static SkFontStyle::Weight SkWeightForWeight(Font::Weight weight)
		{
			switch(weight)
			{
				case Font::Weight::InvisibleWeight:
					return SkFontStyle::Weight::kInvisible_Weight;
				case Font::Weight::ThinWeight:
					return SkFontStyle::Weight::kThin_Weight;
				case Font::Weight::ExtraLightWeight:
					return SkFontStyle::Weight::kExtraLight_Weight;
				case Font::Weight::LightWeight:
					return SkFontStyle::Weight::kLight_Weight;
				case Font::Weight::NormalWeight:
					return SkFontStyle::Weight::kNormal_Weight;
				case Font::Weight::MediumWeight:
					return SkFontStyle::Weight::kMedium_Weight;
				case Font::Weight::SemiBoldWeight:
					return SkFontStyle::Weight::kSemiBold_Weight;
				case Font::Weight::BoldWeight:
					return SkFontStyle::Weight::kBold_Weight;
				case Font::Weight::ExtraBoldWeight:
					return SkFontStyle::Weight::kExtraBold_Weight;
				case Font::Weight::BlackWeight:
					return SkFontStyle::Weight::kBlack_Weight;
				case Font::Weight::ExtraBlackWeight:
					return SkFontStyle::Weight::kExtraBlack_Weight;
			}
		}
	
		static SkFontStyle::Width SkWidthForWidth(Font::Width width)
		{
			switch(width)
			{
				case Font::Width::UltraCondensedWidth:
					return SkFontStyle::Width::kUltraCondensed_Width;
				case Font::Width::ExtraCondensedWidth:
					return SkFontStyle::Width::kExtraCondensed_Width;
				case Font::Width::CondensedWidth:
					return SkFontStyle::Width::kCondensed_Width;
				case Font::Width::SemiCondensedWidth:
					return SkFontStyle::Width::kSemiCondensed_Width;
				case Font::Width::NormalWidth:
					return SkFontStyle::Width::kNormal_Width;
				case Font::Width::SemiExpandedWidth:
					return SkFontStyle::Width::kSemiExpanded_Width;
				case Font::Width::ExpandedWidth:
					return SkFontStyle::Width::kExpanded_Width;
				case Font::Width::ExtraExpandedWidth:
					return SkFontStyle::Width::kExtraExpanded_Width;
				case Font::Width::UltraExpandedWidth:
					return SkFontStyle::Width::kUltraExpanded_Width;
			}
		}
	
		static SkFontStyle::Slant SkSlantForSlant(Font::Slant slant)
		{
			switch(slant)
			{
				case Font::Slant::UprightSlant:
					return SkFontStyle::Slant::kUpright_Slant;
				case Font::Slant::ItalicSlant:
					return SkFontStyle::Slant::kItalic_Slant;
				case Font::Slant::ObliqueSlant:
					return SkFontStyle::Slant::kOblique_Slant;
			}
		}
		
		Font *Font::WithFamilyName(const String *familyName, float size, Weight weight, Width width, Slant slant)
		{
			Font *font = new Font();
			
/*			sk_sp<SkFontMgr> mgr(SkFontMgr::RefDefault());
			
			for(int i = 0; i < mgr->countFamilies(); i++)
			{
				SkString familyName;
				mgr->getFamilyName(i, &familyName);
				RNDebug(familyName.c_str());
			}*/
			
			sk_sp<SkTypeface> typeface = SkTypeface::MakeFromName(familyName->GetUTF8String(), SkFontStyle(SkWeightForWeight(weight), SkWidthForWidth(width), SkSlantForSlant(slant)));
			font->_internals->font.setTypeface(typeface);
			font->_internals->font.setSize(size);
			
			return font->Autorelease();
		}

		float Font::GetSize() const
		{
			return _internals->font.getSize();
		}
	}
}
