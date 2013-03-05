//
//  RNRenderingPipeline.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderingPipeline.h"
#include "RNLightEntity.h"
#include "RNKernel.h"

#define kRNRenderingPipelineInstancingCutOff 100

#define kRNRenderingPipelineFeatureLightning 1
#define kRNRenderingPipelineFeatureInstancing 1
#define kRNRenderingPipelineFeatureStages 1
#define kRNRenderingPipelineFeatureSorting 1

namespace RN
{
	RenderingPipeline::RenderingPipeline() :
		PipelineSegment(false)
	{
		// Some general state variables
		_defaultFBO = 0;
		_defaultWidth = _defaultHeight = 0;

		_currentMaterial = 0;
		_currentVAO      = 0;
		_currentShader	 = 0;

		_finishFrame = 0;
		_scaleFactor = Kernel::SharedInstance()->ScaleFactor();

		_cullingEnabled   = false;
		_depthTestEnabled = false;
		_blendingEnabled  = false;
		_depthWrite       = false;

		_cullMode  = GL_CCW;
		_depthFunc = GL_LESS;

		_blendSource      = GL_ONE;
		_blendDestination = GL_ZERO;

		_frameLock = new Mutex();
		
		// Light indices
		_lightindexoffsetSize = 100;
		_lightindicesSize = 500;
		
		_lightindexoffset = (int *)malloc(_lightindexoffsetSize * sizeof(int));
		_lightindices = (int *)malloc(_lightindicesSize * sizeof(int));
		
		_lightTextures[0] = 0;
		_lightBuffers[0] = 0;

		// Setup framebuffer copy stuff
		_copyShader = 0;

		_copyVertices[0] = Vector4(-1.0f, 1.0f, 0.0f, 1.0f);
		_copyVertices[1] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		_copyVertices[2] = Vector4(1.0f, -1.0f, 1.0f, 0.0f);
		_copyVertices[3] = Vector4(-1.0f, -1.0f, 0.0f, 0.0f);

		_copyIndices[0] = 0;
		_copyIndices[1] = 3;
		_copyIndices[2] = 1;
		_copyIndices[3] = 2;
		_copyIndices[4] = 1;
		_copyIndices[5] = 3;

		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, (GLint *)&_maxTextureUnits);
		_textureUnit = 0;
		
		_hasValidFramebuffer = false;
		_initialized = false;

		_numInstancingMatrices = 0;
		_instancingMatrices = 0;
	}

	RenderingPipeline::~RenderingPipeline()
	{
		if(_copyShader)
		{
			_copyShader->Release();

			glDeleteBuffers(1, &_copyVBO);
			glDeleteBuffers(1, &_copyIBO);

			gl::DeleteVertexArrays(1, &_copyVAO);
		}

#if !(RN_PLATFORM_IOS)
		if(_lightTextures[0])
			glDeleteTextures(4, _lightTextures);
		
		if(_lightBuffers[0])
			glDeleteBuffers(4, _lightBuffers);
#endif

		if(_instancingMatrices)
			free(_instancingMatrices);
		
		free(_lightindexoffset);
		free(_lightindices);

		_frameLock->Release();
	}

	void RenderingPipeline::Initialize()
	{
		_copyShader = new Shader("shader/rn_CopyFramebuffer");

		gl::GenVertexArrays(1, &_copyVAO);
		gl::BindVertexArray(_copyVAO);

		glGenBuffers(1, &_copyVBO);
		glGenBuffers(1, &_copyIBO);

		glBindBuffer(GL_ARRAY_BUFFER, _copyVBO);
		glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), _copyVertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _copyIBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLshort), _copyIndices, GL_STATIC_DRAW);

		gl::BindVertexArray(0);
		glGenBuffers(1, &_instancingVBO);

#if !(RN_PLATFORM_IOS)
		glGenTextures(4, _lightTextures);
		glGenBuffers(4, _lightBuffers);

		//indexpos
		glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[0]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[0]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32I, _lightBuffers[0]);

		//indices
		glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[1]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[1]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, _lightBuffers[1]);

		//lightpos
		glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[2]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[2]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _lightBuffers[2]);

		//lightcol
		glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[3]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[3]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, _lightBuffers[3]);
		
		_lightBufferLengths[0] = 0;
		_lightBufferLengths[1] = 0;
		_lightBufferLengths[2] = 0;
