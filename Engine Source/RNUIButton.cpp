//
//  RNUIButton.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIButton.h"
#include "RNUIStyle.h"
#include "RNNumber.h"

#define kRNUIButtonContentGap 5.0f

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(Button)
		
		Button::Button()
		{
			Initialize();
		}
		
		Button::Button(Type type)
		{
			Dictionary *style = nullptr;
			switch(type)
			{
				case Type::Bezel:
					style = Style::GetSharedInstance()->GetButtonStyleWithKeyPath(RNCSTR("buttons.RNBezel"));
					break;
					
				case Type::CheckBox:
					style = Style::GetSharedInstance()->GetButtonStyleWithKeyPath(RNCSTR("buttons.RNCheckBox"));
					break;
					
				case Type::DisclosureTriangle:
					style = Style::GetSharedInstance()->GetButtonStyleWithKeyPath(RNCSTR("buttons.RNDisclosureTriangle"));
					break;
			}
			
			Initialize();
			InitializeFromStyle(style);
		}
		
		Button::Button(Dictionary *style)
		{
			Initialize();
			InitializeFromStyle(style);
		}
		
		Button::~Button()
		{
			_label->Release();
			_image->Release();
			_backgroundImage->Release();
		}
		
		Button *Button::WithType(Type type)
		{
			Button *button = new Button(type);
			return button->Autorelease();
		}
		
		
		void Button::Initialize()
		{
			_behavior = Behavior::Momentarily;
			_position = ImagePosition::Left;
			
			_backgroundImage = new ImageView();
			_image = new ImageView();
			_label = new Label();
			
			_backgroundImage->SetFrame(GetBounds());
			
			_label->SetFrame(GetBounds());
			_label->SetAlignment(TextAlignment::Center);
			
			_image->SetFrame(GetBounds());
			_image->SetScaleMode(ScaleMode::ProportionallyDown);
			
			_currentTitle = nullptr;
			_currentImage = nullptr;
			
			AddSubview(_backgroundImage);
			AddSubview(_image);
			AddSubview(_label);
			
			StateChanged(GetState());
			SetBackgroundColor(RN::Color::ClearColor());
		}
		
		void Button::InitializeFromStyle(Dictionary *style)
		{
			RN_ASSERT(style, "Button style mustn't be NULL!");
			
			Style *styleSheet = Style::GetSharedInstance();
			Texture *texture = styleSheet->GetTextureWithName(style->GetObjectForKey<String>(RNCSTR("texture")));
			Dictionary *tlInsets = style->GetObjectForKey<Dictionary>(RNCSTR("insets"));
			String *mode = style->GetObjectForKey<String>(RNCSTR("mode"));
			Number *background = style->GetObjectForKey<Number>(RNCSTR("background"));
			
			bool useBackground = background ? background->GetBoolValue() : true;
			
			if(mode->IsEqual(RNCSTR("momentarily")))
				SetBehavior(Behavior::Momentarily);
			
			if(mode->IsEqual(RNCSTR("switch")))
				SetBehavior(Behavior::Switch);
			
			Array *states = style->GetObjectForKey<Array>(RNCSTR("states"));
			states->Enumerate([&](Object *object, size_t index, bool *stop) {
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
				
				if(useBackground)
					SetBackgroundImageForState(image->Autorelease(), tstate);
				else
					SetImageForState(image->Autorelease(), tstate);
			});
			
			SetContentInsets(Style::ParseEdgeInsets(style->GetObjectForKey<Dictionary>(RNCSTR("contentInsets"))));
			SizeToFit();
		}
		
		
		
		void Button::StateChanged(State state)
		{
			String *title = _titles.GetValueForState(state);
			
			_backgroundImage->SetImage(_backgroundImages.GetValueForState(state));
			_image->SetImage(_images.GetValueForState(state));
			_label->SetText(title ? title : RNCSTR(""));
			
			_currentImage = _image->GetImage();
			_currentTitle = _label->GetText();
			
			SetNeedsLayoutUpdate();
		}
		

		
		void Button::SetFrame(const Rect& frame)
		{
			Control::SetFrame(frame);
			_backgroundImage->SetFrame(GetBounds());
		}
		
		void Button::SetContentInsets(const EdgeInsets& insets)
		{
			_contentInsets = insets;
			SetNeedsLayoutUpdate();
		}
		
		void Button::SetTitleForState(String *title, State state)
		{
			_titles.SetValueForState(title, state);
			StateChanged(GetState());
		}
		
		void Button::SetBackgroundImageForState(Image *image, State state)
		{
			_backgroundImages.SetValueForState(image, state);
			StateChanged(GetState());
		}
		
		void Button::SetImageForState(Image *image, State state)
		{
			_images.SetValueForState(image, state);
			StateChanged(GetState());
		}
		
		void Button::SetBehavior(Behavior behavior)
		{
			_behavior = behavior;
		}
		
		void Button::SetImagePosition(ImagePosition position)
		{
			_position = position;
			
			switch(_position)
			{
				case ImagePosition::NoImage:
					_image->RemoveFromSuperview();
					AddSubview(_label);
					break;
					
				case ImagePosition::ImageOnly:
					_label->RemoveFromSuperview();
					AddSubview(_image);
					break;
					
				default:
					AddSubview(_label);
					AddSubview(_image);
					break;
			}
		}
		
		
		bool Button::PostEvent(EventType event)
		{
			if(!IsEnabled())
				return true;
			
			if(_behavior == Behavior::Switch)
			{
				switch(event)
				{
					case Control::EventType::MouseEntered:
					case Control::EventType::MouseLeft:
						return true;
						
					case Control::EventType::MouseDown:
						SetHighlighted(true);
						return true;
						
					case Control::EventType::MouseUpInside:
						SetSelected(!IsSelected());
						SetHighlighted(false);
						return true;
						
					case Control::EventType::MouseUpOutside:
						SetHighlighted(false);
						return true;
						
					default:
						break;
				}
			}
			
			return Control::PostEvent(event);
		}
		
		Vector2 Button::GetSizeThatFits()
		{
			State temp = GetState();
			StateChanged(Control::Normal);
			
			Vector2 size = Vector2(_contentInsets.left + _contentInsets.right, _contentInsets.top + _contentInsets.bottom);
			
			Vector2 titleSize = std::move(_label->GetSizeThatFits());
			Vector2 imageSize = std::move(_image->GetSizeThatFits());
			
			Vector2 max;
			max.x = std::max(titleSize.x, imageSize.x);
			max.y = std::max(titleSize.y, imageSize.y);
			
			switch(_position)
			{
				case ImagePosition::NoImage:
					size += titleSize;
					break;
					
				case ImagePosition::ImageOnly:
					size += imageSize;
					break;
					
				case ImagePosition::Overlaps:
					size += max;
					break;
					
				case ImagePosition::Left:
				case ImagePosition::Right:
					size.x += titleSize.x + imageSize.x + size.x + kRNUIButtonContentGap;
					size.y += max.y;
					break;
					
				case ImagePosition::Above:
				case ImagePosition::Below:
					size.x += max.x;
					size.y += titleSize.y + imageSize.y + size.y + kRNUIButtonContentGap;
					break;
			}
			
			StateChanged(temp);
			return size;
		}
		
		void Button::LayoutSubviews()
		{
			Control::LayoutSubviews();
			
			Vector2 titleSize = std::move(_label->GetSizeThatFits());
			Vector2 imageSize = std::move(_image->GetSizeThatFits());
			
			Vector2 insetSize = Vector2(_contentInsets.left + _contentInsets.right, _contentInsets.top + _contentInsets.bottom);
			Vector2 size = GetFrame().Size();
			Vector2 truncatedSize = size - insetSize;
			
			titleSize.x = truncatedSize.x > titleSize.x ? titleSize.x : truncatedSize.x;
			titleSize.y = truncatedSize.y > titleSize.y ? titleSize.y : truncatedSize.y;
			
			imageSize.x = truncatedSize.x > imageSize.x ? imageSize.x : truncatedSize.x;
			imageSize.y = truncatedSize.y > imageSize.y ? imageSize.y : truncatedSize.y;
			
			Vector2 halfTitle = Vector2(roundf(titleSize.x * 0.5f), roundf(titleSize.y * 0.5f));
			Vector2 halfImage = Vector2(roundf(imageSize.x * 0.5f), roundf(imageSize.y * 0.5f));
			
			Vector2 center = Vector2(roundf(size.x * 0.5f), roundf(size.y * 0.5f));
			
			Vector2 centeredTitle = center - halfTitle;
			Vector2 centeredImage = center - halfImage;
			
			bool simpleCenter = (_position == ImagePosition::Overlaps || _position == ImagePosition::NoImage || _position == ImagePosition::ImageOnly);
			
			_label->SetFrame(Rect(_contentInsets.left, centeredTitle.y, size.x - insetSize.x, titleSize.y));
			_image->SetFrame(Rect(_contentInsets.left, centeredImage.y, size.x - insetSize.x, imageSize.y));
			
			if(!_currentImage || !_currentTitle || simpleCenter)
				return;
			
			bool fitsHorizontally = (size.x - insetSize.x > titleSize.x + (imageSize.x * 2));
			//bool fitsVertically   = (size.y >= (titleSize.y + imageSize.y) - insetSize.y);
			
			switch(_position)
			{
				case ImagePosition::Left:
					if(!fitsHorizontally || _label->GetAlignment() == TextAlignment::Left)
					{
						Rect titleRect = Rect(_contentInsets.left + imageSize.x, centeredTitle.y, size.x - (imageSize.x + insetSize.x), titleSize.y);
						_label->SetFrame(titleRect);
					}
					
					_image->SetFrame(Rect(_contentInsets.left, centeredImage.y, imageSize.x, imageSize.y));
					break;
					
				case ImagePosition::Right:
					if(!fitsHorizontally || _label->GetAlignment() == TextAlignment::Right)
					{
						Rect titleRect = Rect(_contentInsets.left, centeredTitle.y, size.x - (imageSize.x + insetSize.x), titleSize.y);
						_label->SetFrame(titleRect);
					}
					
					_image->SetFrame(Rect(size.x - imageSize.x - _contentInsets.right, centeredImage.y, imageSize.x, imageSize.y));
					break;
					
				default:
					throw Exception(Exception::Type::GenericException, "Fix it, you lazy bastard!");
			}
		}
	}
}
