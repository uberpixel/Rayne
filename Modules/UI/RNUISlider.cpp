//
//  RNUISlider.cpp
//  Rayne
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#include "RNUISlider.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Slider, View)

		Slider::Slider(const Rect &frame, float value, float from, float to, float step) : View(frame), _value(value), _from(from), _to(to), _step(step)
		{
			float lineWidth = 4.0;
			View *rangeView = new View(Rect(0.0f, (frame.height - lineWidth) * 0.5, frame.width, lineWidth));
			rangeView->SetBackgroundColor(Color::White());
			AddSubview(rangeView);
			
			if(_value < from) _value = from;
			if(_value > to) _value = to;
			
			float sliderPosition = (_value - _from) / (_to - _from);
			
			_handleView = new View(Rect(sliderPosition * frame.width, 0.0f, 15.0f, frame.height));
			_handleView->SetBackgroundColor(Color::Black());
			AddSubview(_handleView);
		}

		Slider::~Slider()
		{
			
		}

		void Slider::Update(float delta, Vector2 cursorPosition, bool touched)
		{
			if(!GetFrame().ContainsPoint(cursorPosition)) return;
			
			if(touched)
			{
				_value = cursorPosition.x * (_to - _from) / GetFrame().width + _from;
				
				if(_step > k::EpsilonFloat) _value = std::round(_value / _step) * _step;
				
				float sliderPosition = (_value - _from) / (_to - _from);
				Rect handleFrame = _handleView->GetFrame();
				handleFrame.x = sliderPosition * GetFrame().width;
				_handleView->SetFrame(handleFrame);
			}
		}
	}
}
