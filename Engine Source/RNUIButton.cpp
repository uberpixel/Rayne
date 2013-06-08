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
			if(_mesh)
				_mesh->Release();
			
			for(auto i=_images.begin(); i!=_images.end(); i++)
			{
				i->second->Release();
			}
		}
		
		void Button::Initialize()
		{
			_mesh = nullptr;
			_activeImage = nullptr;
			
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
				SetActiveImage(iterator->second);
				return true;
			}
			
			return false;
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
		
		void Button::SetActiveImage(Image *image)
		{
			if(_mesh)
			{
				_mesh->Release();
				_mesh = nullptr;
			}
			
			_activeImage = image;
			if(!image)
				return;
			
			_mesh = image->FittingMesh(Frame().Size())->Retain();
			
			Rect frame = Frame();
			frame.width  = image->Width();
			frame.height = image->Height();
			
			SetFrame(frame);
			DrawMaterial()->RemoveTextures();
			DrawMaterial()->AddTexture(_activeImage->Texture());
		}
		
		bool Button::Render(RenderingObject& object)
		{
			object.mesh = _mesh;
			return (_activeImage != 0);
		}
	}
}
