//
//  RNUIImageView.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIImageView.h"
#include "RNResourcePool.h"

#define kRNImageViewShaderResourceName "kRNImageViewShaderResourceName"

namespace RN
{
	namespace UI
	{
		ImageView::ImageView()
		{
			Initialize();
		}
		
		ImageView::ImageView(const Rect& frame) :
			View(frame)
		{
			Initialize();
		}
		
		ImageView::ImageView(Image *image)
		{
			Initialize();
			
			if(image)
			{
				Rect frame = Frame();
				frame.width = image->Width();
				frame.height = image->Height();
				
				SetFrame(frame);
				SetImage(image);
			}
		}
		
		ImageView::~ImageView()
		{
			_mesh->Release();
			
			if(_image)
				_image->Release();
		}
		
		
		void ImageView::Initialize()
		{
			static std::once_flag flag;
			std::call_once(flag, []() {
				Shader *shader = new Shader("shader/rn_ImageView");
				ResourcePool::SharedInstance()->AddResource(shader, kRNImageViewShaderResourceName);
			});
			
			Shader *shader = ResourcePool::SharedInstance()->ResourceWithName<Shader>(kRNImageViewShaderResourceName);
			DrawMaterial()->SetShader(shader);
			
			_mesh = BasicMesh()->Retain();
			_image = 0;
		}
		
		
		void ImageView::SetImage(Image *image)
		{
			DrawMaterial()->RemoveTextures();
			
			if(_image)
				_image->Release();
			
			if(image)
			{
				DrawMaterial()->AddTexture(image->Texture());
				_image = image->Retain();
			}
			else
			{
				_image = 0;
			}
		}
		
		bool ImageView::Render(RenderingObject& object)
		{
			object.mesh = _mesh;
			return (_image != 0);
		}
	}
}
