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
			float movementRange = frame.width - 15.0f;
			
			_handleView = new View(Rect(sliderPosition * movementRange, 0.0f, 15.0f, frame.height));
			_handleView->SetBackgroundColor(Color::Black());
			AddSubview(_handleView);
		}

		Slider::~Slider()
		{
			
		}

		void Slider::Update(float delta, Vector2 cursorPosition, bool touched)
		{
			Vector2 transformedPosition = ConvertPointFromBase(cursorPosition);
			if(!GetBounds().ContainsPoint(transformedPosition)) return;
			
			if(touched)
			{
				float movementRange = GetBounds().width - 15.0f;
				_value = (transformedPosition.x - GetBounds().x) * (_to - _from) / movementRange + _from;
				if(_step > k::EpsilonFloat) _value = std::round(_value / _step) * _step;
				if(_value > _to) _value = _to;
				if(_value < _from) _value = _from;
				
				float sliderPosition = (_value - _from) / (_to - _from);
				Rect handleFrame = _handleView->GetFrame();
				handleFrame.x = sliderPosition * movementRange;
				_handleView->SetFrame(handleFrame);
			}
		}
	}
}
