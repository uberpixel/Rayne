//
//  RNUIImageView.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIImageView.h"
#include "RNResourceCoordinator.h"

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
				Rect frame = GetFrame();
				frame.width  = image->GetWidth();
				frame.height = image->GetHeight();
				
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
			
			_material->Release();
		}
		
		
		void ImageView::Initialize()
		{
			SetInteractionEnabled(false);
			SetBackgroundColor(Color::ClearColor());
			
			_mesh  = nullptr;
			_material = BasicMaterial(ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyUIImageShader, nullptr));
			_material->Retain();
			
			_image   = nullptr;
			_isDirty = true;
			_scaleMode = ScaleMode::AxisIndependently;
		}

		Vector2 ImageView::GetFittingImageSize(const Vector2& tsize)
		{			
			Vector2 size = tsize;
			
			float width = static_cast<float>(_image->GetWidth());
			float height = static_cast<float>(_image->GetHeight());
			
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
		
		
		Vector2 ImageView::GetSizeThatFits()
		{
			if(!_image)
				return Vector2(0.0f, 0.0f);
			
			return Vector2(_image->GetWidth(), _image->GetHeight());
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
					_material->RemoveTextures();
					_material->AddTexture(_image->GetTexture());
					
					Vector2 size = std::move(GetFittingImageSize(GetFrame().Size()));
					Vector2 offset = (GetFrame().Size() - size) * 0.5f;
					
					_mesh = _image->GetFittingMesh(size, offset)->Retain();
				}
				
				_isDirty = false;
			}
		}
		
		void ImageView::Draw(Renderer *renderer)
		{
			View::Draw(renderer);
			
			if(_mesh)
			{
				RenderingObject object;
				PopulateRenderingObject(object);
				
				object.mesh = _mesh;
				object.material = _material;
				
				renderer->RenderObject(object);
			}
		}
	}
}
