//
//  RNUIImageView.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUIImageView.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(ImageView, View)

		ImageView::ImageView() :
			_image(nullptr), _isFirstImage(true)
		{}
		ImageView::~ImageView()
		{
			SafeRelease(_image);
		}

		void ImageView::SetImage(Texture *image)
		{
			if(_image == image && (!_isFirstImage || !_image)) return;
			
			bool needsMaterialChange = (_image != nullptr) != (image != nullptr);
			
			SafeRelease(_image);
			_image = SafeRetain(image);
			
			Model *model = GetModel();
			if(model)
			{
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(0);
				material->RemoveAllTextures();
				
				if(_image) material->AddTexture(_image);
				
				if(needsMaterialChange || _isFirstImage)
				{
					Shader::Options *shaderOptions = Shader::Options::WithNone();
					shaderOptions->EnableAlpha();
					shaderOptions->AddDefine(RNCSTR("RN_UI"), RNCSTR("1"));
					shaderOptions->AddDefine(RNCSTR("RN_COLOR"), RNCSTR("1"));
					if(_image) shaderOptions->AddDefine(RNCSTR("RN_UV0"), RNCSTR("1"));
					
					material->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions));
					material->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions));
				}
			}
		}
	
		void ImageView::UpdateModel()
		{
			View::UpdateModel();
			SafeRetain(_image);
			SetImage(_image);
			SafeRelease(_image);
			_isFirstImage = false;
		}
	}
}
