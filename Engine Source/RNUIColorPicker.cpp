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
			
			
			AddSubview(_colorWheel);
			AddSubview(_colorKnob);
			
			AddSubview(_brightnessView);
		}
		
		ColorPicker::~ColorPicker()
		{
			_colorWheel->Release();
			_colorKnob->Release();
		}
		
		
		void ColorPicker::SetColor(const RN::Color &color)
		{
			_color = color;
			
			float brightness = 0;
			Vector2 position = ConvertColorToWheel(_color, brightness);
			position *= _colorWheel->GetBounds().GetSize();
			position += _colorWheel->GetBounds().GetSize() * 0.5f;
			UpdateKnob(position);
		}
		
		
		
		void ColorPicker::MouseDown(Event *event)
		{
			Control::MouseDown(event);
			
			Vector2 point = event->GetMousePosition();
			point = std::move(_colorWheel->ConvertPointFromBase(point));
			
			UpdateKnob(point);
		}
		void ColorPicker::MouseDragged(Event *event)
		{
			Control::MouseDragged(event);
			
			Vector2 point = event->GetMousePosition();
			point = std::move(_colorWheel->ConvertPointFromBase(point));
			
			UpdateKnob(point);
		}
		
		void ColorPicker::UpdateKnob(const Vector2 &position)
		{
			Vector2 location = position;
			Vector2 center = _colorWheel->GetBounds().GetSize() * 0.5f;
			float distance = position.GetDistance(center);
			
			if(distance > center.x)
			{
				float angle = atan2f(location.x - center.x, location.y - center.y);
				
				location.x = center.x + (center.x * sinf(angle));
				location.y = center.x + (center.x * cosf(angle));
			}
			
			
			Rect frame = _colorKnob->GetFrame();
			
			frame.x = location.x + 2.0;
			frame.y = location.y + 2.0;
			
			_colorKnob->SetFrame(frame);
			
			location = location / _colorWheel->GetBounds().GetSize();
			
			_color = ConvertColorFromWheel(location * 2.0f - 1.0f, 1.0);
			
			UpdateBrightness();
			DispatchEvent(EventType::ValueChanged);
		}
		
		void ColorPicker::UpdateBrightness()
		{
			_brightnessView->SetStartColor(_color);
			_brightnessView->SetEndColor(RN::Color::Black());
		}
		
		void ColorPicker::LayoutSubviews()
		{
			Control::LayoutSubviews();
			
			Rect frame = GetBounds();
			
			Rect wheelRect = Rect(frame.x, frame.y, frame.width - 20.0f, frame.height);
			
			
			wheelRect.width  = std::min(wheelRect.height, wheelRect.width);
			wheelRect.height = wheelRect.width;
			
			
			Rect brightnessRect = Rect(frame.width - 15.0f, 0.0f, 15.0f, wheelRect.height);
			
			_colorWheel->SetFrame(wheelRect);
			_brightnessView->SetFrame(brightnessRect);
			
			SetColor(_color);
		}
		
		RN::Color ColorPicker::ConvertColorFromWheel(const Vector2 &position, float brightness)
		{
			float theta = atan2(position.y, -position.x);
			float r = position.GetLength();
			
			return RN::Color::WithHSV(theta, r, brightness);
		}
		
		Vector2 ColorPicker::ConvertColorToWheel(const RN::Color &color, float &brightness)
		{
			Vector4 hsva = color.GetHSV();
			brightness = hsva.z;
			
			return Vector2(hsva.y * cos(hsva.x), hsva.y * -sin(hsva.x));
		}
	}
}