#endif
		
		_initialized = true;
	}

	void RenderingPipeline::SetDefaultFBO(GLuint fbo)
	{
		_defaultFBO = fbo;
		_hasValidFramebuffer = false;
	}

	void RenderingPipeline::SetDefaultFrame(uint32 width, uint32 height)
	{
		_defaultWidth  = width;
		_defaultHeight = height;
	}


	// ---------------------
	// MARK: -
	// MARK: Camera flushing
	// ---------------------
	
	void RenderingPipeline::FlushCameras()
	{
#if GL_EXT_debug_marker
		glPushGroupMarkerEXT(0, "Flushing cameras");
#endif

		glBindFramebuffer(GL_FRAMEBUFFER, _defaultFBO);
		glClear(GL_COLOR_BUFFER_BIT);

		glViewport(0, 0, _defaultWidth * _scaleFactor, _defaultHeight * _scaleFactor);

		for(auto iterator=_flushCameras.begin(); iterator!=_flushCameras.end(); iterator++)
		{
			Camera *camera  = *iterator;
			FlushCamera(camera);
		}

#if GL_EXT_debug_marker
		glPopGroupMarkerEXT();
#endif

		_flushCameras.clear();
	}
	
	void RenderingPipeline::FlushCamera(Camera *camera)
	{
		camera->Push();

		if(_depthTestEnabled)
		{
			glDisable(GL_DEPTH_TEST);
			_depthTestEnabled = false;
		}

		if(_currentVAO != _copyVAO)
		{
			gl::BindVertexArray(_copyVAO);
			_currentVAO = _copyVAO;

			glBindBuffer(GL_ARRAY_BUFFER, _copyVBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _copyIBO);
		}

		Shader *shader = _copyShader;
		
		BindShader(_copyShader);
		UpdateShaderWithCamera(_copyShader, camera);

		glEnableVertexAttribArray(shader->vertPosition);
		glVertexAttribPointer(shader->vertPosition,  2, GL_FLOAT, GL_FALSE, 16, (const void *)0);

		glEnableVertexAttribArray(shader->vertTexcoord0);
		glVertexAttribPointer(shader->vertTexcoord0, 2, GL_FLOAT, GL_FALSE, 16, (const void *)8);

		uint32 targetmaps = MIN((uint32)shader->targetmaplocations.Count(), camera->RenderTargets());
		if(targetmaps >= 1)
		{
			Texture *texture = camera->RenderTarget(0);
			GLuint location = shader->targetmaplocations.ObjectAtIndex(0);

			glUniform1i(location, BindTexture(texture));
		}

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		glDisableVertexAttribArray(shader->vertPosition);
		glDisableVertexAttribArray(shader->vertTexcoord0);

		camera->Pop();
	}

	void RenderingPipeline::DrawCameraStage(Camera *camera, Camera *stage)
	{
		stage->Push();

		Material *material = stage->Material();
		Shader *shader = material->Shader();
		
		_currentCamera = stage;
		
		BindMaterial(material);
		BindShader(shader);
		UpdateShaderWithCamera(shader, stage);
		
		if(_currentVAO != _copyVAO)
		{
			gl::BindVertexArray(_copyVAO);
			_currentVAO = _copyVAO;

			glBindBuffer(GL_ARRAY_BUFFER, _copyVBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _copyIBO);
		}

		glEnableVertexAttribArray(shader->vertPosition);
		glVertexAttribPointer(shader->vertPosition,  2, GL_FLOAT, GL_FALSE, 16, (const void *)0);

		glEnableVertexAttribArray(shader->vertTexcoord0);
		glVertexAttribPointer(shader->vertTexcoord0, 2, GL_FLOAT, GL_FALSE, 16, (const void *)8);

		uint32 targetmaps = MIN((uint32)shader->targetmaplocations.Count(), camera->RenderTargets());
		for(uint32 i=0; i<targetmaps; i++)
		{
			Texture *texture = camera->RenderTarget(i);
			GLuint location = shader->targetmaplocations.ObjectAtIndex(i);

			glUniform1i(location, BindTexture(texture));
		}

		if(shader->depthmap != -1)
		{
			Texture *depthmap = camera->Storage()->DepthTarget();
			if(depthmap)
			{
				glUniform1i(shader->depthmap, BindTexture(depthmap));
			}
		}

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		glDisableVertexAttribArray(shader->vertPosition);
		glDisableVertexAttribArray(shader->vertTexcoord0);

		stage->Pop();
	}

	// ---------------------
	// MARK: -
	// MARK: Drawing
	// ---------------------
	
	void RenderingPipeline::DrawGroup(RenderingGroup *group)
	{
		Camera *previous = 0;
		Camera *camera = group->camera;
		machine_uint sortOder = 0;
		
		// Object pre-pass
		Array<Entity> *frame = &group->entities;
		Array<RenderingObject> objects = Array<RenderingObject>(frame->Capacity());
		
		// Skycube
		Model *skycube = camera->SkyCube();
		Matrix camrotationmatrix;
		camrotationmatrix.MakeRotate(camera->Rotation().AccessPast());
		
		if(skycube != 0)
		{
			uint32 meshes = skycube->Meshes();
			
			for(uint32 j=0; j<meshes; j++)
			{
				RenderingObject object;
				object.mesh = skycube->MeshAtIndex(j);
				object.material = skycube->MaterialForMesh(object.mesh);
				object.transform = &camrotationmatrix;
				
				objects.AddObject(object);
			}
		}

		// Unpack the frame
		for(machine_uint i=0; i<frame->Count(); i++)
		{
			Entity *entity = frame->ObjectAtIndex(i);
			Model  *model  = entity->Model();
			
			uint32 meshes = model->Meshes();
			for(uint32 j=0; j<meshes; j++)
			{
				RenderingObject object;
				object.mesh = model->MeshAtIndex(j);
				object.material = model->MaterialForMesh(object.mesh);
				object.transform = entity->PastWorldTransform();

				objects.AddObject(object);
			}
		}

		// Light list
#if kRNRenderingPipelineFeatureLightning
		Vector4 *lightPos = 0;
		Vector3 *lightColor = 0;
		int lightCount = 0;
		
		Rect rect = camera->Frame();
		int tilesWidth  = rect.width / camera->LightTiles().x;
		int tilesHeight = rect.height / camera->LightTiles().y;
		
		Vector2 lightTilesSize = camera->LightTiles() * _scaleFactor;
		Vector2 lightTilesCount = Vector2(tilesWidth, tilesHeight);
		
		CreateLightList(group, camera, &lightPos, &lightColor, &lightCount);
#endif
		
		// Draw all camera stages
		bool changedShader;
		bool changedCamera;
		
		while(camera)
		{
			camera->Bind();
			camera->PrepareForRendering();
			
			_currentCamera = camera;
			changedCamera  = true;

			if(!(camera->CameraFlags() & Camera::FlagDrawTarget))
			{
				Material *surfaceMaterial = camera->Material();
				
#if kRNRenderingPipelineFeatureSorting
				machine_uint bestOrder = surfaceMaterial ? 1 : 2;
				
				if(bestOrder != sortOder)
				{
					// Sort the objects
					objects.SortUsingFunction([&](const RenderingObject& a, const RenderingObject& b) {
						if(surfaceMaterial)
						{
							machine_uint objA = (machine_uint)a.mesh;
							machine_uint objB = (machine_uint)b.mesh;
							
							if(objA > objB)
								return kRNCompareGreaterThan;
							
							if(objB > objA)
								return kRNCompareLessThan;
							
							return kRNCompareEqualTo;
						}
						else
						{
							// Sort by material
							const Material *materialA = a.material;
							const Material *materialB = b.material;
							
							if(materialA->blending != materialB->blending)
							{
								if(!materialB->blending)
									return kRNCompareGreaterThan;
								
								if(!materialA->blending)
									return kRNCompareLessThan;
							}
							
							if(materialA->alphatest != materialB->alphatest)
							{
								if(!materialB->alphatest)
									return kRNCompareGreaterThan;
								
								if(!materialA->alphatest)
									return kRNCompareLessThan;
							}
							
							if(materialA->Shader() > materialB->Shader())
								return kRNCompareGreaterThan;
							
							if(materialB->Shader() > materialA->Shader())
								return kRNCompareLessThan;
							
							// Sort by mesh
							if(a.mesh > b.mesh)
								return kRNCompareGreaterThan;
							
							if(b.mesh > a.mesh)
								return kRNCompareLessThan;
							
							return kRNCompareEqualTo;
						}
					}, 6);
					
					sortOder = bestOrder;
				}
#endif
				
				Matrix& projectionMatrix = camera->projectionMatrix.AccessPast();
				Matrix& inverseProjectionMatrix = camera->inverseProjectionMatrix.AccessPast();
				
				Matrix& viewMatrix = camera->viewMatrix.AccessPast();
				Matrix& inverseViewMatrix = camera->inverseViewMatrix.AccessPast();
				
				Matrix projectionViewMatrix = projectionMatrix * viewMatrix;
				Matrix inverseProjectionViewMatrix = inverseProjectionMatrix * inverseViewMatrix;
				
				uint32 offset;
				uint32 noCheck = 0;

				machine_uint objectsCount = objects.Count();
				for(machine_uint i=0; i<objectsCount; i+=offset)
				{
					offset = 1;
					
					RenderingObject& object = objects.ObjectAtIndex(i);
					
					Mesh *mesh = (Mesh *)object.mesh;
					Material *material = surfaceMaterial ? surfaceMaterial : (Material *)object.material;
					Shader *shader = material->Shader();
					
					Matrix& transform = (Matrix &)*object.transform;
					Matrix inverseTransform = transform.Inverse();

					// Send generic attributes to the shader
					changedShader = (_currentShader != shader->program);
					
					BindShader(shader);
					BindMaterial(material);
					
					if(changedShader || changedCamera)
					{
						UpdateShaderWithCamera(shader, camera);
						changedCamera = false;
					}
					
#if kRNRenderingPipelineFeatureLightning
					if(changedShader)
					{					
						// Light data
						if(shader->lightCount != -1)
							glUniform1i(shader->lightCount, lightCount);

						if(shader->lightPosition != -1 && lightCount > 0)
							glUniform4fv(shader->lightPosition, lightCount, &(lightPos[0].x));

						if(shader->lightColor != -1 && lightCount > 0)
							glUniform3fv(shader->lightColor, lightCount, &(lightColor[0].x));
					
#if !(RN_PLATFORM_IOS)
						if(camera->LightTiles() != 0)
						{
							if(shader->lightListOffset != -1)
							{
								_textureUnit ++;
								_textureUnit %= _maxTextureUnits;
								
								glActiveTexture((GLenum)(GL_TEXTURE0 + _textureUnit));
								glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[0]);
								glUniform1i(shader->lightListOffset, _textureUnit);
							}
							
							if(shader->lightList != -1)
							{
								_textureUnit ++;
								_textureUnit %= _maxTextureUnits;
								
								glActiveTexture((GLenum)(GL_TEXTURE0 + _textureUnit));
								glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[1]);
								glUniform1i(shader->lightList, _textureUnit);
							}
							
							if(shader->lightListPosition != -1)
							{
								_textureUnit ++;
								_textureUnit %= _maxTextureUnits;
								
								glActiveTexture((GLenum)(GL_TEXTURE0 + _textureUnit));
								glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[2]);
								glUniform1i(shader->lightListPosition, _textureUnit);
							}
							
							if(shader->lightListColor != -1)
							{
								_textureUnit ++;
								_textureUnit %= _maxTextureUnits;
								
								glActiveTexture((GLenum)(GL_TEXTURE0 + _textureUnit));
								glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[3]);
								glUniform1i(shader->lightListColor, _textureUnit);
							}
							
							if(shader->lightTileSize != -1)
							{
								glUniform4f(shader->lightTileSize, lightTilesSize.x, lightTilesSize.y, lightTilesCount.x, lightTilesCount.y);
							}
						}
#endif
					}
