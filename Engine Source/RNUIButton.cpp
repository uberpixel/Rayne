//
//  RNUIButton.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIButton.h"
#include "RNResourcePool.h"

namespace RN
{
	namespace UI
	{
		RNDeclareMeta(Button)
		
		Button::Button()
		{
			Initialize();
		}
		
		Button::~Button()
		{
			for(auto i=_images.begin(); i!=_images.end(); i++)
			{
				i->second->Release();
			}
		}
		
		void Button::Initialize()
		{
			_image = new ImageView();
			_image->SetFrame(Bounds());
			
			AddSubview(_image);
			
			StateChanged(ControlState());
			DrawMaterial()->SetShader(ResourcePool::SharedInstance()->ResourceWithName<Shader>(kRNResourceKeyUIImageShader));
		}
		
		void Button::StateChanged(State state)
		{
			Control::StateChanged(state);
			
			if((state & Control::Disabled) && ActivateImage(Control::Disabled))
				return;
			
			if((state & Control::Selected) && ActivateImage(Control::Selected))
				return;
			
			if((state & Control::Highlighted) && ActivateImage(Control::Highlighted))
				return;
			
			ActivateImage(Control::Normal);
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
		
		
		void Button::SetFrame(const Rect& frame)
		{
			Control::SetFrame(frame);
			_image->SetFrame(Bounds());
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
