//
//  RNUIColor.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UICOLOR_H__
#define __RAYNE_UICOLOR_H__

#include "RNBase.h"
#include "RNColor.h"
#include "RNObject.h"

namespace RN
{
	namespace UI
	{
		class Color : public Object
		{
		public:
			RNAPI Color();
			RNAPI Color(const RN::Color &color);
			RNAPI Color(Color *other);
			
			RNAPI static Color *WithRNColor(const RN::Color &color);
			RNAPI static Color *WithCorrectedRNColor(const RN::Color &color);
			RNAPI static Color *WithRGB(float r, float g, float b);
			RNAPI static Color *WithRGBA(float r, float g, float b, float a);
			RNAPI static Color *WithWhite(float white, float a);
			RNAPI static Color *WithHSV(float h, float s, float v, float a=1.0f);
			RNAPI static Color *WithHSV(const Vector4 &hsva);
			
			RNAPI static Color *RedColor();
			RNAPI static Color *GreenColor();
			RNAPI static Color *BlueColor();
			RNAPI static Color *YellowColor();
			RNAPI static Color *BlackColor();
			RNAPI static Color *WhiteColor();
			RNAPI static Color *GrayColor();
			RNAPI static Color *ClearColor();
			
			RNAPI bool IsEqual(Object *other) const override;
			
			RNAPI const RN::Color &GetRNColor() const { return _color; }
			RNAPI const RN::Color &GetUncorrectedRNColor() const { return _uncorrected; }
			
		private:
			Color(const RN::Color &color, std::nullptr_t null);
			
			RN::Color _uncorrected;
			RN::Color _color;
			
			RNDeclareMeta(Color);
		};
	}
}

#endif /* __RAYNE_UICOLOR_H__ */