#endif
	
#if kRNRenderingPipelineFeatureInstancing
					// Check if we can use instancing here
					if(noCheck == 0 && SupportsOpenGLFeature(kOpenGLFeatureInstancing))
					{
						machine_uint end = i + 1;
						offset = 1;

						while(end < objectsCount)
						{
							RenderingObject& temp = objects.ObjectAtIndex(end);
							
							if(temp.mesh != mesh)
								break;

							if(!surfaceMaterial && temp.material != material)
								break;

							offset ++;
							end ++;
						}
						
						if(offset >= kRNRenderingPipelineInstancingCutOff)
						{
							if(material->Shader()->imatModel != -1)
							{
								DrawMeshInstanced(objects, i, offset);
								continue;
							}
						}

						noCheck = offset;
						offset = 1;
					}
					else
					{
						noCheck --;
					}
#endif
					

					// Send the object related uniforms tot he shader
					if(shader->matModel != -1)
						glUniformMatrix4fv(shader->matModel, 1, GL_FALSE, transform.m);

					if(shader->matModelInverse != -1)
						glUniformMatrix4fv(shader->matModelInverse, 1, GL_FALSE, inverseTransform.m);
					
					if(shader->matViewModel != -1)
					{
						Matrix viewModel = viewMatrix * transform;
						glUniformMatrix4fv(shader->matViewModel, 1, GL_FALSE, viewModel.m);
					}

					if(shader->matViewModelInverse != -1)
					{
						Matrix viewModel = inverseViewMatrix * inverseTransform;
						glUniformMatrix4fv(shader->matViewModelInverse, 1, GL_FALSE, viewModel.m);
					}

					
					if(shader->matProjViewModel != -1)
					{
						Matrix projViewModel = projectionViewMatrix * transform;
						glUniformMatrix4fv(shader->matProjViewModel, 1, GL_FALSE, projViewModel.m);
					}

					if(shader->matProjViewModelInverse != -1)
					{
						Matrix projViewModelInverse = inverseProjectionViewMatrix * inverseTransform;
						glUniformMatrix4fv(shader->matProjViewModelInverse, 1, GL_FALSE, projViewModelInverse.m);
					}

					DrawMesh(mesh);
				}
			}
			else
			{
				if(previous)
					DrawCameraStage(previous, camera);
			}

			previous = camera;
			
			camera->Unbind();
			camera = camera->Stage();
			
