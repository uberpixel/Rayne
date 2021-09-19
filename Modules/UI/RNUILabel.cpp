//
//  RNUILabel.cpp
//  Rayne
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNUILabel.h"
#include "RNUIWindow.h"
#include "RNUIServer.h"

namespace RN
{
	namespace UI
	{
		RNDefineMeta(Label, View)

		Label::Label(const TextAttributes &defaultAttributes) : _attributedText(nullptr), _defaultAttributes(defaultAttributes), _additionalLineHeight(0.0f), _shadowColor(Color::ClearColor()), _verticalAlignment(TextVerticalAlignmentTop), _labelDepthMode(DepthMode::GreaterOrEqual)
		{
			
		}
		Label::~Label()
		{
			
		}
		
		void Label::SetText(const String *text)
		{
			Lock();
			if(_attributedText && text && text->IsEqual(_attributedText) && !_attributedText->GetAttributesAtIndex(0))
			{
				Unlock();
				return;
			}
			if(!text) text = RNCSTR("");
			
			SafeRelease(_attributedText);
			_attributedText = new AttributedString(text);
			_needsMeshUpdate = true;
			Unlock();
		}
	
		void Label::SetAttributedText(AttributedString *text)
		{
			Lock();
			SafeRelease(_attributedText);
			_attributedText = text;
			SafeRetain(_attributedText);
			_needsMeshUpdate = true;
			Unlock();
		}
	
		void Label::SetDefaultAttributes(const TextAttributes &attributes)
		{
			Lock();
			_defaultAttributes = attributes;
			_needsMeshUpdate = true;
			Unlock();
		}
	
		void Label::SetTextColor(const Color &color)
		{
			Lock();
			if(color == _defaultAttributes.GetColor())
			{
				Unlock();
				return;
			}
			
			_defaultAttributes.SetColor(color);
			_needsMeshUpdate = true;
			Unlock();
		}
	
		void Label::SetVerticalAlignment(TextVerticalAlignment alignment)
		{
			Lock();
			_verticalAlignment = alignment;
			_needsMeshUpdate = true;
			Unlock();
		}
    
        void Label::SetAdditionalLineHeight(float lineHeight)
        {
			Lock();
			_additionalLineHeight = lineHeight;
			_needsMeshUpdate = true;
			Unlock();
        }
		
		void Label::SetShadowColor(Color color)
		{
			Lock();
			_shadowColor = color;
			
			Model *model = GetModel();
			if(model)
			{
				Material *shadowMaterial = model->GetLODStage(0)->GetMaterialAtIndex(2);
				shadowMaterial->SetDiffuseColor(_shadowColor);
				shadowMaterial->SetSkipRendering(_shadowColor.a < k::EpsilonFloat);
			}
			Unlock();
		}
		
		void Label::SetShadowOffset(Vector2 offset)
		{
			Lock();
			_shadowOffset = offset;
			
			Model *model = GetModel();
			if(model)
			{
				model->GetLODStage(0)->GetMaterialAtIndex(1)->SetCustomShaderUniform(RNCSTR("uiOffset"), Value::WithVector2(Vector2(_shadowOffset.x, -_shadowOffset.y)));
			}
			Unlock();
		}
		
		void Label::SetTextDepthMode(DepthMode depthMode)
		{
			Lock();
			_labelDepthMode = depthMode;
			RN::Model *model = GetModel();
			if(model)
			{
				Material *material = model->GetLODStage(0)->GetMaterialAtIndex(1);
				material->SetDepthMode(_labelDepthMode);
				
				material = model->GetLODStage(0)->GetMaterialAtIndex(2);
				material->SetDepthMode(_labelDepthMode);
			}
			Unlock();
		}
		
