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
			
			Style *styleSheet = Style::SharedInstance();
			Texture *texture = styleSheet->TextureWithName(style->ObjectForKey<String>(RNCSTR("texture")));
			
			Array *states = style->ObjectForKey<Array>(RNCSTR("states"));
			states->Enumerate([&](Object *object, size_t index, bool *stop) {
				Dictionary *state = object->Downcast<Dictionary>();
				
				String *name = state->ObjectForKey<String>(RNCSTR("name"));
				Dictionary *atlas  = state->ObjectForKey<Dictionary>(RNCSTR("atlas"));
				Dictionary *insets = state->ObjectForKey<Dictionary>(RNCSTR("insets"));
				
				State tstate;
				if(name->IsEqual(RNCSTR("normal")))
					tstate = Normal;
				else
				if(name->IsEqual(RNCSTR("selected")))
					tstate = Selected;
				else
				if(name->IsEqual(RNCSTR("highlighted")))
					tstate = Highlighted;
				else
					return;
				
				Image *image = new Image(texture);
				
				if(atlas)
				{
					float x = atlas->ObjectForKey<Number>(RNCSTR("x"))->FloatValue();
					float y = atlas->ObjectForKey<Number>(RNCSTR("y"))->FloatValue();
					float width  = atlas->ObjectForKey<Number>(RNCSTR("width"))->FloatValue();
					float height = atlas->ObjectForKey<Number>(RNCSTR("height"))->FloatValue();
					
					image->SetAtlas(Atlas(x, y, width, height), false);
				}
				
				if(insets)
				{
					float top = insets->ObjectForKey<Number>(RNCSTR("top"))->FloatValue();
					float bottom = insets->ObjectForKey<Number>(RNCSTR("bottom"))->FloatValue();
					float left  = insets->ObjectForKey<Number>(RNCSTR("left"))->FloatValue();
					float right = insets->ObjectForKey<Number>(RNCSTR("right"))->FloatValue();
					
					image->SetEdgeInsets(EdgeInsets(top, bottom, left, right));
				}
				
				this->SetImageForState(image->Autorelease(), tstate);
			});
		}
		
		Button::~Button()
		{
			for(auto i=_images.begin(); i!=_images.end(); i++)
			{
				i->second->Release();
			}
		}
		
		Button *Button::WithType(Type type)
		{
			Dictionary *style = nullptr;
			
			switch(type)
			{
				case Type::RoundedRect:
					style = Style::SharedInstance()->ButtonStyle(RNCSTR("RNRoundedRect"));
					break;
					
				case Type::PushButton:
					style = Style::SharedInstance()->ButtonStyle(RNCSTR("RNPushButton"));
					break;
			}
			
			Button *button = new Button(style);
			return button->Autorelease();
		}
		
		
		void Button::Initialize()
		{
			_image = new ImageView();
			_label = new Label();
			
			_image->SetFrame(Bounds());
			_label->SetFrame(Bounds());
			
			_label->SetTextColor(Color::White());
			_label->SetAlignment(TextAlignment::Center);
			
			AddSubview(_image);
			AddSubview(_label);
			
			StateChanged(ControlState());
			DrawMaterial()->SetShader(ResourcePool::SharedInstance()->ResourceWithName<Shader>(kRNResourceKeyUIImageShader));
		}
		
		void Button::StateChanged(State state)
		{
			Control::StateChanged(state);
			
			do {
				if((state & Control::Disabled) && ActivateImage(Control::Disabled))
					break;
				
				if((state & Control::Selected) && ActivateImage(Control::Selected))
					break;
				
				if((state & Control::Highlighted) && ActivateImage(Control::Highlighted))
					break;
				
				ActivateImage(Control::Normal);
			} while(0);
			
			do {
				if((state & Control::Disabled) && ActivateTitle(Control::Disabled))
					break;
				
				if((state & Control::Selected) && ActivateTitle(Control::Selected))
					break;
				
				if((state & Control::Highlighted) && ActivateTitle(Control::Highlighted))
					break;
				
				ActivateTitle(Control::Normal);
			} while(0);
		}
		
		bool Button::ActivateImage(State state)
		{
			auto iterator = _images.find(state);
			if(iterator != _images.end())
			{
				_image->SetImage(iterator->second);
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
				return true;
			}
			
			return false;
		}
		
		
		
		void Button::SetFrame(const Rect& frame)
		{
			Control::SetFrame(frame);
			
			_image->SetFrame(Bounds());
			_label->SetFrame(Bounds());
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
	}
}
