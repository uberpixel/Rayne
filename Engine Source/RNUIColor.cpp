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
		RNDefineMeta(Color, Object)
		
		
		Color::Color() :
			_color(RN::Color::Black()),
			_uncorrected(RN::Color::Black())
		{}
		
		Color::Color(const RN::Color &color) :
			_color(color),
			_uncorrected(color)
		{
			if(Settings::GetSharedInstance()->GetBoolForKey(kRNSettingsGammaCorrectionKey))
			{
				_color.r = powf(_color.r, 2.2f);
				_color.g = powf(_color.g, 2.2f);
				_color.b = powf(_color.b, 2.2f);
			}
		}
		
		Color::Color(const RN::Color &color, std::nullptr_t null) :
			_uncorrected(color),
			_color(color)
		{
			if(Settings::GetSharedInstance()->GetBoolForKey(kRNSettingsGammaCorrectionKey))
			{
				_uncorrected.r = powf(_uncorrected.r, 0.454545455f);
				_uncorrected.g = powf(_uncorrected.g, 0.454545455f);
				_uncorrected.b = powf(_uncorrected.b, 0.454545455f);
			}
		}
		
		Color::Color(Color *other)
		{
			_color = other->_color;
			_uncorrected = other->_uncorrected;
		}
		
		
		
		
		Color *Color::WithRNColor(const RN::Color &tcolor)
		{
			Color *color = new Color(tcolor);
			return color->Autorelease();
		}
		Color *Color::WithCorrectedRNColor(const RN::Color &tcolor)
		{
			Color *color = new Color(tcolor, nullptr);
			return color->Autorelease();
		}
		Color *Color::WithRGB(float r, float g, float b)
		{
			Color *color = new Color(RN::Color(r, g, b));
			return color->Autorelease();
		}
		Color *Color::WithRGBA(float r, float g, float b, float a)
		{
			Color *color = new Color(RN::Color(r, g, b, a));
			return color->Autorelease();
		}
		Color *Color::WithWhite(float white, float a)
		{
			Color *color = new Color(RN::Color(white, white, white, a));
			return color->Autorelease();
		}
		Color *Color::WithHSV(float h, float s, float v)
		{
			Color *color = new Color(RN::Color::WithHSV(h, s, v));
			return color->Autorelease();
		}
		
		
		Color *Color::RedColor()
		{
			static Color *color = nullptr;
			if(!color)
				color = new Color(RN::Color::Red());
			
			return color;
		}
		Color *Color::GreenColor()
		{
			static Color *color = nullptr;
			if(!color)
				color = new Color(RN::Color::Green());
			
			return color;
		}
		Color *Color::BlueColor()
		{
			static Color *color = nullptr;
			if(!color)
				color = new Color(RN::Color::Blue());
			
			return color;
		}
		Color *Color::YellowColor()
		{
			static Color *color = nullptr;
			if(!color)
				color = new Color(RN::Color::Yellow());
			
			return color;
		}
		Color *Color::BlackColor()
		{
			static Color *color = nullptr;
			if(!color)
				color = new Color(RN::Color::Black());
			
			return color;
		}
		Color *Color::WhiteColor()
		{
			static Color *color = nullptr;
			if(!color)
				color = new Color(RN::Color::White());
			
			return color;
		}
		Color *Color::GrayColor()
		{
			static Color *color = nullptr;
			if(!color)
				color = new Color(RN::Color::Gray());
			
			return color;
		}
		Color *Color::ClearColor()
		{
			static Color *color = nullptr;
			if(!color)
				color = new Color(RN::Color::ClearColor());
			
			return color;
		}
		
		
		bool Color::IsEqual(Object *other) const
		{
			if(other->IsKindOfClass(Color::GetMetaClass()))
			{
				Color *otherColor = static_cast<Color *>(other);
				return (otherColor->_color == _color);
			}
			
			return false;
		}
	}
}