#if !kRNRenderingPipelineFeatureStages
			camera = 0;
#endif

			if(!camera)
				_flushCameras.push_back(previous);
		}

#if kRNRenderingPipelineFeatureLightning
		delete[] lightColor;
		delete[] lightPos;
#endif
	}

	void RenderingPipeline::DrawMesh(Mesh *mesh)
	{
		mesh->Push();
		
		MeshLODStage *stage = mesh->LODStage(0);
		MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureIndices);
		
		BindVAO(std::tuple<Material *, MeshLODStage *>(_currentMaterial, stage));
		
		GLenum type = (descriptor->elementSize == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
		glDrawElements(GL_TRIANGLES, (GLsizei)descriptor->elementCount, type, 0);
		
		mesh->Pop();
	}
	
	void RenderingPipeline::DrawMeshInstanced(const Array<RenderingObject>& group, machine_uint start, machine_uint count)
	{
		Mesh *mesh = (Mesh *)group[(int)start].mesh;
		MeshLODStage *stage = mesh->LODStage(0);
		MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureIndices);
		Shader *shader = _currentMaterial->Shader();
		
		if(count > _numInstancingMatrices)
		{
			_numInstancingMatrices = (uint32)count;
			
			if(_instancingMatrices)
				free(_instancingMatrices);
			
			_instancingMatrices = (Matrix *)malloc((_numInstancingMatrices * 2) * sizeof(Matrix));
		}
		
		
		mesh->Push();
		
		BindVAO(std::tuple<Material *, MeshLODStage *>(_currentMaterial, stage));
		
		uint32 offset = 0;
		
		if(shader->imatModel != -1)
		{
			for(int i=0; i<4; i++)
			{
				glEnableVertexAttribArray(shader->imatModel + i);
				glVertexAttribPointer(shader->imatModel + i, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 16, (void *)(sizeof(float) * ((i * 4) + offset)));
				gl::VertexAttribDivisor(shader->imatModel + i, 1);
			}
			
			for(machine_uint i=0; i<count; i++)
			{
				RenderingObject& object = group[(int)(start + i)];
				_instancingMatrices[i + offset] = *object.transform;
			}
			
			offset += count;
		}
		
		if(shader->imatModelInverse != -1)
		{
			for(int i=0; i<4; i++)
			{
				glEnableVertexAttribArray(shader->imatModelInverse + i);
				glVertexAttribPointer(shader->imatModelInverse + i, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 16, (void *)(sizeof(float) * ((i * 4) + offset)));
				gl::VertexAttribDivisor(shader->imatModelInverse + i, 1);
			}
			
			for(machine_uint i=0; i<count; i++)
			{
				RenderingObject& object = group[(int)(start + i)];
				_instancingMatrices[i + offset] = object.transform->Inverse();
			}
			
			offset += count;
		}
		
		if(offset == 0)
		{
			mesh->Pop();
			return;
		}
		
		glBindBuffer(GL_ARRAY_BUFFER, _instancingVBO);
		glBufferData(GL_ARRAY_BUFFER, offset * sizeof(Matrix), _instancingMatrices, GL_DYNAMIC_DRAW);
		
		GLenum type = (descriptor->elementSize == 2) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
		gl::DrawElementsInstanced(GL_TRIANGLES, (GLsizei)descriptor->elementCount, type, 0, (GLsizei)count);
		
		if(shader->imatModel != -1)
		{
			for(int i=0; i<4; i++)
				glDisableVertexAttribArray(shader->imatModel + i);
		}
		
		if(shader->imatModelInverse != -1)
		{
			for(int i=0; i<4; i++)
				glDisableVertexAttribArray(shader->imatModelInverse + i);
		}
		
		mesh->Pop();
	}
	
	// ---------------------
	// MARK: -
	// MARK: Additional helper
	// ---------------------
	
	void RenderingPipeline::UpdateShaderWithCamera(Shader *shader, Camera *camera)
	{
		Matrix& projectionMatrix = camera->projectionMatrix.AccessPast();
		Matrix& inverseProjectionMatrix = camera->inverseProjectionMatrix.AccessPast();
		
		Matrix& viewMatrix = camera->viewMatrix.AccessPast();
		Matrix& inverseViewMatrix = camera->inverseViewMatrix.AccessPast();
		
		
		if(shader->frameSize != -1)
			glUniform4f(shader->frameSize, 1.0f/camera->Frame().width/_scaleFactor, 1.0f/camera->Frame().height/_scaleFactor, camera->Frame().width*_scaleFactor, camera->Frame().height*_scaleFactor);
		
		if(shader->clipPlanes != -1)
			glUniform2f(shader->clipPlanes, camera->clipnear, camera->clipfar);
		
		if(shader->matProj != -1)
			glUniformMatrix4fv(shader->matProj, 1, GL_FALSE, projectionMatrix.m);
		
		if(shader->matProjInverse != -1)
			glUniformMatrix4fv(shader->matProjInverse, 1, GL_FALSE, inverseProjectionMatrix.m);
		
		if(shader->matView != -1)
			glUniformMatrix4fv(shader->matView, 1, GL_FALSE, viewMatrix.m);
		
		if(shader->matViewInverse != -1)
			glUniformMatrix4fv(shader->matViewInverse, 1, GL_FALSE, inverseViewMatrix.m);
		
		if(shader->matProjView != -1)
		{
			Matrix projectionViewMatrix = projectionMatrix * viewMatrix;
			glUniformMatrix4fv(shader->matProjView, 1, GL_FALSE, projectionViewMatrix.m);
		}
		
		if(shader->matProjViewInverse != -1)
		{
			Matrix inverseProjectionViewMatrix = inverseProjectionMatrix * inverseViewMatrix;
			glUniformMatrix4fv(shader->matProjViewInverse, 1, GL_FALSE, inverseProjectionViewMatrix.m);
		}
	}
	
	void RenderingPipeline::CreateLightList(RenderingGroup *group, Camera *camera, Vector4 **outLightPos, Vector3 **outLightColor, int *outLightCount)
	{
		Array<LightEntity> *lights = &group->lights;
		
		Vector4 *lightPos = *outLightPos;
		Vector3 *lightColor = *outLightColor;
		machine_uint lightCount = 0;
		
		if(!lightPos)
			lightPos = new Vector4[lights->Count()];
		
		if(!lightColor)
			lightColor = new Vector3[lights->Count()];
		
		for(machine_uint i=0; i<lights->Count(); i++, lightCount++)
		{
			LightEntity *light = lights->ObjectAtIndex(i);
			const Vector3& position = light->Position().AccessPast();
			
			lightPos[lightCount] = Vector4(position.x, position.y, position.z, light->Range().AccessPast());
			lightColor[lightCount] = light->Color().AccessPast();
		}
		
#if !(RN_PLATFORM_IOS)
		size_t lightIndexOffsetCount = 0;
		size_t lightIndicesCount = 0;
		
		Rect rect = camera->Frame();
		int tilesWidth  = rect.width / camera->LightTiles().x;
		int tilesHeight = rect.height / camera->LightTiles().y;
		
		if(camera->DepthTiles() != 0)
		{
			Vector3 corner1 = camera->CamToWorld(Vector3(-1.0f, -1.0f, 1.0f));
			Vector3 corner2 = camera->CamToWorld(Vector3(1.0f, -1.0f, 1.0f));
			Vector3 corner3 = camera->CamToWorld(Vector3(-1.0f, 1.0f, 1.0f));
			
			Vector3 dirx = (corner2-corner1) / tilesWidth;
			Vector3 diry = (corner3-corner1) / tilesHeight;
			
			const Vector3& camPosition = camera->Position().AccessPast();
			
			Plane plleft;
			Plane plright;
			Plane pltop;
			Plane plbottom;
			Plane plfar;
			Plane plnear;
			
			size_t count = lights->Count();
			LightEntity **allLights = lights->Data();
			
			size_t lightindicesSize = tilesWidth * tilesHeight * lights->Count();
			if(lightindicesSize > _lightindicesSize)
			{
				_lightindices = (int *)realloc(_lightindices, lightindicesSize * sizeof(int));
				_lightindicesSize = lightindicesSize;
			}
			
			size_t lightindexoffsetSize = tilesWidth * tilesHeight * 2;
			if(lightindexoffsetSize > _lightindexoffsetSize)
			{
				_lightindexoffset = (int *)realloc(_lightindexoffset, lightindexoffsetSize * sizeof(int));
				_lightindexoffsetSize = lightindexoffsetSize;
			}
			
			for(float y=0.0f; y<tilesHeight; y+=1.0f)
			{
				for(float x=0.0f; x<tilesWidth; x+=1.0f)
				{
					plleft.SetPlane(camPosition, corner1+dirx*x+diry*(y+1.0f), corner1+dirx*x+diry*(y-1.0f));
					plright.SetPlane(camPosition, corner1+dirx*(x+1.0f)+diry*(y+1.0f), corner1+dirx*(x+1.0f)+diry*(y-1.0f));
					pltop.SetPlane(camPosition, corner1+dirx*(x-1.0f)+diry*(y+1.0f), corner1+dirx*(x+1.0f)+diry*(y+1.0f));
					plbottom.SetPlane(camPosition, corner1+dirx*(x-1.0f)+diry*y, corner1+dirx*(x+1.0f)+diry*y);
					
					size_t previous = lightIndicesCount;
					_lightindexoffset[lightIndexOffsetCount ++] = static_cast<int>(previous);
					
					for(size_t i=0; i<count; i++)
					{
						LightEntity *light = allLights[i];
						
						const Vector3& position = light->_position.AccessPast();
						float range = light->_range.AccessPast();
						
#define Distance(plane, op, r) { \
float dot = (position.x * plane._normal.x + position.y * plane._normal.y + position.z * plane._normal.z);\
float distance = dot - plane._d; \
if(distance op r) \
continue; \
}
						Distance(plleft, >, range);
						Distance(plright, <, -range);
						Distance(pltop, <, -range);
						Distance(plbottom, >, range);
#undef Distance
						
						_lightindices[lightIndicesCount ++] = static_cast<int>(i);
					}
					
					_lightindexoffset[lightIndexOffsetCount ++] = static_cast<int>(lightIndicesCount - previous);
				}
			}
			
			
			if(_lightBufferLengths[0] < lightIndexOffsetCount)
			{
				_lightBufferLengths[0] = (uint32)lightIndexOffsetCount;
				
				//indexpos
				glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[0]);
				glBufferData(GL_TEXTURE_BUFFER, lightIndexOffsetCount * sizeof(int), _lightindexoffset, GL_DYNAMIC_DRAW);
			}
			else
			{
				//indexpos
				glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[0]);
				glBufferSubData(GL_TEXTURE_BUFFER, 0, lightIndexOffsetCount * sizeof(int), _lightindexoffset);
			}
			
			if(_lightBufferLengths[1] < lightIndicesCount)
			{
				_lightBufferLengths[1] = (uint32)lightIndicesCount;
				
				//indices
				glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[1]);
				glBufferData(GL_TEXTURE_BUFFER, lightIndicesCount * sizeof(int), _lightindices, GL_DYNAMIC_DRAW);
			}
			else
			{
				//indices
				glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[1]);
				glBufferSubData(GL_TEXTURE_BUFFER, 0, lightIndicesCount * sizeof(int), _lightindices);
			}
			
			if(_lightBufferLengths[2] < lightCount)
			{
				_lightBufferLengths[2] = (uint32)lightCount;
				
				//lightpos
				glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[2]);
				glBufferData(GL_TEXTURE_BUFFER, lightCount * 4 * sizeof(float), lightPos, GL_DYNAMIC_DRAW);
				
				//lightcol
				glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[3]);
				glBufferData(GL_TEXTURE_BUFFER, lightCount * 3 * sizeof(float), lightColor, GL_DYNAMIC_DRAW);
			}
			else
			{
				//lightpos
				glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[2]);
				glBufferSubData(GL_TEXTURE_BUFFER, 0, lightCount * 4 * sizeof(float), lightPos);
				
				//lightcol
				glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[3]);
				glBufferSubData(GL_TEXTURE_BUFFER, 0, lightCount * 3 * sizeof(float), lightColor);
			}
			
			glBindBuffer(GL_TEXTURE_BUFFER, 0);
		}
