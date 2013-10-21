//
//  RNRenderer32.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderer32.h"
#include "RNThreadPool.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"

namespace RN
{
	Renderer32::Renderer32()
	{
		// Setup framebuffer copy stuff
		_copyVertices[0] = Vector4(-1.0f, -1.0f, 0.0f, 0.0f);
		_copyVertices[1] = Vector4(1.0f, -1.0f,  1.0f, 0.0f);
		_copyVertices[2] = Vector4(-1.0f, 1.0f,  0.0f, 1.0f);
		_copyVertices[3] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		
		glGenVertexArrays(1, &_copyVAO);
		glBindVertexArray(_copyVAO);
		
		glGenBuffers(1, &_copyVBO);
		
		glBindBuffer(GL_ARRAY_BUFFER, _copyVBO);
		glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), _copyVertices, GL_STREAM_DRAW);
		
		glBindVertexArray(0);
		
		_maxLightFastPath = 10;
		
		RN_CHECKOPENGL();
	}
	
	Renderer32::~Renderer32()
	{
		glDeleteBuffers(1, &_copyVBO);
		glDeleteVertexArrays(1, &_copyVAO);
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Camera rendering
	// ---------------------
	
	void Renderer32::AdjustDrawBuffer(Camera *camera, Camera *target)
	{
		const Vector2& size = camera->GetStorage()->GetSize();
		const Rect& offset = camera->GetRenderingFrame();
		const Rect& frame = camera->GetFrame();
		
		Vector4 atlas = Vector4(offset.x / size.x, offset.y / size.y, (offset.x + offset.width) / size.x, (offset.y + offset.height) / size.y);
		
		_copyVertices[0] = Vector4(-1.0f, -1.0f, atlas.x, atlas.y);
		_copyVertices[1] = Vector4(1.0f, -1.0f,  atlas.z, atlas.y);
		_copyVertices[2] = Vector4(-1.0f, 1.0f,  atlas.x, atlas.w);
		_copyVertices[3] = Vector4(1.0f, 1.0f,   atlas.z, atlas.w);
		
		Camera::BlitMode blitMode = camera->GetBlitMode();
		bool stretchHorizontal = (blitMode == Camera::BlitMode::StretchedHorizontal || blitMode == Camera::BlitMode::Stretched);
		bool stretchVertical   = (blitMode == Camera::BlitMode::StretchedVertical   || blitMode == Camera::BlitMode::Stretched);
		
		float x, y;
		float width, height;
		
		
		if(!target)
		{
			x = !stretchHorizontal ? ceilf((frame.x * _scaleFactor) * _defaultWidthFactor) : 0.0f;
			y = !stretchVertical   ? ceilf((frame.y * _scaleFactor) * _defaultHeightFactor) : 0.0f;
			
			width  = !stretchHorizontal ? ceilf((frame.width  * _scaleFactor) * _defaultWidthFactor) : _defaultWidth * _scaleFactor;
			height = !stretchVertical   ? ceilf((frame.height * _scaleFactor) * _defaultHeightFactor) : _defaultHeight * _scaleFactor;
		}
		else
		{
			Rect tframe(target->GetRenderingFrame());
			const Rect& targetFrame = target->GetFrame();
			
			if(stretchHorizontal)
			{
				tframe.x = targetFrame.x;
				tframe.width = targetFrame.width;
			}
			
			if(stretchVertical)
			{
				tframe.y = targetFrame.y;
				tframe.height = targetFrame.height;
			}
			
			x = ceilf(tframe.x * _scaleFactor);
			y = ceilf(tframe.y * _scaleFactor);
			
			width  = ceilf(tframe.width  * _scaleFactor);
			height = ceilf(tframe.height * _scaleFactor);
		}
		
		glViewport(x, y, width, height);
		
		glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), nullptr, GL_STREAM_DRAW);
		glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), _copyVertices, GL_STREAM_DRAW);
	}
	
	void Renderer32::FlushCamera(Camera *camera, Shader *drawShader)
	{
		Renderer::FlushCamera(camera, drawShader);
		
		BindVAO(_copyVAO);
		glBindBuffer(GL_ARRAY_BUFFER, _copyVBO);
		
		AdjustDrawBuffer(camera, nullptr);
		
		glEnableVertexAttribArray(_currentProgram->attPosition);
		glVertexAttribPointer(_currentProgram->attPosition,  2, GL_FLOAT, GL_FALSE, 16, (const void *)0);
		
		glEnableVertexAttribArray(_currentProgram->attTexcoord0);
		glVertexAttribPointer(_currentProgram->attTexcoord0, 2, GL_FLOAT, GL_FALSE, 16, (const void *)8);
		
		
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		glDisableVertexAttribArray(_currentProgram->attPosition);
		glDisableVertexAttribArray(_currentProgram->attTexcoord0);
	}
	
	void Renderer32::DrawCameraStage(Camera *camera, Camera *stage)
	{
		Renderer::DrawCameraStage(camera, stage);
		
		BindVAO(_copyVAO);
		glBindBuffer(GL_ARRAY_BUFFER, _copyVBO);
		
		AdjustDrawBuffer(camera, stage);
		
		glEnableVertexAttribArray(_currentProgram->attPosition);
		glVertexAttribPointer(_currentProgram->attPosition,  2, GL_FLOAT, GL_FALSE, 16, (const void *)0);
		
		glEnableVertexAttribArray(_currentProgram->attTexcoord0);
		glVertexAttribPointer(_currentProgram->attTexcoord0, 2, GL_FLOAT, GL_FALSE, 16, (const void *)8);
		
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		glDisableVertexAttribArray(_currentProgram->attPosition);
		glDisableVertexAttribArray(_currentProgram->attTexcoord0);
	}
	
	// ---------------------
	// MARK: -
	// MARK: Rendering
	// ---------------------
	
	void Renderer32::DrawCamera(Camera *camera, Camera *source, uint32 skyCubeMeshes)
	{
		Renderer::DrawCamera(camera, source, skyCubeMeshes);
		
		bool changedCamera = true;
		bool changedShader;
		bool changedMaterial;
		
		SetDepthWriteEnabled(true);
		
		camera->Bind();
		camera->PrepareForRendering(this);
		
		if(_currentMaterial)
			SetDepthWriteEnabled(_currentMaterial->depthwrite);
		
		bool wantsFog = _currentCamera->usefog;
		bool wantsClipPlane = _currentCamera->useclipplane;
		
		Matrix identityMatrix;
		
		if(!source && !(camera->GetFlags() & Camera::FlagNoRender))
		{
			Material *surfaceMaterial = camera->GetMaterial();

			// Create the light lists for the camera
			int lightPointCount = CreatePointLightList(camera);
			int lightSpotCount  = CreateSpotLightList(camera);
			int lightDirectionalCount = CreateDirectionalLightList(camera);
			
			_renderedLights += lightPointCount + lightSpotCount + lightDirectionalCount;
			
			// Update the shader
			const Matrix& projectionMatrix = camera->projectionMatrix;
			const Matrix& inverseProjectionMatrix = camera->inverseProjectionMatrix;
			
			const Matrix& viewMatrix = camera->viewMatrix;
			const Matrix& inverseViewMatrix = camera->inverseViewMatrix;
			
			Matrix projectionViewMatrix = projectionMatrix * viewMatrix;
			Matrix inverseProjectionViewMatrix = inverseProjectionMatrix * inverseViewMatrix;
			
			size_t objectsCount = _frame.size();
			size_t i = (camera->GetFlags() & Camera::FlagNoSky) ? skyCubeMeshes : 0;
			
			for(; i<objectsCount; i++)
			{
				RenderingObject& object = _frame[i];
				if(object.prepare)
					object.prepare(this, object);
				
				SetScissorEnabled(object.flags & RenderingObject::ScissorTest);
				
				if(_scissorTest)
					SetScissorRect(object.scissorRect);
				
				Mesh     *mesh = object.mesh;
				Material *material = object.material;
				Shader   *shader = surfaceMaterial ? surfaceMaterial->GetShader() : material->GetShader();
				
				Matrix& transform = object.transform ? *object.transform : identityMatrix;
				Matrix inverseTransform = transform.GetInverse();
				
				// Check if we can use instancing here
				bool wantsInstancing = (object.type == RenderingObject::Type::Instanced);
				if(RN_EXPECT_FALSE(wantsInstancing))
				{
					if(!shader->SupportsProgramOfType(ShaderProgram::TypeInstanced))
						continue;
				}
				
				// Grab the correct shader program
				uint32 programTypes = 0;
				ShaderProgram *program = 0;
				
				bool wantsDiscard = material->discard;
				if(surfaceMaterial && !(material->override & Material::OverrideDiscard))
					wantsDiscard = surfaceMaterial->discard;
				
				if(object.skeleton && shader->SupportsProgramOfType(ShaderProgram::TypeAnimated))
					programTypes |= ShaderProgram::TypeAnimated;
				
				bool wantsLighting = material->lighting;
				if(surfaceMaterial)
					wantsLighting = surfaceMaterial->lighting;

				if(wantsLighting && shader->SupportsProgramOfType(ShaderProgram::TypeLighting))
				{
					programTypes |= ShaderProgram::TypeLighting;
					
					if(_lightDirectionalDepth.size() > 0)
						programTypes |= ShaderProgram::TypeDirectionalShadows;
					if(_lightPointDepth.size() > 0)
						programTypes |= ShaderProgram::TypePointShadows;
					if(_lightSpotDepth.size() > 0)
						programTypes |= ShaderProgram::TypeSpotShadows;
				}
				
				if(wantsFog && shader->SupportsProgramOfType(ShaderProgram::TypeFog))
					programTypes |= ShaderProgram::TypeFog;
				
				if(wantsClipPlane && shader->SupportsProgramOfType(ShaderProgram::TypeClipPlane))
					programTypes |= ShaderProgram::TypeClipPlane;
				
				if(wantsInstancing)
					programTypes |= ShaderProgram::TypeInstanced;
				
				if(wantsDiscard && shader->SupportsProgramOfType(ShaderProgram::TypeDiscard))
					programTypes |= ShaderProgram::TypeDiscard;
				
				// Set lighting defines
				std::vector<ShaderDefine> defines;
				if(lightDirectionalCount > 0)
					defines.emplace_back(ShaderDefine("RN_DIRECTIONAL_LIGHTS", lightDirectionalCount));
				
				if(lightPointCount < _maxLightFastPath)
					defines.emplace_back(ShaderDefine("RN_POINT_LIGHTS_FASTPATH", lightPointCount));
				
				if(lightSpotCount < _maxLightFastPath)
					defines.emplace_back(ShaderDefine("RN_SPOT_LIGHTS_FASTPATH", lightSpotCount));
				
				if(surfaceMaterial)
				{
					program = shader->GetProgramWithLookup(surfaceMaterial->GetLookup() + material->GetLookup() + ShaderLookup(programTypes) + ShaderLookup(defines));
				}
				else
				{
					program = shader->GetProgramWithLookup(material->GetLookup() + ShaderLookup(programTypes) + ShaderLookup(defines));
				}
				
				RN_ASSERT(program, "");
				
				changedShader   = (_currentProgram != program);
				changedMaterial = (_currentMaterial != material);
				
				BindMaterial(material, program);
				
				// Update the shader data
				if(changedShader || changedCamera)
				{
					UpdateShaderData();
					changedCamera = false;
					changedShader = true;
				}
				
				if(changedShader)
				{
					// Light data
					glUniform1i(program->lightPointCount, lightPointCount);
					glUniform4fv(program->lightPointPosition, lightPointCount, (float*)_lightPointPosition.data());
					glUniform4fv(program->lightPointColor, lightPointCount, (float*)_lightPointColor.data());
					
					glUniform1i(program->lightSpotCount, lightSpotCount);
					glUniform4fv(program->lightSpotPosition, lightSpotCount, (float*)_lightSpotPosition.data());
					glUniform4fv(program->lightSpotDirection, lightSpotCount, (float*)_lightSpotDirection.data());
					glUniform4fv(program->lightSpotColor, lightSpotCount, (float*)_lightSpotColor.data());
					
					glUniform1i(program->lightDirectionalCount, lightDirectionalCount);
					glUniform3fv(program->lightDirectionalDirection, lightDirectionalCount, (float*)_lightDirectionalDirection.data());
					glUniform4fv(program->lightDirectionalColor, lightDirectionalCount, (float*)_lightDirectionalColor.data());
					
					float *data = reinterpret_cast<float *>(_lightDirectionalMatrix.data());
					glUniformMatrix4fv(program->lightDirectionalMatrix, (GLuint)_lightDirectionalMatrix.size(), GL_FALSE, data);
					
					if(lightPointCount >= _maxLightFastPath || lightSpotCount >= _maxLightFastPath)
					{
						if(program->lightTileSize != -1)
						{
							Rect rect = camera->GetFrame();
							int tilesWidth  = ceil(rect.width / camera->GetLightTiles().x);
							int tilesDepth = 15;//camera->GetLightTiles().z;
							
							Vector2 lightTilesSize;
							lightTilesSize.x = camera->GetLightTiles().x * _scaleFactor;
							lightTilesSize.y = camera->GetLightTiles().y * _scaleFactor;
							Vector2 lightTilesCount = Vector2(tilesWidth, tilesDepth);
							
							glUniform4f(program->lightTileSize, lightTilesSize.x, lightTilesSize.y, lightTilesCount.x, lightTilesCount.y);
						}
					}
					
					if(program->discardThreshold != -1)
					{
						float threshold = material->discardThreshold;
						
						if(surfaceMaterial && !(material->override & Material::OverrideDiscardThreshold))
							threshold = surfaceMaterial->discardThreshold;
						
						glUniform1f(program->discardThreshold, threshold);
					}
				}
				
				if(changedShader || changedMaterial)
				{
					if(program->lightDirectionalDepth != -1 && _lightDirectionalDepth.size() > 0)
					{
						uint32 textureUnit = BindTexture(_lightDirectionalDepth.front());
						glUniform1i(program->lightDirectionalDepth, textureUnit);
					}
					
					if(_lightPointDepth.size() > 0)
					{
						const std::vector<GLuint>& lightPointDepthLocations = program->lightPointDepthLocations;
						
						if(lightPointDepthLocations.size() > 0)
						{
							size_t textureCount = std::min(lightPointDepthLocations.size(), _lightPointDepth.size());
							
							
							uint32 lastpointdepth = 0;
							for(size_t i=0; i<textureCount; i++)
							{
								GLint location = lightPointDepthLocations[i];
								lastpointdepth = BindTexture(_lightPointDepth[i]);
								glUniform1i(location, lastpointdepth);
							}
							
							for(size_t i = textureCount; i < lightPointDepthLocations.size(); i++)
							{
								GLint location = lightPointDepthLocations[i];
								glUniform1i(location, lastpointdepth);
							}
						}
					}
					
					if(_lightSpotDepth.size() > 0)
					{
						const std::vector<GLuint>& lightSpotDepthLocations = program->lightSpotDepthLocations;
						
						if(lightSpotDepthLocations.size() > 0)
						{
							size_t textureCount = std::min(lightSpotDepthLocations.size(), _lightSpotDepth.size());
							
							
							uint32 lastspotdepth = 0;
							for(size_t i=0; i<textureCount; i++)
							{
								GLint location = lightSpotDepthLocations[i];
								lastspotdepth = BindTexture(_lightSpotDepth[i]);
								glUniform1i(location, lastspotdepth);
							}
							
							for(size_t i = textureCount; i < lightSpotDepthLocations.size(); i++)
							{
								GLint location = lightSpotDepthLocations[i];
								glUniform1i(location, lastspotdepth);
							}
						}
					}
					
					if(lightPointCount >= _maxLightFastPath || lightSpotCount >= _maxLightFastPath)
					{
						// Point lights
						if(program->lightPointList != -1)
						{
							uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, _lightPointTextures[kRNRendererPointLightListIndicesIndex]);
							glUniform1i(program->lightPointList, textureUnit);
						}
						
						if(program->lightPointListOffset != -1)
						{
							uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, _lightPointTextures[kRNRendererPointLightListOffsetIndex]);
							glUniform1i(program->lightPointListOffset, textureUnit);
						}
						
						if(program->lightPointListData != -1)
						{
							uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, _lightPointTextures[kRNRendererPointLightListDataIndex]);
							glUniform1i(program->lightPointListData, textureUnit);
						}
						
						// Spot lights
						if(program->lightSpotList != -1)
						{
							uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, _lightSpotTextures[kRNRendererSpotLightListIndicesIndex]);
							glUniform1i(program->lightSpotList, textureUnit);
						}
						
						if(program->lightSpotListOffset!= -1)
						{
							uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, _lightSpotTextures[kRNRendererSpotLightListOffsetIndex]);
							glUniform1i(program->lightSpotListOffset, textureUnit);
						}
						
						if(program->lightSpotListData != -1)
						{
							uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, _lightSpotTextures[kRNRendererSpotLightListDataIndex]);
							glUniform1i(program->lightSpotListData, textureUnit);
						}
					}
					
					glUniform4fv(program->ambient, 1, &material->ambient->r);
					glUniform4fv(program->diffuse, 1, &material->diffuse->r);
					glUniform4fv(program->emissive, 1, &material->emissive->r);
					glUniform4fv(program->specular, 1, &material->specular->r);
					
					material->ApplyUniforms(program);
				}
				
				if(RN_EXPECT_FALSE(wantsInstancing))
				{
					DrawMeshInstanced(object);
					continue;
				}
				
				// More updates
				if(object.skeleton)
				{
					const float *data = reinterpret_cast<const float *>(object.skeleton->GetMatrices().data());
					glUniformMatrix4fv(program->matBones, object.skeleton->GetBoneCount(), GL_FALSE, data);
				}
				
				glUniformMatrix4fv(program->matModel, 1, GL_FALSE, transform.m);
				glUniformMatrix4fv(program->matModelInverse, 1, GL_FALSE, inverseTransform.m);
				
				if(object.rotation)
				{
					if(program->matNormal != -1)
						glUniformMatrix4fv(program->matNormal, 1, GL_FALSE, object.rotation->GetRotationMatrix().m);
					
					if(program->matNormalInverse != -1)
						glUniformMatrix4fv(program->matNormalInverse, 1, GL_FALSE, object.rotation->GetRotationMatrix().GetInverse().m);
				}
				
				if(program->matViewModel != -1)
				{
					Matrix viewModel = viewMatrix * transform;
					glUniformMatrix4fv(program->matViewModel, 1, GL_FALSE, viewModel.m);
				}
				
				if(program->matViewModelInverse != -1)
				{
					Matrix viewModel = inverseViewMatrix * inverseTransform;
					glUniformMatrix4fv(program->matViewModelInverse, 1, GL_FALSE, viewModel.m);
				}
				
				if(program->matProjViewModel != -1)
				{
					Matrix projViewModel = projectionViewMatrix * transform;
					glUniformMatrix4fv(program->matProjViewModel, 1, GL_FALSE, projViewModel.m);
				}
				
				if(program->matProjViewModelInverse != -1)
				{
					Matrix projViewModelInverse = inverseProjectionViewMatrix * inverseTransform;
					glUniformMatrix4fv(program->matProjViewModelInverse, 1, GL_FALSE, projViewModelInverse.m);
				}
				
				if(RN_EXPECT_FALSE(object.type == RenderingObject::Type::Custom))
				{
					object.callback(this, object);
					continue;
				}
				
				DrawMesh(mesh, object.offset, object.count);
			}
		}
		else if(source)
		{
			DrawCameraStage(source, camera);
		}
		
		camera->Unbind();
	}
	
	void Renderer32::DrawMesh(Mesh *mesh, uint32 offset, uint32 count)
	{
		bool usesIndices = mesh->SupportsFeature(kMeshFeatureIndices);
		MeshDescriptor *descriptor = usesIndices ? mesh->GetDescriptor(kMeshFeatureIndices) : mesh->GetDescriptor(kMeshFeatureVertices);
		
		BindVAO(std::tuple<ShaderProgram *, Mesh *>(_currentProgram, mesh));
		
		GLsizei glCount = static_cast<GLsizei>(descriptor->elementCount);
		if(count != 0)
			glCount = std::min(glCount, static_cast<GLsizei>(count));
		
		if(usesIndices)
		{
			GLenum type;
			switch(descriptor->elementSize)
			{
				case 1:
					type = GL_UNSIGNED_BYTE;
					break;
					
				case 2:
					type = GL_UNSIGNED_SHORT;
					break;
					
				case 4:
					type = GL_UNSIGNED_INT;
					break;
					
				default:
					throw Exception(Exception::Type::InconsistencyException, "");
					break;
			}
			
			glDrawElements(mesh->GetMode(), glCount, type, reinterpret_cast<void *>(offset));
		}
		else
		{
			glDrawArrays(mesh->GetMode(), 0, glCount);
		}
		
		_renderedVertices += glCount;
		BindVAO(0);
	}
	
	void Renderer32::DrawMeshInstanced(const RenderingObject& object)
	{
		Mesh *mesh = object.mesh;
		MeshDescriptor *descriptor = mesh->GetDescriptor(kMeshFeatureIndices);
		
		BindVAO(std::tuple<ShaderProgram *, Mesh *>(_currentProgram, mesh));
		RN_ASSERT(_currentProgram->instancingData != -1, "");
		
		uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, object.instancingData);
		glUniform1i(_currentProgram->instancingData, textureUnit);
		
		if(descriptor)
		{
			GLenum type;
			switch(descriptor->elementSize)
			{
				case 1:
					type = GL_UNSIGNED_BYTE;
					break;
					
				case 2:
					type = GL_UNSIGNED_SHORT;
					break;
					
				case 4:
					type = GL_UNSIGNED_INT;
					break;
					
				default:
					throw Exception(Exception::Type::InconsistencyException, "");
					break;
			}
			
			glDrawElementsInstanced(mesh->GetMode(), (GLsizei)descriptor->elementCount, type, 0, (GLsizei)object.count);
		}
		else
		{
			descriptor = mesh->GetDescriptor(kMeshFeatureVertices);
			glDrawArraysInstanced(mesh->GetMode(), 0, (GLsizei)descriptor->elementCount, (GLsizei)object.count);
		}
		
		_renderedVertices += descriptor->elementCount * object.count;
		BindVAO(0);
	}
	
	// ---------------------
	// MARK: -
	// MARK: Rendering
	// ---------------------
	
	void Renderer32::SetMaxLightFastPathCount(uint32 maxLights)
	{
		_maxLightFastPath = maxLights;
	}
}
