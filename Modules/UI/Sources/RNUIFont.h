//
//  RNUIFont.h
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UIFONT_H_
#define __RAYNE_UIFONT_H_

#include "RNUIConfig.h"

namespace RN
{
	namespace UI
	{
		struct FontInternals;
		
		class Font : public Object
		{
		public:
			friend class Context;
			friend class Label;
			
			enum Weight
			{
				InvisibleWeight,
				ThinWeight,
				ExtraLightWeight,
				LightWeight,
				NormalWeight,
				MediumWeight,
				SemiBoldWeight,
				BoldWeight,
				ExtraBoldWeight,
				BlackWeight,
				ExtraBlackWeight
			};
			
			enum Width
			{
				UltraCondensedWidth,
				ExtraCondensedWidth,
				CondensedWidth,
				SemiCondensedWidth,
				NormalWidth,
				SemiExpandedWidth,
				ExpandedWidth,
				ExtraExpandedWidth,
				UltraExpandedWidth
			};
			
			enum Slant
			{
				UprightSlant,
				ItalicSlant,
				ObliqueSlant
			};
			
			UIAPI ~Font();
			float GetSize() const;
			
			UIAPI static Font *WithFamilyName(const String *familyName, float size, Weight weight = Weight::NormalWeight, Width width = Width::NormalWidth, Slant slant = Slant::UprightSlant);
			
		private:
			UIAPI Font();
			
			PIMPL<FontInternals> _internals;
			
			RNDeclareMetaAPI(Font, UIAPI)
		};
	}
}

#endif /* __RAYNE_UIFONT_H_ */
