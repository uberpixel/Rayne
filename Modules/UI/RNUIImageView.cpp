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
			_image(nullptr)
		{
			
		}
	
		ImageView::~ImageView()
		{
			SafeRelease(_image);
		}

		void ImageView::SetImage(Texture *image)
		{
			if(_image == image) return;
			
			SafeRelease(_image);
			_image = SafeRetain(image);
			
			Model *model = GetModel();
			if(model)
			{
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(1);
				material->RemoveAllTextures();
				if(_image) material->AddTexture(_image);
				
				material->SetSkipRendering(_image == nullptr);
			}
		}
	
		void ImageView::UpdateModel()
		{
			View::UpdateModel();
			RN::Model::LODStage *lodStage = GetModel()->GetLODStage(0);
			
			if(lodStage->GetCount() < 2)
			{
				RN::Shader::Options *shaderOptions = RN::Shader::Options::WithNone();
				shaderOptions->EnableAlpha();
				shaderOptions->AddDefine(RNCSTR("RN_UI"), RNCSTR("1"));
				shaderOptions->AddDefine(RNCSTR("RN_UV0"), RNCSTR("1"));
				
				RN::Material *material = RN::Material::WithShaders(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions), Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions));
				material->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, RN::Shader::UsageHint::Multiview), RN::Shader::UsageHint::Multiview);
				material->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, RN::Shader::UsageHint::Multiview), RN::Shader::UsageHint::Multiview);
				material->SetAlphaToCoverage(false);
				material->SetDepthWriteEnabled(false);
				material->SetCullMode(CullMode::None);
				material->SetBlendOperation(BlendOperation::Add, BlendOperation::Add);
				material->SetBlendFactorSource(BlendFactor::SourceAlpha, BlendFactor::SourceAlpha);
				material->SetBlendFactorDestination(BlendFactor::OneMinusSourceAlpha, BlendFactor::OneMinusSourceAlpha);
				material->SetDiffuseColor(Color::White());
				
				const Rect &scissorRect = GetScissorRect();
				material->SetCustomShaderUniform(RNCSTR("uiClippingRect"), Value::WithVector4(Vector4(scissorRect.GetLeft(), scissorRect.GetRight(), scissorRect.GetTop(), scissorRect.GetBottom())));
				material->SetCustomShaderUniform(RNCSTR("uiOffset"), Value::WithVector2(Vector2(0.0f, 0.0f)));
				
				lodStage->AddMesh(lodStage->GetMeshAtIndex(0), material);
				
				if(_image) material->AddTexture(_image);
				material->SetSkipRendering(_image == nullptr);
				
				SetModel(GetModel());
			}
			else
			{
				lodStage->ReplaceMesh(lodStage->GetMeshAtIndex(0), 1);
			}
		}
	}
}
