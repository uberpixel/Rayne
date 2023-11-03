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

		Slider::Slider(const Rect &frame, float value, float from, float to, float step) : View(frame), _value(value), _from(from), _to(to), _step(step), _isActive(false)
		{
			float lineWidth = 4.0;
			_rangeView = new View(Rect(0.0f, (frame.height - lineWidth) * 0.5, frame.width, lineWidth));
			_rangeView->SetCornerRadius(lineWidth * 0.5f);
			_rangeView->SetBackgroundColor(Color::White());
			AddSubview(_rangeView);
			
			_handleView = new View(Rect(0.0f, 0.0f, 15.0f, GetFrame().height));
			_handleView->SetCornerRadius(7.5f);
			_handleView->SetBackgroundColor(Color::Black());
			SetValue(_value);
			AddSubview(_handleView);
		}

		Slider::~Slider()
		{
			
		}
	
		void Slider::SetValue(float value)
		{
			_value = value;
			
			float clampedValue = value;
			if(clampedValue < _from) clampedValue = _from;
			if(clampedValue > _to) clampedValue = _to;
			
			float sliderPosition = (clampedValue - _from) / (_to - _from);
			float movementRange = GetFrame().width - 15.0f;
			
			Rect handleFrame = _handleView->GetFrame();
			handleFrame.x = sliderPosition * movementRange;
			_handleView->SetFrame(handleFrame);
		}
	
		void Slider::SetRange(float from, float to, float step)
		{
			_from = from;
			_to = to;
			_step = step;
			SetValue(_value);
		}

		void Slider::Update(float delta, Vector2 cursorPosition, bool touched)
		{
			Vector2 transformedPosition = ConvertPointFromBase(cursorPosition);
			if(!GetBounds().ContainsPoint(transformedPosition) && !_isActive) return;
			
			if(touched)
			{
				_isActive = true;
				
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
			else
			{
				_isActive = false;
			}
		}
	}
}
