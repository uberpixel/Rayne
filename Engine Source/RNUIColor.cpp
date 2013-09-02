//
//  RNUIColor.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIColor.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(Color)
		
		
		Color::Color()
		{
		}
		
		Color::Color(const RN::Color& color)
		{
			_color = color;
		}
		
		Color::Color(Color *other)
		{
			_color = other->_color;
		}
		
		
		Color *Color::WithRNColor(const RN::Color& tcolor)
		{
			Color *color = new Color(tcolor);
			return color->Autorelease();
		}
		
		
		bool Color::IsEqual(Object *other) const
		{
			if(other->IsKindOfClass(Color::MetaClass()))
			{
				Color *otherColor = static_cast<Color *>(other);
				return (otherColor->_color == _color);
			}
			
			return false;
		}
	}
}
