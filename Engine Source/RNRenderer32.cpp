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
#include "RNLightManager.h"

//TODO: Cleanup!!! (same defines are in RNLightManager.cpp)
#define kRNLightManagerLightListOffsetCountIndex 0
#define kRNLightManagerLightListIndicesIndex 1
#define kRNLightManagerLightListPointDataIndex 2
#define kRNLightManagerLightListSpotDataIndex 3

namespace RN
{
	Renderer32::Renderer32()
	{
		// Setup framebuffer copy stuff
		_copyVertices[0] = Vector4(-1.0f, -1.0f, 0.0f, 0.0f);
		_copyVertices[1] = Vector4(1.0f, -1.0f,  1.0f, 0.0f);
		_copyVertices[2] = Vector4(-1.0f, 1.0f,  0.0f, 1.0f);
		_copyVertices[3] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		
		gl::GenVertexArrays(1, &_copyVAO);
		gl::BindVertexArray(_copyVAO);
		
		gl::GenBuffers(1, &_copyVBO);
		
		gl::BindBuffer(GL_ARRAY_BUFFER, _copyVBO);
		gl::BufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), _copyVertices, GL_STREAM_DRAW);
		
		gl::BindVertexArray(0);
		
		_maxLightFastPath = 10;
		
		RN_CHECKOPENGL();
	}
	
	Renderer32::~Renderer32()
	{
		gl::DeleteBuffers(1, &_copyVBO);
		gl::DeleteVertexArrays(1, &_copyVAO);
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
		
		gl::Viewport(x, y, width, height);
		
		gl::BufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), nullptr, GL_STREAM_DRAW);
		gl::BufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), _copyVertices, GL_STREAM_DRAW);
	}
	
	void Renderer32::FlushCamera(Camera *camera, Shader *drawShader)
	{
		Renderer::FlushCamera(camera, drawShader);
		
		BindVAO(_copyVAO);
		gl::BindBuffer(GL_ARRAY_BUFFER, _copyVBO);
		
		AdjustDrawBuffer(camera, nullptr);
		
		gl::EnableVertexAttribArray(_currentProgram->attPosition);
		gl::VertexAttribPointer(_currentProgram->attPosition,  2, GL_FLOAT, GL_FALSE, 16, (const void *)0);
		
		gl::EnableVertexAttribArray(_currentProgram->attTexcoord0);
		gl::VertexAttribPointer(_currentProgram->attTexcoord0, 2, GL_FLOAT, GL_FALSE, 16, (const void *)8);
		
		
		gl::DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		gl::DisableVertexAttribArray(_currentProgram->attPosition);
		gl::DisableVertexAttribArray(_currentProgram->attTexcoord0);
	}
	
	void Renderer32::DrawCameraStage(Camera *camera, Camera *stage)
	{
		Renderer::DrawCameraStage(camera, stage);
		
		BindVAO(_copyVAO);
		gl::BindBuffer(GL_ARRAY_BUFFER, _copyVBO);
		
		AdjustDrawBuffer(camera, stage);
		
		gl::EnableVertexAttribArray(_currentProgram->attPosition);
		gl::VertexAttribPointer(_currentProgram->attPosition,  2, GL_FLOAT, GL_FALSE, 16, (const void *)0);
		
		gl::EnableVertexAttribArray(_currentProgram->attTexcoord0);
		gl::VertexAttribPointer(_currentProgram->attTexcoord0, 2, GL_FLOAT, GL_FALSE, 16, (const void *)8);
		
		gl::DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		gl::DisableVertexAttribArray(_currentProgram->attPosition);
		gl::DisableVertexAttribArray(_currentProgram->attTexcoord0);
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

			//TODO: Cleanup!!!
			LightManager *lightManager = camera->lightManager;
			int lightPointCount = 100;
			int lightSpotCount = 0;
			int lightDirectionalCount = 0;
			if(lightManager != nullptr)
			{
				lightManager->CreateLightLists(camera);
				// Create the light lists for the camera
//				lightPointCount = lightManager->CreatePointLightList(camera);
//				lightSpotCount = lightManager->CreateSpotLightList(camera);
				lightDirectionalCount = lightManager->CreateDirectionalLightList(camera);
			}
			
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
					
					//TODO: Cleanup!!!
					if(lightManager != nullptr)
					{
						if(lightManager->_lightDirectionalDepth.size() > 0)
							programTypes |= ShaderProgram::TypeDirectionalShadows;
						if(lightManager->_lightPointDepth.size() > 0)
							programTypes |= ShaderProgram::TypePointShadows;
						if(lightManager->_lightSpotDepth.size() > 0)
							programTypes |= ShaderProgram::TypeSpotShadows;
					}
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
					//TODO: Cleanup!!!
					if(lightManager != nullptr)
					{
						// Light data
						gl::Uniform1i(program->lightPointCount, lightPointCount);
						gl::Uniform4fv(program->lightPointPosition, lightPointCount, (float*)lightManager->_lightPointPosition.data());
						gl::Uniform4fv(program->lightPointColor, lightPointCount, (float*)lightManager->_lightPointColor.data());
						
						gl::Uniform1i(program->lightSpotCount, lightSpotCount);
						gl::Uniform4fv(program->lightSpotPosition, lightSpotCount, (float*)lightManager->_lightSpotPosition.data());
						gl::Uniform4fv(program->lightSpotDirection, lightSpotCount, (float*)lightManager->_lightSpotDirection.data());
						gl::Uniform4fv(program->lightSpotColor, lightSpotCount, (float*)lightManager->_lightSpotColor.data());
						
						gl::Uniform1i(program->lightDirectionalCount, lightDirectionalCount);
						gl::Uniform3fv(program->lightDirectionalDirection, lightDirectionalCount, (float*)lightManager->_lightDirectionalDirection.data());
						gl::Uniform4fv(program->lightDirectionalColor, lightDirectionalCount, (float*)lightManager->_lightDirectionalColor.data());
						
						float *data = reinterpret_cast<float *>(lightManager->_lightDirectionalMatrix.data());
						gl::UniformMatrix4fv(program->lightDirectionalMatrix, (GLuint)lightManager->_lightDirectionalMatrix.size(), GL_FALSE, data);
						
						if(lightPointCount >= _maxLightFastPath || lightSpotCount >= _maxLightFastPath)
						{
							if(program->lightTileSize != -1)
							{
								Rect rect = camera->GetFrame();
								int tilesHeight  = ceil(rect.height / camera->GetLightTiles().y);
								int tilesDepth = ceil(camera->clipfar/camera->GetLightTiles().z);
								
								Vector2 lightTilesSize;
								lightTilesSize.x = camera->GetLightTiles().x * _scaleFactor;
								lightTilesSize.y = camera->GetLightTiles().y * _scaleFactor;
								Vector2 lightTilesCount = Vector2(tilesHeight, tilesDepth);
								
								gl::Uniform4f(program->lightTileSize, lightTilesSize.x, lightTilesSize.y, lightTilesCount.x, lightTilesCount.y);
							}
						}
					}
						
					if(program->discardThreshold != -1)
					{
						float threshold = material->discardThreshold;
						
						if(surfaceMaterial && !(material->override & Material::OverrideDiscardThreshold))
							threshold = surfaceMaterial->discardThreshold;
						
						gl::Uniform1f(program->discardThreshold, threshold);
					}
				}
				
				if(changedShader || changedMaterial)
				{
					//TODO: Cleanup!!!
					if(lightManager != nullptr)
					{
						if(program->lightDirectionalDepth != -1 && lightManager->_lightDirectionalDepth.size() > 0)
						{
							uint32 textureUnit = BindTexture(lightManager->_lightDirectionalDepth.front());
							gl::Uniform1i(program->lightDirectionalDepth, textureUnit);
						}
						
						if(lightManager->_lightPointDepth.size() > 0)
						{
							const std::vector<GLuint>& lightPointDepthLocations = program->lightPointDepthLocations;
							
							if(lightPointDepthLocations.size() > 0)
							{
								size_t textureCount = std::min(lightPointDepthLocations.size(), lightManager->_lightPointDepth.size());
								
								
								uint32 lastpointdepth = 0;
								for(size_t i=0; i<textureCount; i++)
								{
									GLint location = lightPointDepthLocations[i];
									lastpointdepth = BindTexture(lightManager->_lightPointDepth[i]);
									gl::Uniform1i(location, lastpointdepth);
								}
								
								for(size_t i = textureCount; i < lightPointDepthLocations.size(); i++)
								{
									GLint location = lightPointDepthLocations[i];
									gl::Uniform1i(location, lastpointdepth);
								}
							}
						}
						
						if(lightManager->_lightSpotDepth.size() > 0)
						{
							const std::vector<GLuint>& lightSpotDepthLocations = program->lightSpotDepthLocations;
							
							if(lightSpotDepthLocations.size() > 0)
							{
								size_t textureCount = std::min(lightSpotDepthLocations.size(), lightManager->_lightSpotDepth.size());
								
								
								uint32 lastspotdepth = 0;
								for(size_t i=0; i<textureCount; i++)
								{
									GLint location = lightSpotDepthLocations[i];
									lastspotdepth = BindTexture(lightManager->_lightSpotDepth[i]);
									gl::Uniform1i(location, lastspotdepth);
								}
								
								for(size_t i = textureCount; i < lightSpotDepthLocations.size(); i++)
								{
									GLint location = lightSpotDepthLocations[i];
									gl::Uniform1i(location, lastspotdepth);
								}
							}
						}
						
						if(lightPointCount >= _maxLightFastPath || lightSpotCount >= _maxLightFastPath)
						{
							if(program->lightListIndices != -1)
							{
								uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, lightManager->_lightTextures[kRNLightManagerLightListIndicesIndex]);
								gl::Uniform1i(program->lightListIndices, textureUnit);
							}
							
							if(program->lightListOffsetCount != -1)
							{
								uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, lightManager->_lightTextures[kRNLightManagerLightListOffsetCountIndex]);
								gl::Uniform1i(program->lightListOffsetCount, textureUnit);
							}
							
							if(program->lightListDataPoint != -1)
							{
								uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, lightManager->_lightTextures[kRNLightManagerLightListPointDataIndex]);
								gl::Uniform1i(program->lightListDataPoint, textureUnit);
							}
							
							if(program->lightListDataSpot != -1)
							{
								uint32 textureUnit = BindTexture(GL_TEXTURE_BUFFER, lightManager->_lightTextures[kRNLightManagerLightListSpotDataIndex]);
								gl::Uniform1i(program->lightListDataSpot, textureUnit);
							}
						}
					}
					
					gl::Uniform4fv(program->ambient, 1, &material->ambient->r);
					gl::Uniform4fv(program->diffuse, 1, &material->diffuse->r);
					gl::Uniform4fv(program->emissive, 1, &material->emissive->r);
					gl::Uniform4fv(program->specular, 1, &material->specular->r);
					
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
					gl::UniformMatrix4fv(program->matBones, object.skeleton->GetBoneCount(), GL_FALSE, data);
				}
				
				gl::UniformMatrix4fv(program->matModel, 1, GL_FALSE, transform.m);
				gl::UniformMatrix4fv(program->matModelInverse, 1, GL_FALSE, inverseTransform.m);
				
				if(object.rotation)
				{
					if(program->matNormal != -1)
						gl::UniformMatrix4fv(program->matNormal, 1, GL_FALSE, object.rotation->GetRotationMatrix().m);
					
					if(program->matNormalInverse != -1)
						gl::UniformMatrix4fv(program->matNormalInverse, 1, GL_FALSE, object.rotation->GetRotationMatrix().GetInverse().m);
				}
				
				if(program->matViewModel != -1)
				{
					Matrix viewModel = viewMatrix * transform;
					gl::UniformMatrix4fv(program->matViewModel, 1, GL_FALSE, viewModel.m);
				}
				
				if(program->matViewModelInverse != -1)
				{
					Matrix viewModel = inverseViewMatrix * inverseTransform;
					gl::UniformMatrix4fv(program->matViewModelInverse, 1, GL_FALSE, viewModel.m);
				}
				
				if(program->matProjViewModel != -1)
				{
					Matrix projViewModel = projectionViewMatrix * transform;
					gl::UniformMatrix4fv(program->matProjViewModel, 1, GL_FALSE, projViewModel.m);
				}
				
				if(program->matProjViewModelInverse != -1)
				{
					Matrix projViewModelInverse = inverseProjectionViewMatrix * inverseTransform;
					gl::UniformMatrix4fv(program->matProjViewModelInverse, 1, GL_FALSE, projViewModelInverse.m);
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
			
			gl::DrawElements(mesh->GetMode(), glCount, type, reinterpret_cast<void *>(offset));
		}
		else
		{
			gl::DrawArrays(mesh->GetMode(), 0, glCount);
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
		gl::Uniform1i(_currentProgram->instancingData, textureUnit);
		
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
			
			gl::DrawElementsInstanced(mesh->GetMode(), (GLsizei)descriptor->elementCount, type, 0, (GLsizei)object.count);
		}
		else
		{
			descriptor = mesh->GetDescriptor(kMeshFeatureVertices);
			gl::DrawArraysInstanced(mesh->GetMode(), 0, (GLsizei)descriptor->elementCount, (GLsizei)object.count);
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
