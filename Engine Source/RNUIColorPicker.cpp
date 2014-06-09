//
//  RNUIColorPicker.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIColorPicker.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(ColorPicker, Control)
		
		ColorPicker::ColorPicker() :
			ColorPicker(Rect())
		{}
		
		ColorPicker::ColorPicker(const Rect &frame) :
			Control(frame),
			_color(RN::Color::White())
		{
			_colorWheel = new ColorWheel();
			_brightnessView = new GradientView();
			
			
			{
				_colorKnob = new View(Rect(0.0f, 0.0f, 4.0, 4.0));
				_colorKnob->SetBackgroundColor(RN::Color::Black());
				
				View *secondKnob = new View(Rect(1.0f, 1.0f, 2.0f, 2.0f));
				secondKnob->SetBackgroundColor(RN::Color::White());
				
				_colorKnob->AddSubview(secondKnob->Autorelease());
			}
			
			{
				_brightnessKnob = new View(Rect(0.0f, 0.0f, 16.0, 5.0));
				
				View *top    = new View(Rect(0.0f, 0.0, 16.0, 1.0));
				View *bottom = new View(Rect(0.0f, 4.0, 16.0, 1.0));
				
				View *left  = new View(Rect(0.0f, 0.0,  1.0, 5.0));
				View *right = new View(Rect(16.0, 0.0, 1.0, 5.0));
				
				top->SetBackgroundColor(RN::Color::Black());
				bottom->SetBackgroundColor(RN::Color::Black());
				left->SetBackgroundColor(RN::Color::Black());
				right->SetBackgroundColor(RN::Color::Black());
				
				_brightnessKnob->AddSubview(top->Autorelease());
				_brightnessKnob->AddSubview(bottom->Autorelease());
				_brightnessKnob->AddSubview(left->Autorelease());
				_brightnessKnob->AddSubview(right->Autorelease());
			}
			
			_alphaSlider = new Slider(Slider::Direction::Horizontal);
			_alphaSlider->SetMinValue(0.0f);
			_alphaSlider->SetMaxValue(1.0f);
			_alphaSlider->AddListener(EventType::ValueChanged, [this](Control *control, EventType event) {
				
				RN::Color color = _color;
				color.a = _alphaSlider->GetValue();
				
				UpdateColor(color);
				
			}, nullptr);
			
			AddSubview(_colorWheel);
			AddSubview(_colorKnob);
			
			AddSubview(_brightnessView);
			AddSubview(_brightnessKnob);
			
			AddSubview(_alphaSlider);
		}
		
		ColorPicker::~ColorPicker()
		{
			_colorWheel->Release();
			_colorKnob->Release();
			
			_brightnessView->Release();
			_brightnessKnob->Release();
		}
		
		
		
		void ColorPicker::SetColor(const RN::Color &color)
		{
			if(color == _color)
				return;
			
			float brightness;
			Vector2 position = ConvertColorToWheel(color, brightness);
			
			position *= _colorWheel->GetBounds().GetSize();
			position += _colorWheel->GetBounds().GetSize() * 0.5f;
			
			_alphaSlider->SetValue(_color.a);
			
			UpdateColorKnob(position, false);
			
			UpdateColor(color);
			UpdateBrightness();
		}
		
		
		
		void ColorPicker::MouseDown(Event *event)
		{
			Control::MouseDown(event);
			
			_hit = nullptr;
			
			Vector2 point = event->GetMousePosition();
			UpdateWithMouseLocation(point);
		}
		void ColorPicker::MouseDragged(Event *event)
		{
			Control::MouseDragged(event);
			
			Vector2 point = event->GetMousePosition();
			UpdateWithMouseLocation(point);
		}
		void ColorPicker::MouseUp(Event *event)
		{
			Control::MouseUp(event);
			_hit = nullptr;
		}
		
		void ColorPicker::UpdateWithMouseLocation(const Vector2 &point)
		{
			if(!_hit)
			{
				Vector2 location = ConvertPointFromBase(point);
				
				if(_colorWheel->GetFrame().ContainsPoint(location))
					_hit = _colorWheel;
				
				if(_brightnessView->GetFrame().ContainsPoint(location))
					_hit = _brightnessView;
				
				if(_alphaSlider->GetFrame().ContainsPoint(location))
					_hit = _alphaSlider;
			}
			
			
			if(_hit == _colorWheel)
			{
				Vector2 wheelPoint = _colorWheel->ConvertPointFromBase(point);
				UpdateColorKnob(wheelPoint, true);
				return;
			}
			
			if(_hit == _brightnessView)
			{
				Vector2 brightnessPoint = _brightnessView->ConvertPointFromBase(point);
				
				if(brightnessPoint.y > _brightnessView->GetFrame().height || brightnessPoint.y < 0.0)
					return;
				
				_brightness = 1.0f - (brightnessPoint.y / _brightnessView->GetFrame().height);
				
				UpdateBrightness();
				DispatchEvent(EventType::ValueChanged);
				
				return;
			}
		}
		
		
		
		
		void ColorPicker::UpdateColorKnob(const Vector2 &position, bool updateColor)
		{
			Vector2 location = position;
			Vector2 center   = _colorWheel->GetBounds().GetSize() * 0.5f;
			float distance   = position.GetDistance(center);
			
			if(distance > center.x)
			{
				float angle = atan2f(location.x - center.x, location.y - center.y);
				
				location.x = center.x + (center.x * sinf(angle));
				location.y = center.x + (center.x * cosf(angle));
			}
			
			
			Rect frame = _colorKnob->GetFrame();
			
			frame.x = location.x - 2.0;
			frame.y = location.y - 2.0;
			
			_colorKnob->SetFrame(frame);
			
			if(updateColor)
			{
				location = location / _colorWheel->GetBounds().GetSize();
				UpdateColor(ConvertColorFromWheel(location * 2.0f - 1.0f, _brightness));
			}
		}
		
		
		void ColorPicker::UpdateColor(const RN::Color &color)
		{
			_color = color;
			
			{
				Vector2 position = ConvertColorToWheel(_color, _brightness);
				_fullColor = ConvertColorFromWheel(position, 1.0);
				_fullColor.a = 1.0f;
			}
			
			_brightnessView->SetStartColor(_fullColor);
			_brightnessView->SetEndColor(RN::Color::Black());
			
			UpdateBrightness();
			DispatchEvent(EventType::ValueChanged);
		}
		
		void ColorPicker::UpdateBrightness()
		{
			Rect frame = Rect(_brightnessView->GetFrame().x - 1.0f, 0.0, 16.0, 4.0f);
			
			frame.y  = _brightnessView->GetFrame().height * (1.0 - _brightness);
			frame.y -= 2.0;
			
			_brightnessKnob->SetFrame(frame);
			_colorWheel->SetBrightness(_brightness);
			
			{
				float brightness;
				Vector2 position = ConvertColorToWheel(_color, brightness);
				_color = ConvertColorFromWheel(position, _brightness);
			}
		}
		
		
		void ColorPicker::LayoutSubviews()
		{
			Control::LayoutSubviews();
			
			Rect frame = GetBounds();
			
			Rect wheelRect = Rect(frame.x, frame.y, frame.width - 20.0f, frame.height - 25.0f);
			
			wheelRect.width  = std::min(wheelRect.height, wheelRect.width);
			wheelRect.height = wheelRect.width;
			
			
			Rect brightnessRect = Rect(frame.width - 15.0f, 0.0f, 15.0f, wheelRect.height);
			
			_colorWheel->SetFrame(wheelRect);
			_brightnessView->SetFrame(brightnessRect);
			
			{
				Vector2 position = ConvertColorToWheel(_color, _brightness);
				position *= _colorWheel->GetFrame().GetSize();
				
				UpdateColorKnob(position, false);
				UpdateBrightness();
			}
			
			_alphaSlider->SetFrame(Rect(0.0f, frame.height - 20.0f, frame.width, 20.0f));
		}
		
		RN::Color ColorPicker::ConvertColorFromWheel(const Vector2 &position, float brightness)
		{
			float theta = atan2(position.y, -position.x);
			float r = position.GetLength();
			
			return RN::Color::WithHSV(theta, r, brightness, _alphaSlider->GetValue());
		}
		
		Vector2 ColorPicker::ConvertColorToWheel(const RN::Color &color, float &brightness)
		{
			Vector4 hsva = color.GetHSV();
			brightness = hsva.z;
			
			return Vector2(hsva.y * cos(hsva.x), hsva.y * -sin(hsva.x));
		}
	}
}