#endif
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: Binding
	// ---------------------
	
	uint32 RenderingPipeline::BindTexture(Texture *texture)
	{
		_textureUnit ++;
		_textureUnit %= _maxTextureUnits;
		
		glActiveTexture((GLenum)(GL_TEXTURE0 + _textureUnit));
		glBindTexture(GL_TEXTURE_2D, texture->Name());
		
		return _textureUnit;
	}
	
	void RenderingPipeline::BindShader(Shader *shader)
	{
		if(_currentShader != shader->program)
		{
			glUseProgram(shader->program);
			
			if(shader->time != -1)
				glUniform1f(shader->time, _time);
			
			_currentShader = shader->program;
		}
	}
	
	void RenderingPipeline::BindMaterial(Material *material)
	{
		material->Push();
		
		if(material != _currentMaterial)
		{
			Array<Texture> *textures = material->Textures();
			Array<GLuint> *textureLocations = &material->Shader()->texlocations;
			
			if(textureLocations->Count() > 0)
			{
				machine_uint textureCount = MIN(textureLocations->Count(), textures->Count());
				
				for(machine_uint i=0; i<textureCount; i++)
				{
					GLint location = textureLocations->ObjectAtIndex(i);
					Texture *texture = textures->ObjectAtIndex(i);
					
					glUniform1i(location, BindTexture(texture));
				}
			}
			
			_currentMaterial = material;
		}
		
		if(material->culling != _cullingEnabled)
		{
			if(material->culling)
			{
				if(!_cullingEnabled)
				{
					glEnable(GL_CULL_FACE);
					_cullingEnabled = true;
				}
				
				if(material->cullmode != _cullMode)
				{
					glFrontFace(material->cullmode);
					_cullMode = material->cullmode;
				}
			}
			else if(_cullingEnabled)
			{
				glDisable(GL_CULL_FACE);
				_cullingEnabled = false;
			}
		}
		
		if(material->depthtest != _depthTestEnabled || material->depthtestmode != _depthFunc)
		{
			if(material->depthtest)
			{
				if(!_depthTestEnabled)
				{
					glEnable(GL_DEPTH_TEST);
					_depthTestEnabled = true;
				}
				
				if(material->depthtestmode != _depthFunc)
				{
					glDepthFunc(material->depthtestmode);
					_depthFunc = material->depthtestmode;
				}
			}
			else if(_depthTestEnabled)
			{
				glDisable(GL_DEPTH_TEST);
				_depthTestEnabled = false;
			}
		}
		
		if(!_currentCamera->AllowsDepthWrite())
		{
			if(_depthWrite)
			{
				glDepthMask(GL_FALSE);
				_depthWrite = false;
			}
		}
		else
		{
			if(material->depthwrite != _depthWrite)
			{
				GLboolean depthwrite = (material->depthwrite) ? GL_TRUE : GL_FALSE;
				glDepthMask(depthwrite);
				
				_depthWrite = material->depthwrite;
			}
		}
		
		if(material->blending != _blendingEnabled || material->blendSource != _blendSource || material->blendDestination != _blendDestination)
		{
			if(material->blending)
			{
				if(!_blendingEnabled)
				{
					glEnable(GL_BLEND);
					_blendingEnabled = true;
				}
				
				if(material->blendSource != _blendSource || material->blendDestination != _blendDestination)
				{
					glBlendFunc(material->blendSource, material->blendDestination);
					_blendSource = material->blendSource;
					_blendDestination = material->blendDestination;
				}
			}
			else if(_blendingEnabled)
			{
				glDisable(GL_BLEND);
				_blendingEnabled = false;
			}
		}
		
		material->Pop();
	}

	GLuint RenderingPipeline::BindVAO(const std::tuple<Material *, MeshLODStage *>& tuple)
	{
		auto iterator = _vaos.find(tuple);
		GLuint vao;
		
		if(iterator == _vaos.end())
		{
			Material *material  = std::get<0>(tuple);
			MeshLODStage *stage = std::get<1>(tuple);

			Shader *shader = material->Shader();

			gl::GenVertexArrays(1, &vao);
			gl::BindVertexArray(vao);

			glBindBuffer(GL_ARRAY_BUFFER, stage->VBO());
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, stage->IBO());

			// Vertices
			if(shader->vertPosition != -1 && stage->SupportsFeature(kMeshFeatureVertices))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureVertices);
				size_t offset = stage->OffsetForFeature(kMeshFeatureVertices);

				glEnableVertexAttribArray(shader->vertPosition);
				glVertexAttribPointer(shader->vertPosition, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}

			// Normals
			if(shader->vertNormal != -1 && stage->SupportsFeature(kMeshFeatureNormals))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureNormals);
				size_t offset = stage->OffsetForFeature(kMeshFeatureNormals);

				glEnableVertexAttribArray(shader->vertNormal);
				glVertexAttribPointer(shader->vertNormal, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}

			// Tangents
			if(shader->vertTangent != -1 && stage->SupportsFeature(kMeshFeatureTangents))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureTangents);
				size_t offset = stage->OffsetForFeature(kMeshFeatureTangents);

				glEnableVertexAttribArray(shader->vertTangent);
				glVertexAttribPointer(shader->vertTangent, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}

			// Texcoord0
			if(shader->vertTexcoord0 != -1 && stage->SupportsFeature(kMeshFeatureUVSet0))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureUVSet0);
				size_t offset = stage->OffsetForFeature(kMeshFeatureUVSet0);

				glEnableVertexAttribArray(shader->vertTexcoord0);
				glVertexAttribPointer(shader->vertTexcoord0, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}

			// Texcoord1
			if(shader->vertTexcoord1 != -1 && stage->SupportsFeature(kMeshFeatureUVSet1))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureUVSet1);
				size_t offset = stage->OffsetForFeature(kMeshFeatureUVSet1);

				glEnableVertexAttribArray(shader->vertTexcoord1);
				glVertexAttribPointer(shader->vertTexcoord1, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}

			// Color0
			if(shader->vertColor0 != -1 && stage->SupportsFeature(kMeshFeatureColor0))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureColor0);
				size_t offset = stage->OffsetForFeature(kMeshFeatureColor0);

				glEnableVertexAttribArray(shader->vertColor0);
				glVertexAttribPointer(shader->vertColor0, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}

			// Color1
			if(shader->vertColor1 != -1 && stage->SupportsFeature(kMeshFeatureColor1))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureColor1);
				size_t offset = stage->OffsetForFeature(kMeshFeatureColor1);

				glEnableVertexAttribArray(shader->vertColor1);
				glVertexAttribPointer(shader->vertColor1, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}

			_vaos[tuple] = vao;
			_currentVAO = vao;
			return vao;
		}

		vao = iterator->second;
		if(vao != _currentVAO)
		{
			gl::BindVertexArray(vao);
			_currentVAO = vao;
		}
		
		return iterator->second;
	}

	
	// ---------------------
	// MARK: -
	// MARK: Misc
	// ---------------------
	void RenderingPipeline::WorkOnTask(TaskID task, float delta)
	{
		if(!_hasValidFramebuffer)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, _defaultFBO);

			if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				_frame.clear();
				return;
			}
			
			_hasValidFramebuffer = true;
		}

		if(!_initialized)
			Initialize();

		_time += delta;
		
		// Reset the previous frames data
		_currentMaterial = 0;
		_currentCamera = 0;
		
		while(!_finishFrame || _pushedGroups > 0)
		{
			_frameLock->Lock();

			auto iterator = _frame.begin();
			if(iterator != _frame.end())
			{
				RenderingGroup *group = *iterator;

				_frame.erase(iterator);
				_pushedGroups --;
				_frameLock->Unlock();

				DrawGroup(group);
				delete group;
			}
			else
			{
				_frameLock->Unlock();
			}
		}

		FlushCameras();
	}

	void RenderingPipeline::PushGroup(RenderingGroup *group)
	{
		_frameLock->Lock();

		_frame.push_back(group);
		_pushedGroups ++;

		_frameLock->Unlock();
	}

	void RenderingPipeline::PepareFrame()
	{
		_finishFrame = false;
		_pushedGroups = 0;
	}

	void RenderingPipeline::FinishFrame()
	{
		_finishFrame = true;
	}
}
