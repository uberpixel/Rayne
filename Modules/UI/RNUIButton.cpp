//
//  RNUIButton.cpp
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIButton.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Button, ImageView)

		Button::Button(const TextAttributes &defaultTextAttributes) : _imageNormal(nullptr), _imageHighlight(nullptr), _backgroundColorNormal(Color::ClearColor()), _backgroundColorHighlight(Color::ClearColor()), _textColorNormal(Color::White()), _textColorHighlight(Color::Gray()), _isHighlighted(false)
		{
			_label = new Label(defaultTextAttributes);
			AddSubview(_label->Autorelease());
			
			_label->SetTextColor(_textColorNormal);
			_label->SetVerticalAlignment(TextVerticalAlignmentCenter);
			SetBackgroundColor(_backgroundColorNormal);
			SetImage(_imageNormal);
		}
	
		Button::~Button()
		{
			SafeRelease(_imageNormal);
			SafeRelease(_imageHighlight);
		}
	
		void Button::SetFrame(const Rect &frame)
		{
			View::SetFrame(frame);
			_label->SetFrame(GetBounds());
		}

		void Button::SetImageNormal(Texture *image)
		{
			SafeRelease(_imageNormal);
			_imageNormal = image;
			SafeRetain(_imageNormal);
			
			if(!_isHighlighted) SetImage(_imageNormal);
		}
	
		void Button::SetImageHighlight(Texture *image)
		{
			SafeRelease(_imageHighlight);
			_imageHighlight = image;
			SafeRetain(_imageHighlight);
			
			if(_isHighlighted) SetImage(_imageHighlight);
		}
	
		void Button::SetBackgroundColorNormal(const Color &color)
		{
			_backgroundColorNormal = color;
			if(!_isHighlighted) SetBackgroundColor(_backgroundColorNormal);
		}
	
		void Button::SetBackgroundColorHighlight(const Color &color)
		{
			_backgroundColorHighlight = color;
			if(_isHighlighted) SetBackgroundColor(_backgroundColorHighlight);
		}
	
		void Button::SetTextColorNormal(const Color &color)
		{
			_textColorNormal = color;
			if(!_isHighlighted) _label->SetTextColor(_textColorNormal);
		}
	
		void Button::SetTextColorHighlight(const Color &color)
		{
			_textColorHighlight = color;
			if(_isHighlighted) _label->SetTextColor(_textColorHighlight);
		}
	
		bool Button::UpdateCursorPosition(const Vector2 &cursorPosition)
		{
			bool needsRedraw = View::UpdateCursorPosition(cursorPosition);
			
			bool wasHighlighted = _isHighlighted;
			
			Vector2 transformedPosition = ConvertPointFromBase(cursorPosition);
			_isHighlighted = GetBounds().ContainsPoint(transformedPosition);
			
			if(GetIsHidden() || _combinedOpacityFactor <= RN::k::EpsilonFloat) _isHighlighted = false;
			
			if(_isHighlighted)
			{
				_label->SetTextColor(_textColorHighlight);
				SetBackgroundColor(_backgroundColorHighlight);
				SetImage(_imageHighlight);
			}
			else if(wasHighlighted)
			{
				_label->SetTextColor(_textColorNormal);
				SetBackgroundColor(_backgroundColorNormal);
				SetImage(_imageNormal);
			}
			
			return needsRedraw || wasHighlighted != _isHighlighted;
		}
	}
}
