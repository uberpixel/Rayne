//
//  RNUIButton.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIButton.h"
#include "RNUIStyle.h"
#include "RNResourcePool.h"
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
		
		Button::Button(Dictionary *style)
		{
			Initialize();
			
			Style *styleSheet = Style::GetSharedInstance();
			Texture *texture = styleSheet->TextureWithName(style->GetObjectForKey<String>(RNCSTR("texture")));
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
					this->SetBackgroundImageForState(image->Autorelease(), tstate);
				else
					this->SetImageForState(image->Autorelease(), tstate);
			});
			
			SetContentInsets(Style::ParseEdgeInsets(style->GetObjectForKey<Dictionary>(RNCSTR("contentInsets"))));
			SizeToFit();
		}
		
		Button::~Button()
		{
			for(auto i=_titles.begin(); i!=_titles.end(); i++)
			{
				i->second->Release();
			}
			
			for(auto i=_images.begin(); i!=_images.end(); i++)
			{
				i->second->Release();
			}
			
			for(auto i=_backgroundImages.begin(); i!=_backgroundImages.end(); i++)
			{
				i->second->Release();
			}
			
			_label->Release();
			_image->Release();
			_backgroundImage->Release();
		}
		
		Button *Button::WithType(Type type)
		{
			Dictionary *style = nullptr;
			
			switch(type)
			{
				case Type::RoundedRect:
					style = Style::GetSharedInstance()->ButtonStyle(RNCSTR("RNRoundedRect"));
					break;
					
				case Type::PushButton:
					style = Style::GetSharedInstance()->ButtonStyle(RNCSTR("RNPushButton"));
					break;
					
				case Type::CheckBox:
					style = Style::GetSharedInstance()->ButtonStyle(RNCSTR("RNCheckBox"));
					break;
			}
			
			Button *button = new Button(style);
			return button->Autorelease();
		}
		
		
		void Button::Initialize()
		{
			_currentBackground = nullptr;
			_currentImage = nullptr;
			_currentTitle = nullptr;
			
			_behavior = Behavior::Momentarily;
			_position = ImagePosition::Left;
			
			_backgroundImage = new ImageView();
			_image = new ImageView();
			_label = new Label();
			
			_backgroundImage->SetFrame(Bounds());
			
			_label->SetFrame(Bounds());
			_label->SetAlignment(TextAlignment::Center);
			
			_image->SetFrame(Bounds());
			_image->SetScaleMode(ScaleMode::ProportionallyDown);
			
			AddSubview(_backgroundImage);
			AddSubview(_image);
			AddSubview(_label);
			
			StateChanged(ControlState());
		}
		
		void Button::StateChanged(State state)
		{
#define TryActivatingBackgroundImage(s) \
			if((state & s) && ActivateBackgroundImage(s)) \
				break
			
#define TryActivatingImage(s) \
			if((state & s) && ActivateImage(s)) \
				break

#define TryActivatingTitle(s) \
			if((state & s) && ActivateTitle(s)) \
				break
	
			_backgroundImage->SetImage(nullptr);
			_image->SetImage(nullptr);
			_label->SetText(RNCSTR(""));
			
			_currentTitle = nullptr;
			_currentImage = _currentBackground = nullptr;
			
			do {
				TryActivatingBackgroundImage(Control::Disabled);
				TryActivatingBackgroundImage(Control::Selected);
				TryActivatingBackgroundImage(Control::Highlighted);
				
				ActivateBackgroundImage(Control::Normal);
			} while(0);
			
			do {
				TryActivatingImage(Control::Disabled);
				TryActivatingImage(Control::Selected);
				TryActivatingImage(Control::Highlighted);
				
				ActivateImage(Control::Normal);
			} while(0);
			
			do {
				TryActivatingTitle(Control::Disabled);
				TryActivatingTitle(Control::Selected);
				TryActivatingTitle(Control::Highlighted);
				
				ActivateTitle(Control::Normal);
			} while(0);
			
			SetNeedsLayoutUpdate();
		}
		
		bool Button::ActivateBackgroundImage(State state)
		{
			auto iterator = _backgroundImages.find(state);
			if(iterator != _backgroundImages.end())
			{
				_backgroundImage->SetImage(iterator->second);
				_currentBackground = iterator->second;
				
				return true;
			}
			
			return false;
		}
		
		bool Button::ActivateImage(State state)
		{
			auto iterator = _images.find(state);
			if(iterator != _images.end())
			{
				_image->SetImage(iterator->second);
				_currentImage = iterator->second;
				
				return true;
			}
			
			return false;
		}
		
		bool Button::ActivateTitle(State state)
		{
			auto iterator = _titles.find(state);
			if(iterator != _titles.end())
			{
				_label->SetText(iterator->second);
				_currentTitle = iterator->second;
				
				return true;
			}
			
			return false;
		}
		
		
		
		void Button::SetFrame(const Rect& frame)
		{
			Control::SetFrame(frame);
			_backgroundImage->SetFrame(Bounds());
		}
		
		void Button::SetContentInsets(const EdgeInsets& insets)
		{
			_contentInsets = insets;
			SetNeedsLayoutUpdate();
		}
		
		void Button::SetTitleForState(String *title, State state)
		{
			auto iterator = _titles.find(state);
			if(iterator != _titles.end())
			{
				iterator->second->Release();
				
				if(title)
				{
					iterator->second = title->Retain();
					
					StateChanged(ControlState());
					return;
				}
				
				_titles.erase(iterator);
				
				StateChanged(ControlState());
				return;
			}
			
			_titles.insert(std::map<State, String *>::value_type(state, title->Retain()));
			StateChanged(ControlState());
		}
		
		void Button::SetBackgroundImageForState(Image *image, State state)
		{
			auto iterator = _backgroundImages.find(state);
			if(iterator != _backgroundImages.end())
			{
				iterator->second->Release();
				
				if(image)
				{
					iterator->second = image->Retain();
					
					StateChanged(ControlState());
					return;
				}
				
				_backgroundImages.erase(iterator);
				
				StateChanged(ControlState());
				return;
			}
			
			_backgroundImages.insert(std::map<State, Image *>::value_type(state, image->Retain()));
			StateChanged(ControlState());
		}
		
		void Button::SetImageForState(Image *image, State state)
		{
			auto iterator = _images.find(state);
			if(iterator != _images.end())
			{
				iterator->second->Release();
				
				if(image)
				{
					iterator->second = image->Retain();
					
					StateChanged(ControlState());
					return;
				}
				
				_images.erase(iterator);
				
				StateChanged(ControlState());
				return;
			}
			
			_images.insert(std::map<State, Image *>::value_type(state, image->Retain()));
			StateChanged(ControlState());
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
		
		Vector2 Button::SizeThatFits()
		{
			State temp = ControlState();
			StateChanged(Control::Normal);
			
			Vector2 size = Vector2(_contentInsets.left + _contentInsets.right, _contentInsets.top + _contentInsets.bottom);
			
			Vector2 titleSize = std::move(_label->SizeThatFits());
			Vector2 imageSize = std::move(_image->SizeThatFits());
			
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
			
			Vector2 titleSize = std::move(_label->SizeThatFits());
			Vector2 imageSize = std::move(_image->SizeThatFits());
			
			Vector2 insetSize = Vector2(_contentInsets.left + _contentInsets.right, _contentInsets.top + _contentInsets.bottom);
			Vector2 size = Frame().Size();
			Vector2 truncatedSize = size - insetSize;
			
			titleSize.x = truncatedSize.x > titleSize.x ? titleSize.x : truncatedSize.x;
			titleSize.y = truncatedSize.y > titleSize.y ? titleSize.y : truncatedSize.y;
			
			imageSize.x = truncatedSize.x > imageSize.x ? imageSize.x : truncatedSize.x;
			imageSize.y = truncatedSize.y > imageSize.y ? imageSize.y : truncatedSize.y;
			
			Vector2 halfTitle = titleSize * 0.5f;
			Vector2 halfImage = imageSize * 0.5f;
			
			Vector2 center = size * 0.5f;
			
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
					if(!fitsHorizontally || _label->Alignment() == TextAlignment::Left)
					{
						Rect titleRect = Rect(_contentInsets.left + imageSize.x, centeredTitle.y, size.x - (imageSize.x + insetSize.x), titleSize.y);
						_label->SetFrame(titleRect);
					}
					
					_image->SetFrame(Rect(_contentInsets.left, centeredImage.y, imageSize.x, imageSize.y));
					break;
					
				case ImagePosition::Right:
					if(!fitsHorizontally || _label->Alignment() == TextAlignment::Right)
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
