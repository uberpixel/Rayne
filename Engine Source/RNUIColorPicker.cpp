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
			_color(nullptr)
		{
			_colorWheel = new ColorWheel();
			
			_colorKnob = new View(Rect(0.0f, 0.0f, 4.0, 4.0));
			_colorKnob->SetBackgroundColor(RN::Color::Black());
			
			View *secondKnob = new View(Rect(1.0f, 1.0f, 2.0f, 2.0f));
			secondKnob->SetBackgroundColor(RN::Color::White());
			
			_colorKnob->AddSubview(secondKnob->Autorelease());
			
			AddSubview(_colorWheel);
			AddSubview(_colorKnob);
			
			_color = Color::WithRNColor(RN::Color::Yellow())->Retain();
		}
		
		ColorPicker::~ColorPicker()
		{
			_colorWheel->Release();
			_colorKnob->Release();
			
			SafeRelease(_color);
		}
		
		
		void ColorPicker::SetColor(Color *color)
		{
			RN_ASSERT(color, "Color mustn't be NULL");
			
			SafeRelease(_color);
			_color = color->Retain();
			
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
			
			SafeRelease(_color);
			_color = ConvertColorFromWheel(location, 1.0)->Retain();
			
			DispatchEvent(EventType::ValueChanged);
		}
		
		void ColorPicker::LayoutSubviews()
		{
			Control::LayoutSubviews();
			
			Rect frame = GetBounds();
			Rect wheelRect = Rect(frame).Inset(5.0f, 5.0f);
			
			_colorWheel->SetFrame(wheelRect);
			
			_color->Retain();
			SetColor(_color);
			_color->Release();
		}
		
		
		
		
		Vector3 ColorPicker::ColorFromHSV(float h, float s, float v)
		{
			float hi = h * 3.0 / k::Pi;
			float f  = hi - floorf(hi);
			
			Vector4 components(0.0, s, s * f, s * (1.0 - f));
			components = (Vector4(1.0 - components.x, 1.0 - components.y, 1.0 - components.z, 1.0 - components.w)) * v;
			
			if(hi < -2.0)
			{
				return Vector3(components.x, components.w, components.y);
			}
			else if(hi < -1.0)
			{
				return Vector3(components.z, components.x, components.y);
			}
			else if(hi < 0.0)
			{
				return Vector3(components.y, components.x, components.w);
			}
			else if(hi < 1.0)
			{
				return Vector3(components.y, components.z, components.x);
			}
			else if(hi < 2.0)
			{
				return Vector3(components.w, components.y, components.x);
			}
			else
			{
				return Vector3(components);
			}
		}
		
		Vector3 ColorPicker::ColorToHSV(const Vector3 &color)
		{
			float max = color.GetMax();
			float min = color.GetMin();
			float diff = max - min;
			
			float h = 0.0f;
			float s = 0.0f;
			float v = max;
			
			if(!Math::Compare(max, min))
			{
				if(Math::Compare(max, color.x))
				{
					h = k::Pi/3.0f * (color.y - color.z) / diff;
				}
				else if(Math::Compare(max, color.y))
				{
					h = k::Pi/3.0f * (2.0f + (color.z - color.x) / diff);
				}
				else if(Math::Compare(max, color.z))
				{
					h = k::Pi/3.0f * (4.0f + (color.x - color.y) / diff);
				}
				
				if(h < 0.0f)
				{
					h += 2.0f * k::Pi;
				}
			}
			
			if(!Math::Compare(max, 0.0f))
			{
				s = diff/max;
			}
			
			return Vector3(h, s, v);
		}
		
		Color *ColorPicker::ConvertColorFromWheel(const Vector2 &position, float brightness)
		{
			float theta = atan2(position.y, -position.x);
			float r = position.GetLength();
			
			Vector3 rgb = ColorFromHSV(theta, r, brightness);
			return Color::WithRNColor(RN::Color(rgb.x, rgb.y, rgb.z, 1.0));
		}
		
		Vector2 ColorPicker::ConvertColorToWheel(Color *color, float &brightness)
		{
			RN::Color rnColor = color->GetRNColor();
			Vector3 myColor(rnColor.r, rnColor.g, rnColor.b);
			
			Vector3 hsv = ColorToHSV(myColor);
			brightness = hsv.z;
			
			return Vector2(hsv.y * cos(hsv.x), hsv.y * -sin(hsv.x));
		}
	}
}
