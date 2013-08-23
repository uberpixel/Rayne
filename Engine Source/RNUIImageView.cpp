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
		RNDeclareMeta(ImageView)
		
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
				frame.width  = image->Width();
				frame.height = image->Height();
				
				SetFrame(frame);
				SetImage(image);
			}
		}
		
		ImageView::~ImageView()
		{
			if(_mesh)
				_mesh->Release();
			
			if(_image)
				_image->Release();
		}
		
		
		void ImageView::Initialize()
		{
			Shader *shader = ResourcePool::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyUIImageShader);
			DrawMaterial()->SetShader(shader);
			SetInteractionEnabled(false);
			
			_mesh  = nullptr;
			_image = 0;
			_isDirty = true;
			_scaleMode = ScaleMode::AxisIndependently;
		}

		Vector2 ImageView::FittingImageSize(const Vector2& tsize)
		{			
			Vector2 size = tsize;
			
			float width = static_cast<float>(_image->Width());
			float height = static_cast<float>(_image->Height());
			
			switch(_scaleMode)
			{
				case ScaleMode::ProportionallyDown:
					size.x = std::min(size.x, width);
					size.y = std::min(size.y, height);
					
				case ScaleMode::ProportionallyUpOrDown:
					if(size.x > size.y)
					{
						float factor = size.x / width;
						size.y = height * factor;
					}
					else
					{
						float factor = size.y / height;
						size.x = width * factor;
					}
					break;
					
				case ScaleMode::None:
					size = Vector2(width, height);
					break;
					
				default:
					break;
			}
			
			return size;
		}
		
		
		Vector2 ImageView::SizeThatFits()
		{
			if(!_image)
				return Vector2(0.0f, 0.0f);
			
			return Vector2(_image->Width(), _image->Height());
		}
		
		void ImageView::SetFrame(const Rect& frame)
		{
			View::SetFrame(frame);
			_isDirty = true;
		}
		
		void ImageView::SetScaleMode(ScaleMode mode)
		{
			_scaleMode = mode;
			_isDirty = true;
		}
		
		void ImageView::SetImage(Image *image)
		{			
			if(_image)
			{
				_image->Release();
				_image = nullptr;
			}
			
			_image = image ? image->Retain() : nullptr;
			_isDirty = true;
		}
		
		void ImageView::Update()
		{
			View::Update();
			
			if(_isDirty)
			{
				if(_mesh)
				{
					_mesh->Release();
					_mesh = nullptr;
				}
				
				if(_image)
				{
					DrawMaterial()->RemoveTextures();
					DrawMaterial()->AddTexture(_image->Texture());
					
					Vector2 size = std::move(FittingImageSize(Frame().Size()));
					Vector2 offset = (Frame().Size() - size) * 0.5f;
					
					_mesh = _image->FittingMesh(size, offset)->Retain();
				}
				
				_isDirty = false;
			}
		}
		
		bool ImageView::Render(RenderingObject& object)
		{
			object.mesh = _mesh;
			return (_image != 0);
		}
	}
}