		Vector2 Label::GetTextSize()
		{
			Lock();
			if(!_attributedText || _attributedText->GetLength() == 0)
			{
				Unlock();
				return Vector2();
			}
			
			Array *characters = (new Array(_attributedText->GetLength()))->Autorelease();
			Array *spacings = (new Array(_attributedText->GetLength()))->Autorelease();
			
			std::vector<int> linebreaks;
			std::vector<float> lineascent;
			std::vector<float> linedescent;
			std::vector<float> lineoffset;
			
			float currentWidth = 0.0f;
			float lastWordWidth = 0.0f;
			
			float totalHeight = 0.0f;
			float maxWidth = 0.0f;
			
			float maxAscent = 0.0f;
			float lastWordMaxAscent = 0.0f;
			float tempMaxAscent = 0.0f;
			float maxDescent = 0.0f;
			float lastWordMaxDescent = 0.0f;
			float tempMaxDescent = 0.0f;
			float maxLineOffset = 0.0f;
			float lastWordMaxLineOffset = 0.0f;
			float tempMaxLineOffset = 0.0f;
			
			int lastWhiteSpaceIndex = -1;
			CharacterSet *whiteSpaces = CharacterSet::WithWhitespaces();
			bool hasOnlyWhitespaces = true;
			for(int i = 0; i < _attributedText->GetLength(); i++)
			{
				int currentCodepoint = _attributedText->GetCharacterAtIndex(i);
				int nextCodepoint = i < _attributedText->GetLength()-1? _attributedText->GetCharacterAtIndex(i+1) : -1;

				const TextAttributes *currentAttributes = _attributedText->GetAttributesAtIndex(i);
				if(!currentAttributes) currentAttributes = &_defaultAttributes;
				
				Font *currentFont = currentAttributes->GetFont();
				
				float scaleFactor = currentAttributes->GetFontSize() / currentAttributes->GetFont()->GetHeight();
				float offset = currentFont->GetOffsetForNextCharacter(currentCodepoint, nextCodepoint) * scaleFactor + currentAttributes->GetKerning();
				
				float characterAscent = currentFont->GetAscent() * scaleFactor;
				float characterDescent = -currentFont->GetDescent() * scaleFactor;
				float characterLineOffset = currentFont->GetLineOffset() * scaleFactor;
				maxAscent = std::max(maxAscent, characterAscent);
				tempMaxAscent = std::max(tempMaxAscent, characterAscent);
				maxDescent = std::max(maxDescent, characterDescent);
				tempMaxDescent = std::max(tempMaxDescent, characterDescent);
				maxLineOffset = std::max(maxLineOffset, characterLineOffset);
				tempMaxLineOffset = std::max(tempMaxLineOffset, characterLineOffset);
				
				if(whiteSpaces->CharacterIsMember(currentCodepoint))
				{
					lastWhiteSpaceIndex = i;
					
					lastWordWidth = currentWidth;
					
					lastWordMaxAscent = maxAscent;
					tempMaxAscent = 0.0f;
					lastWordMaxDescent = maxDescent;
					tempMaxDescent = 0.0f;
					lastWordMaxLineOffset = maxLineOffset;
					tempMaxLineOffset = 0.0f;
					
					if(currentCodepoint > 0)
					{
						//TODO: To adjsut this correctly, the previous characters attributes are needed...
						float previousOffset = currentFont->GetOffsetForNextCharacter(currentCodepoint-1, currentCodepoint) * scaleFactor + currentAttributes->GetKerning();
						float correctedOffset = currentFont->GetOffsetForNextCharacter(currentCodepoint-1, -1) * scaleFactor + currentAttributes->GetKerning();
						lastWordWidth -= previousOffset - correctedOffset;
					}
				}
				else if(currentCodepoint != 10) //Is neither whitespace nor linebreak
				{
					hasOnlyWhitespaces = false;
				}
				
				if(currentCodepoint == 10)
				{
					totalHeight += maxAscent + maxDescent + maxLineOffset + _additionalLineHeight;
					if(currentWidth > maxWidth) maxWidth = currentWidth;
					
					linebreaks.push_back(i);
					lineascent.push_back(maxAscent);
					linedescent.push_back(maxDescent);
					lineoffset.push_back(maxLineOffset + _additionalLineHeight);
					maxAscent = 0.0f;
					maxDescent = 0.0f;
					maxLineOffset = 0.0f;
					currentWidth = 0.0f;
					offset = 0.0f;
				}
				
				if(GetBounds().width > 0.0f && currentWidth + offset > GetBounds().width && currentAttributes->GetWrapMode() != TextWrapModeNone)
				{
					Range lastWhitespaceRange;
					lastWhitespaceRange.length = 0;
					if(currentAttributes->GetWrapMode() == TextWrapModeWord && lastWhiteSpaceIndex != -1 && (linebreaks.size() == 0 || lastWhiteSpaceIndex > linebreaks.back()))
					{
						totalHeight += maxAscent + maxDescent + maxLineOffset + _additionalLineHeight;
						if(lastWordWidth > maxWidth) maxWidth = lastWordWidth;
						
						linebreaks.push_back(lastWhiteSpaceIndex);
						lineascent.push_back(lastWordMaxAscent);
						linedescent.push_back(lastWordMaxDescent);
						lineoffset.push_back(lastWordMaxLineOffset + _additionalLineHeight);
						currentWidth -= lastWordWidth;
						maxAscent = tempMaxAscent;
						tempMaxAscent = 0.0f;
						maxDescent = tempMaxDescent;
						tempMaxDescent = 0.0f;
						maxLineOffset = tempMaxLineOffset;
						tempMaxLineOffset = 0.0f;
						
						if(lastWhiteSpaceIndex != i)
						{
							currentWidth -= spacings->GetObjectAtIndex<RN::Number>(lastWhiteSpaceIndex)->GetFloatValue();
							spacings->ReplaceObjectAtIndex(lastWhiteSpaceIndex, RN::Number::WithFloat(0.0f));
						}
						else
						{
							offset = 0.0f;
						}
					}
					else
					{
						totalHeight += maxAscent + maxDescent + maxLineOffset + _additionalLineHeight;
						if(currentWidth > maxWidth) maxWidth = currentWidth;
						
						currentWidth = 0.0f;
						linebreaks.push_back(i);
						
						lineascent.push_back(maxAscent);
						linedescent.push_back(maxDescent);
						lineoffset.push_back(maxLineOffset + _additionalLineHeight);
						maxAscent = 0.0f;
						maxDescent = 0.0f;
						maxLineOffset = 0.0f;
					}
				}
				
				currentWidth += offset;
				spacings->AddObject(RN::Number::WithFloat(offset));
			}
			
			if(hasOnlyWhitespaces)
			{
				Unlock();
				return Vector2();
			}
			
			totalHeight += maxAscent;// + maxDescent;
			if(currentWidth > maxWidth) maxWidth = currentWidth;
			lineascent.push_back(maxAscent);
			linedescent.push_back(maxDescent);
			lineoffset.push_back(maxLineOffset + _additionalLineHeight);

			Unlock();
			return Vector2(maxWidth, totalHeight);
		}
	
	
		void Label::UpdateModel()
		{
			View::UpdateModel();
			
			Lock();
			if(!_attributedText || _attributedText->GetLength() == 0)
			{
				Model *model = GetModel();
				if(model->GetLODStage(0)->GetCount() > 1)
				{
					Material *textMaterial = model->GetLODStage(0)->GetMaterialAtIndex(2);
					textMaterial->SetSkipRendering(true);
					Material *shadowMaterial = model->GetLODStage(0)->GetMaterialAtIndex(1);
					shadowMaterial->SetSkipRendering(true);
				}
				Unlock();
				return;
			}
					
			uint32 numberOfVertices = 0;
			uint32 numberOfIndices = 0;
			
			Array *characters = (new Array(_attributedText->GetLength()))->Autorelease();
			Array *spacings = (new Array(_attributedText->GetLength()))->Autorelease();
			
			std::vector<int> linebreaks;
			std::vector<float> linewidth;
			std::vector<float> lineascent;
			std::vector<float> linedescent;
			std::vector<float> lineoffset;
			
			float currentWidth = 0.0f;
			float lastWordWidth = 0.0f;
			
			float totalHeight = 0.0f;
			
			float maxAscent = 0.0f;
			float lastWordMaxAscent = 0.0f;
			float tempMaxAscent = 0.0f;
			float maxDescent = 0.0f;
			float lastWordMaxDescent = 0.0f;
			float tempMaxDescent = 0.0f;
			float maxLineOffset = 0.0f;
			float lastWordMaxLineOffset = 0.0f;
			float tempMaxLineOffset = 0.0f;
			
			int lastWhiteSpaceIndex = -1;
			CharacterSet *whiteSpaces = CharacterSet::WithWhitespaces();
			bool hasOnlyWhitespaces = true;
			for(int i = 0; i < _attributedText->GetLength(); i++)
			{
				int currentCodepoint = _attributedText->GetCharacterAtIndex(i);
				int nextCodepoint = i < _attributedText->GetLength()-1? _attributedText->GetCharacterAtIndex(i+1) : -1;

				const TextAttributes *currentAttributes = _attributedText->GetAttributesAtIndex(i);
				if(!currentAttributes) currentAttributes = &_defaultAttributes;
				
				Font *currentFont = currentAttributes->GetFont();
				
				float scaleFactor = currentAttributes->GetFontSize() / currentAttributes->GetFont()->GetHeight();
				float offset = currentFont->GetOffsetForNextCharacter(currentCodepoint, nextCodepoint) * scaleFactor + currentAttributes->GetKerning();
				
				float characterAscent = currentFont->GetAscent() * scaleFactor;
				float characterDescent = -currentFont->GetDescent() * scaleFactor;
				float characterLineOffset = currentFont->GetLineOffset() * scaleFactor;
				maxAscent = std::max(maxAscent, characterAscent);
				tempMaxAscent = std::max(tempMaxAscent, characterAscent);
				maxDescent = std::max(maxDescent, characterDescent);
				tempMaxDescent = std::max(tempMaxDescent, characterDescent);
				maxLineOffset = std::max(maxLineOffset, characterLineOffset);
				tempMaxLineOffset = std::max(tempMaxLineOffset, characterLineOffset);
				
				if(whiteSpaces->CharacterIsMember(currentCodepoint))
				{
					lastWhiteSpaceIndex = i;
					
					lastWordWidth = currentWidth;
					
					lastWordMaxAscent = maxAscent;
					tempMaxAscent = 0.0f;
					lastWordMaxDescent = maxDescent;
					tempMaxDescent = 0.0f;
					lastWordMaxLineOffset = maxLineOffset;
					tempMaxLineOffset = 0.0f;
					
					if(currentCodepoint > 0)
					{
						//TODO: To adjsut this correctly, the previous characters attributes are needed...
						float previousOffset = currentFont->GetOffsetForNextCharacter(currentCodepoint-1, currentCodepoint) * scaleFactor + currentAttributes->GetKerning();
						float correctedOffset = currentFont->GetOffsetForNextCharacter(currentCodepoint-1, -1) * scaleFactor + currentAttributes->GetKerning();
						lastWordWidth -= previousOffset - correctedOffset;
					}
				}
				else if(currentCodepoint != 10) //Is neither whitespace nor linebreak
				{
					hasOnlyWhitespaces = false;
				}
				
				if(currentCodepoint == 10)
				{
					totalHeight += maxAscent + maxDescent + maxLineOffset + _additionalLineHeight;
					
					linebreaks.push_back(i);
					linewidth.push_back(currentWidth);
					lineascent.push_back(maxAscent);
					linedescent.push_back(maxDescent);
					lineoffset.push_back(maxLineOffset + _additionalLineHeight);
					maxAscent = 0.0f;
					maxDescent = 0.0f;
					maxLineOffset = 0.0f;
					currentWidth = 0.0f;
					offset = 0.0f;
				}
				
				Mesh *mesh = currentFont->GetMeshForCharacter(currentCodepoint);
				if(mesh)
				{
					characters->AddObject(mesh);
					
					numberOfVertices += mesh->GetVerticesCount();
					numberOfIndices += mesh->GetIndicesCount();
				}
				else
				{
					characters->AddObject(RN::Null::GetNull());
				}
				
				if(GetBounds().width > 0.0f && currentWidth + offset > GetBounds().width && currentAttributes->GetWrapMode() != TextWrapModeNone)
				{
					Range lastWhitespaceRange;
					lastWhitespaceRange.length = 0;
					if(currentAttributes->GetWrapMode() == TextWrapModeWord && lastWhiteSpaceIndex != -1 && (linebreaks.size() == 0 || lastWhiteSpaceIndex > linebreaks.back()))
					{
						totalHeight += maxAscent + maxDescent + maxLineOffset + _additionalLineHeight;
						
						linebreaks.push_back(lastWhiteSpaceIndex);
						linewidth.push_back(lastWordWidth);
						lineascent.push_back(lastWordMaxAscent);
						linedescent.push_back(lastWordMaxDescent);
						lineoffset.push_back(lastWordMaxLineOffset + _additionalLineHeight);
						currentWidth -= lastWordWidth;
						maxAscent = tempMaxAscent;
						tempMaxAscent = 0.0f;
						maxDescent = tempMaxDescent;
						tempMaxDescent = 0.0f;
						maxLineOffset = tempMaxLineOffset;
						tempMaxLineOffset = 0.0f;
						
						if(lastWhiteSpaceIndex != i)
						{
							currentWidth -= spacings->GetObjectAtIndex<RN::Number>(lastWhiteSpaceIndex)->GetFloatValue();
							spacings->ReplaceObjectAtIndex(lastWhiteSpaceIndex, RN::Number::WithFloat(0.0f));
						}
						else
						{
							offset = 0.0f;
						}
					}
					else
					{
						totalHeight += maxAscent + maxDescent + maxLineOffset + _additionalLineHeight;
						
						linewidth.push_back(currentWidth);
						currentWidth = 0.0f;
						linebreaks.push_back(i);
						
						lineascent.push_back(maxAscent);
						linedescent.push_back(maxDescent);
						lineoffset.push_back(maxLineOffset + _additionalLineHeight);
						maxAscent = 0.0f;
						maxDescent = 0.0f;
						maxLineOffset = 0.0f;
					}
				}
				
				currentWidth += offset;
				spacings->AddObject(RN::Number::WithFloat(offset));
			}
			
			if(hasOnlyWhitespaces)
			{
				Unlock();
				return;
			}
			
			totalHeight += maxAscent;// + maxDescent;
			linewidth.push_back(currentWidth);
			lineascent.push_back(maxAscent);
			linedescent.push_back(maxDescent);
			lineoffset.push_back(maxLineOffset + _additionalLineHeight);

			float *vertexPositionBuffer = new float[numberOfVertices * 2];
			float *vertexUVBuffer = new float[numberOfVertices * 3];
			float *vertexColorBuffer = new float[numberOfVertices * 4];
			
			RN::uint32 *indexBuffer = new RN::uint32[numberOfIndices];
			
			RN::uint32 vertexOffset = 0;
			RN::uint32 indexIndexOffset = 0;
			RN::uint32 indexOffset = 0;
			
			RN::uint32 linebreakIndex = 0;
			
			float characterPositionX = 0.0f;
			float characterPositionY = -lineascent[0] + linedescent[0];
			
			if(_verticalAlignment == TextVerticalAlignmentCenter)
			{
				characterPositionY -= GetBounds().height * 0.5f;
				characterPositionY += totalHeight * 0.5f;
				characterPositionY -= linedescent[0] * 0.5f;
			}
			else if(_verticalAlignment == TextVerticalAlignmentBottom)
			{
				characterPositionY -= GetBounds().height;
				characterPositionY += totalHeight;
			}
			
			const TextAttributes *initialAttributes = _attributedText->GetAttributesAtIndex(0);
			if(!initialAttributes) initialAttributes = &_defaultAttributes;
			
			if(initialAttributes->GetAlignment() == TextAlignmentRight)
				characterPositionX = GetBounds().width - linewidth[linebreakIndex];
			else if(initialAttributes->GetAlignment() == TextAlignmentCenter)
				characterPositionX = (GetBounds().width - linewidth[linebreakIndex]) * 0.5f;
			
			characters->Enumerate<RN::Mesh>([&](RN::Mesh *mesh, size_t index, bool &stop){
				if(index > 0) characterPositionX += spacings->GetObjectAtIndex<RN::Number>(index-1)->GetFloatValue();
				
				const TextAttributes *currentAttributes = _attributedText->GetAttributesAtIndex(index);
				if(!currentAttributes) currentAttributes = &_defaultAttributes;
				float scaleFactor = currentAttributes->GetFontSize() / currentAttributes->GetFont()->GetHeight();
				
				if(linebreakIndex < linebreaks.size() && linebreaks[linebreakIndex] == index)
				{
					characterPositionY -= linedescent[linebreakIndex];
					characterPositionY -= lineoffset[linebreakIndex];
					linebreakIndex += 1;
					characterPositionY -= lineascent[linebreakIndex];
					
					characterPositionX = 0.0f;
					if(currentAttributes->GetAlignment() == TextAlignmentRight)
						characterPositionX = GetBounds().width - linewidth[linebreakIndex];
					else if(currentAttributes->GetAlignment() == TextAlignmentCenter)
						characterPositionX = (GetBounds().width - linewidth[linebreakIndex]) * 0.5f;
					
					return;
				}
				
				if(!mesh->IsKindOfClass(RN::Mesh::GetMetaClass()))
				{
					return;
				}
				
				RN::Mesh::Chunk chunk = mesh->GetChunk();
				for(size_t i = 0; i < mesh->GetVerticesCount(); i++)
				{
					RN::Vector2 vertexPosition = *chunk.GetIteratorAtIndex<RN::Vector2>(RN::Mesh::VertexAttribute::Feature::Vertices, i);
					RN::Vector3 vertexUV = *chunk.GetIteratorAtIndex<RN::Vector3>(RN::Mesh::VertexAttribute::Feature::UVCoords0, i);
					
					RN::uint32 targetIndex = vertexOffset + i;
					
					vertexPositionBuffer[targetIndex * 2 + 0] = vertexPosition.x * scaleFactor + characterPositionX;
					vertexPositionBuffer[targetIndex * 2 + 1] = vertexPosition.y * scaleFactor + characterPositionY;
					
					vertexUVBuffer[targetIndex * 3 + 0] = vertexUV.x;
					vertexUVBuffer[targetIndex * 3 + 1] = vertexUV.y;
					vertexUVBuffer[targetIndex * 3 + 2] = vertexUV.z;
					
					vertexColorBuffer[targetIndex * 4 + 0] = currentAttributes->GetColor().r;
					vertexColorBuffer[targetIndex * 4 + 1] = currentAttributes->GetColor().g;
					vertexColorBuffer[targetIndex * 4 + 2] = currentAttributes->GetColor().b;
					vertexColorBuffer[targetIndex * 4 + 3] = currentAttributes->GetColor().a;
				}
				
				for(size_t i = 0; i < mesh->GetIndicesCount(); i++)
				{
					indexBuffer[indexIndexOffset + i] = *chunk.GetIteratorAtIndex<RN::uint32>(RN::Mesh::VertexAttribute::Feature::Indices, i) + indexOffset;
				}
				
				vertexOffset += mesh->GetVerticesCount();
				indexOffset += mesh->GetVerticesCount();
				indexIndexOffset += mesh->GetIndicesCount();
			});
			
			std::vector<RN::Mesh::VertexAttribute> meshVertexAttributes;
			meshVertexAttributes.emplace_back(RN::Mesh::VertexAttribute::Feature::Indices, RN::PrimitiveType::Uint32);
			meshVertexAttributes.emplace_back(RN::Mesh::VertexAttribute::Feature::Vertices, RN::PrimitiveType::Vector2);
			meshVertexAttributes.emplace_back(RN::Mesh::VertexAttribute::Feature::UVCoords1, RN::PrimitiveType::Vector3);
			meshVertexAttributes.emplace_back(RN::Mesh::VertexAttribute::Feature::Color0, RN::PrimitiveType::Vector4);
			
			RN::Mesh *textMesh = new RN::Mesh(meshVertexAttributes, numberOfVertices, numberOfIndices);
			textMesh->BeginChanges();
			
			textMesh->SetElementData(RN::Mesh::VertexAttribute::Feature::Vertices, vertexPositionBuffer);
			textMesh->SetElementData(RN::Mesh::VertexAttribute::Feature::UVCoords1, vertexUVBuffer);
			textMesh->SetElementData(RN::Mesh::VertexAttribute::Feature::Color0, vertexColorBuffer);
			textMesh->SetElementData(RN::Mesh::VertexAttribute::Feature::Indices, indexBuffer);
			
			textMesh->EndChanges();

			delete[] vertexPositionBuffer;
			delete[] vertexUVBuffer;
			delete[] vertexColorBuffer;
			delete[] indexBuffer;
			
			RN::Model *model = GetModel();
			if(model->GetLODStage(0)->GetCount() == 1)
			{
				RN::Material *material = RN::Material::WithShaders(nullptr, nullptr);
				RN::Shader::Options *shaderOptions = RN::Shader::Options::WithMesh(textMesh);
				shaderOptions->EnableAlpha();
				shaderOptions->AddDefine(RNCSTR("RN_UI"), RNCSTR("1"));
				material->SetAlphaToCoverage(false);
				material->SetDepthWriteEnabled(false);
				material->SetDepthMode(_labelDepthMode);
				material->SetCullMode(CullMode::None);
				material->SetBlendOperation(BlendOperation::Add, BlendOperation::Add);
				material->SetBlendFactorSource(BlendFactor::SourceAlpha, BlendFactor::SourceAlpha);
				material->SetBlendFactorDestination(BlendFactor::OneMinusSourceAlpha, BlendFactor::OneMinusSourceAlpha);
				material->SetDiffuseColor(Color::White());
				material->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions));
				material->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions));
				material->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, RN::Shader::UsageHint::Multiview), RN::Shader::UsageHint::Multiview);
				material->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, RN::Shader::UsageHint::Multiview), RN::Shader::UsageHint::Multiview);

				const Rect &scissorRect = GetScissorRect();
				material->SetCustomShaderUniform(RNCSTR("uiClippingRect"), Value::WithVector4(Vector4(scissorRect.GetLeft(), scissorRect.GetRight(), scissorRect.GetTop(), scissorRect.GetBottom())));
				
				RN::Shader::Options *shadowShaderOptions = RN::Shader::Options::WithNone();
				shadowShaderOptions->AddDefine(RNCSTR("RN_UV1"), RNCSTR("1"));
				shadowShaderOptions->AddDefine(RNCSTR("RN_UI"), RNCSTR("1"));
				shadowShaderOptions->EnableAlpha();
				
				Material *shadowMaterial = material->Copy();
				shadowMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shadowShaderOptions));
				shadowMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shadowShaderOptions));
				shadowMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, RN::Shader::UsageHint::Multiview), RN::Shader::UsageHint::Multiview);
				shadowMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, RN::Shader::UsageHint::Multiview), RN::Shader::UsageHint::Multiview);
				shadowMaterial->SetCustomShaderUniform(RNCSTR("uiOffset"), Value::WithVector2(Vector2(_shadowOffset.x, -_shadowOffset.y)));
				shadowMaterial->SetDiffuseColor(_shadowColor);
				
				material->SetCustomShaderUniform(RNCSTR("uiOffset"), Value::WithVector2(Vector2(0.0f, 0.0f)));
				
				model->GetLODStage(0)->AddMesh(textMesh, shadowMaterial);
				model->GetLODStage(0)->AddMesh(textMesh->Autorelease(), material);
				
				model->Retain();
				SetModel(model);
				model->Release();
			}
			else
			{
				model->GetLODStage(0)->ReplaceMesh(textMesh, 1);
				model->GetLODStage(0)->ReplaceMesh(textMesh->Autorelease(), 2);
			}
			
			Material *textMaterial = model->GetLODStage(0)->GetMaterialAtIndex(2);
			textMaterial->SetSkipRendering(false);
			Material *shadowMaterial = model->GetLODStage(0)->GetMaterialAtIndex(1);
			shadowMaterial->SetSkipRendering(_shadowColor.a < k::EpsilonFloat);
			
			model->CalculateBoundingVolumes();
			SetBoundingBox(model->GetBoundingBox());
			
			Unlock();
		}
	}
}
