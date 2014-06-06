//
//  RNUISlider.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUISlider.h"
#include "RNUIStyle.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Slider, Control)
		
		Slider::Slider(Direction direction) :
			Slider(Style::GetSharedInstance()->GetResourceWithKeyPath<Dictionary>(RNCSTR("slider")), direction)
		{}
		
		Slider::Slider(Dictionary *style, Direction direction) :
			_min(0.0f),
			_max(1.0f),
			_value(0.5f),
			_direction(direction)
		{
			_background = new ImageView();
			_knob = new ImageView();
			
			_background->SetFrame(GetBounds());
			_background->SetAutoresizingMask(AutoresizingMask::FlexibleWidth | AutoresizingMask::FlexibleHeight);
			
			AddSubview(_background);
			AddSubview(_knob);
			
			if(style)
			{
				Style *styleSheet = Style::GetSharedInstance();
				Dictionary *background = nullptr;
				
				if(_direction == Direction::Vertical)
					background = style->GetObjectForKey<Dictionary>(RNCSTR("vertical"));
				else
					background = style->GetObjectForKey<Dictionary>(RNCSTR("horizontal"));
				
				if(background)
				{
					Image *image = Style::ParseImage(background);
					_background->SetImage(image);
				}
				
				Dictionary *knob = style->GetObjectForKey<Dictionary>(RNCSTR("knob"));
				Texture *texture = styleSheet->GetTextureWithName(knob->GetObjectForKey<String>(RNCSTR("texture")));
				Dictionary *tlInsets = knob->GetObjectForKey<Dictionary>(RNCSTR("insets"));
				
				Array *states = knob->GetObjectForKey<Array>(RNCSTR("states"));
				
				states->Enumerate<Dictionary>([&](Dictionary *object, size_t index, bool &stop) {
					Dictionary *state = object->Downcast<Dictionary>();
					
					String *name = state->GetObjectForKey<String>(RNCSTR("name"));
					Dictionary *atlas  = state->GetObjectForKey<Dictionary>(RNCSTR("atlas"));
					Dictionary *insets = state->GetObjectForKey<Dictionary>(RNCSTR("insets"));
					
					if(!insets)
						insets = tlInsets;
					
					State tstate = Style::ParseState(name);
					Image *image = new Image(texture);
					
					if(atlas)
						image->SetAtlas(Style::ParseAtlas(atlas), false);
					
					if(insets)
						image->SetEdgeInsets(Style::ParseEdgeInsets(insets));
					
					_knobImages.SetValueForState(image->Autorelease(), tstate);
					
				});
			}
			
			StateChanged(GetState());
			SetNeedsLayoutUpdate();
		}
		
		Slider::~Slider()
		{
			_background->Release();
			_knob->Release();
		}
		
		void Slider::SetMinValue(float minValue)
		{
			_min = minValue;
			SetNeedsLayoutUpdate();
		}
		
		void Slider::SetMaxValue(float maxValue)
		{
			_max = maxValue;
			SetNeedsLayoutUpdate();
		}
		
		void Slider::SetValue(float value)
		{
			_value = std::min(_max, std::max(_min, value));
			SetNeedsLayoutUpdate();
		}
		
		void Slider::StateChanged(State state)
		{
			Control::StateChanged(state);
			_knob->SetImage(_knobImages.GetValueForState(state));
		}
		
		void Slider::LayoutSubviews()
		{
			Control::LayoutSubviews();
			_value = std::min(_max, std::max(_min, _value));

			Rect rect = GetBounds();
			Rect knob = _knob->GetBounds();
			
			float offset = 0.5; //(_value - _min) / (_max - _min);
			
			if(_direction == Direction::Vertical)
			{
				// Knob
				{
					float x = (rect.width * 0.5) - (knob.width * 0.5);
					float y = (offset * rect.height) - (knob.height * 0.5);
					
					_knob->SetFrame(Rect(x, y, knob.width, knob.height));
				}
				
				// Background
				{
					Image *image = _knob->GetImage();
					float x = (rect.width * 0.5) - (image->GetWidth() * 0.5);
					
					_background->SetFrame(Rect(x, 0.0, image->GetWidth(), rect.height));
				}
			}
			else
			{
				// Knob
				{
					float x = (offset * rect.width) - (knob.width * 0.5);
					float y = (rect.height * 0.5) - (knob.height * 0.5);
					
					_knob->SetFrame(Rect(x, y, knob.width, knob.height));
				}
				
				// Background
				{
					Image *image = _knob->GetImage();
					float y = (rect.height * 0.5) - (image->GetHeight() * 0.5);
					
					_background->SetFrame(Rect(0.0, y, rect.width, image->GetHeight()));
				}
			}
		}
	}
}
