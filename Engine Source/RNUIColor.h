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
			RNAPI Color(const RN::Color& color);
			RNAPI Color(Color *other);
			
			RNAPI static Color *WithRNColor(const RN::Color& color);
			
			RNAPI bool IsEqual(Object *other) const override;
			
			RNAPI const RN::Color& GetRNColor() const { return _color; }
			
		private:
			RN::Color _color;
			
			RNDeclareMeta(Color);
		};
	}
}

#endif /* __RAYNE_UICOLOR_H__ */
