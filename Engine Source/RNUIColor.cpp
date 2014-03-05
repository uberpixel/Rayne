//
//  RNUIColor.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIColor.h"
#include "RNSettings.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Color)
		
		
		Color::Color()
		{}
		
		Color::Color(const RN::Color& color)
		{
			_color = color;
			
			if(Settings::GetSharedInstance()->GetBoolForKey(kRNSettingsGammaCorrectionKey))
			{
				_color.r = powf(_color.r, 2.2f);
				_color.g = powf(_color.g, 2.2f);
				_color.b = powf(_color.b, 2.2f);
			}
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
