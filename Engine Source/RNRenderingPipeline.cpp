//
//  RNRenderingPipeline.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderingPipeline.h"
#include "RNKernel.h"

namespace RN
{	
	bool SortRenderingObject(struct RenderingObject& a, struct RenderingObject& b)
	{
		machine_uint objA = (machine_uint)a.mesh;
		machine_uint objB = (machine_uint)b.mesh;
		
		return (objA < objB);
	}
	
	
	RenderingPipeline::RenderingPipeline()
	{
		// Some general state variables
		_defaultFBO = 0;
		_defaultWidth = _defaultHeight = 0;
		
		_currentMaterial = 0;
		_currentMesh     = 0;
		_currentVAO      = 0;
		
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
		
		_numInstancingMatrices = 500;
		_instancingMatrices = (Matrix *)malloc(_numInstancingMatrices * sizeof(Matrix));
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
		
#if !(PLATFORM_IOS)
		glDeleteTextures(4, _lightTextures);
		glDeleteBuffers(4, _lightBuffers);
#endif
		
		free(_instancingMatrices);
		
		_frameLock->Release();
	}
	
	void RenderingPipeline::Initialize()
	{
		_copyShader = new Shader();
		_copyShader->SetFragmentShader("shader/rn_CopyFramebuffer.fsh");
		_copyShader->SetVertexShader("shader/rn_CopyFramebuffer.vsh");
		_copyShader->Link();
		
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
		
#if !(PLATFORM_IOS)
		glGenTextures(4, _lightTextures);
		glGenBuffers(4, _lightBuffers);
		
		//indexpos
		glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[0]);
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[0]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, _lightBuffers[0]);
		
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
#endif
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
	
	
	void RenderingPipeline::FlushCameras()
	{
#if GL_EXT_debug_marker
		glPushGroupMarkerEXT(0, "Flushing cameras");
#endif

		glBindFramebuffer(GL_FRAMEBUFFER, _defaultFBO);
		
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
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
		
		Shader *shader = _copyShader;
		
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
		
		glUseProgram(shader->program);
		
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
		Shader *shader     = material->Shader();
		
		BindMaterial(material);
		
		if(!stage->HasDepthbuffer())
		{
			if(_depthTestEnabled)
			{
				glDisable(GL_DEPTH_TEST);
				_depthTestEnabled = false;
			}
		}
		
		if(_currentVAO != _copyVAO)
		{
			gl::BindVertexArray(_copyVAO);
			_currentVAO = _copyVAO;
			
			glBindBuffer(GL_ARRAY_BUFFER, _copyVBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _copyIBO);
		}
		
		
		glUseProgram(shader->program);
		
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
		
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
		
		glDisableVertexAttribArray(shader->vertPosition);
		glDisableVertexAttribArray(shader->vertTexcoord0);
		
		stage->Pop();
	}
	
	void RenderingPipeline::DrawGroup(RenderingGroup *group)
	{
		// Object pre-pass
		std::vector<RenderingIntent> *frame = &group->intents;
		std::vector<RenderingObject> objects;
		
		objects.reserve(frame->size());
		
		// Unpack the frame
		for(auto i=frame->begin(); i!=frame->end(); i++)
		{
			Model *model = i->model;
			uint32 meshes = model->Meshes();
			
			for(uint32 j=0; j<meshes; j++)
			{
				RenderingObject object;
				object.mesh = model->MeshAtIndex(j);
				object.material = model->MaterialForMesh(object.mesh);
				object.transform = &i->transform;
				
				objects.push_back(object);
			}
		}
		
		// Sort the frame
		std::sort(objects.begin(), objects.end(), SortRenderingObject);
		
		// Render all cameras
		Camera *previous = 0;
		Camera *camera = group->camera;
		
		// Creating light list
		std::vector<RenderingLight> *lights = &group->lights;
		
		Vector4 *lightpos = new Vector4[lights->size()];
		Vector3 *lightcolor = new Vector3[lights->size()];
		std::vector<RenderingLight>::iterator lightiterator;
		int lightcount = 0;
		for(lightiterator=lights->begin(); lightiterator!=lights->end(); lightiterator++)
		{
			lightpos[lightcount].x = lightiterator->position.x;
			lightpos[lightcount].y = lightiterator->position.y;
			lightpos[lightcount].z = lightiterator->position.z;
			lightpos[lightcount].w = lightiterator->range;
			lightcolor[lightcount] = lightiterator->color;
			lightcount++;
		}
//		printf("Lightcount: %i\n", lightcount);

#if !(PLATFORM_IOS)
		std::vector<int> lightindexpos;
		std::vector<int> lightindices;
		std::vector<int> tempindices;
		Rect rect = camera->Frame();
		int tileswidth = rect.width/camera->LightTiles().x;
		int tilesheight = rect.height/camera->LightTiles().y;
		Vector3 corner1 = camera->CamToWorld(Vector3(-1.0f, -1.0f, 1.0f));
		Vector3 corner2 = camera->CamToWorld(Vector3(1.0f, -1.0f, 1.0f));
		Vector3 corner3 = camera->CamToWorld(Vector3(-1.0f, 1.0f, 1.0f));
		Vector2 lighttilesize = camera->LightTiles()*_scaleFactor;
		
		Vector3 dirx = (corner2-corner1)/tileswidth;
		Vector3 diry = (corner3-corner1)/tilesheight;
		
		Plane plleft;
		Plane plright;
		Plane pltop;
		Plane plbottom;
		int counter;
		for(float x = 0.0f; x < tileswidth; x += 1.0f)
		{
			for(float y = 0.0f; y < tilesheight; y += 1.0f)
			{
				plleft.SetPlane(camera->Position(), corner1+dirx*x+diry*(y+1.0f), corner1+dirx*x+diry*(y-1.0f));
				plright.SetPlane(camera->Position(), corner1+dirx*(x+1.0f)+diry*(y+1.0f), corner1+dirx*(x+1.0f)+diry*(y-1.0f));
				pltop.SetPlane(camera->Position(), corner1+dirx*(x-1.0f)+diry*(y+1.0f), corner1+dirx*(x+1.0f)+diry*(y+1.0f));
				plbottom.SetPlane(camera->Position(), corner1+dirx*(x-1.0f)+diry*y, corner1+dirx*(x+1.0f)+diry*y);
				
				counter = -1;
				for(lightiterator=lights->begin(); lightiterator!=lights->end(); lightiterator++)
				{
					counter++;
					if(plleft.Distance(lightiterator->position) > lightiterator->range)
						continue;
					
					if(plright.Distance(lightiterator->position) < -lightiterator->range)
						continue;
					
					if(pltop.Distance(lightiterator->position) < -lightiterator->range)
						continue;
					
					if(plbottom.Distance(lightiterator->position) > lightiterator->range)
						continue;
					
					tempindices.push_back(counter);
				}
//				printf("lights: %i \n", tempindices.size());
				lightindexpos.push_back(static_cast<int>(lightindices.size()));
				lightindexpos.push_back(static_cast<int>(tempindices.size()));
				lightindices.insert(lightindices.end(), tempindices.begin(), tempindices.end());
				tempindices.clear();
			}
		}
		
		//indexpos
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[0]);
		glBufferData(GL_TEXTURE_BUFFER, lightindexpos.size()*sizeof(int), &lightindexpos[0], GL_STREAM_DRAW);
		
		//indices
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[1]);
		glBufferData(GL_TEXTURE_BUFFER, lightindices.size()*sizeof(int), &lightindices[0], GL_STREAM_DRAW);
		
		//lightpos
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[2]);
		glBufferData(GL_TEXTURE_BUFFER, lightcount*4*sizeof(float), lightpos, GL_STREAM_DRAW);
		
		//lightcol
		glBindBuffer(GL_TEXTURE_BUFFER, _lightBuffers[3]);
		glBufferData(GL_TEXTURE_BUFFER, lightcount*3*sizeof(float), lightcolor, GL_STREAM_DRAW);
		
		glBindBuffer(GL_TEXTURE_BUFFER, 0);
#endif

		while(camera)
		{
			camera->Bind();
			camera->PrepareForRendering();
			
			if(!(camera->CameraFlags() & Camera::FlagDrawTarget))
			{
				Material *surfaceMaterial = camera->Material();
				
				uint32 offset = 1;
				uint32 noCheck = 0;
				
				for(auto i=objects.begin(); i!=objects.end(); i+=offset)
				{
					Mesh *mesh = i->mesh;
					Material *material = surfaceMaterial ? surfaceMaterial : i->material;
					Shader *shader = material->Shader();
					
					// Send generic attributes to the shader
					glUseProgram(shader->program);
					
					if(shader->time != -1)
						glUniform1f(shader->time, _time);
					
					// Light data
					if(shader->lightCount != -1)
						glUniform1i(shader->lightCount, lightcount);
					
					if(shader->lightPosition != -1 && lightcount > 0)
						glUniform4fv(shader->lightPosition, lightcount, &(lightpos[0].x));
						
					if(shader->lightColor != -1 && lightcount > 0)
						glUniform3fv(shader->lightColor, lightcount, &(lightcolor[0].x));
#if !(PLATFORM_IOS)
					if(shader->lightListPosition != -1)
					{
						_textureUnit ++;
						_textureUnit %= _maxTextureUnits;
						glActiveTexture((GLenum)(GL_TEXTURE0 + _textureUnit));
						glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[2]);
						glUniform1i(shader->lightListPosition, _textureUnit);
					}
					
					if(shader->lightList != -1)
					{
						_textureUnit ++;
						_textureUnit %= _maxTextureUnits;
						glActiveTexture((GLenum)(GL_TEXTURE0 + _textureUnit));
						glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[1]);
						glUniform1i(shader->lightList, _textureUnit);
					}
					
					if(shader->lightListOffset != -1)
					{
						_textureUnit ++;
						_textureUnit %= _maxTextureUnits;
						glActiveTexture((GLenum)(GL_TEXTURE0 + _textureUnit));
						glBindTexture(GL_TEXTURE_BUFFER, _lightTextures[0]);
						glUniform1i(shader->lightListOffset, _textureUnit);
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
						glUniform2fv(shader->lightTileSize, 1, &lighttilesize.x);
					}
#endif
					
					
					// Matrices
					if(shader->matProj != -1)
						glUniformMatrix4fv(shader->matProj, 1, GL_FALSE, camera->projectionMatrix.AccessPast().m);
					
					if(shader->matProjInverse != -1)
						glUniformMatrix4fv(shader->matProjInverse, 1, GL_FALSE, camera->inverseProjectionMatrix.AccessPast().m);
					
					if(shader->matView != -1)
						glUniformMatrix4fv(shader->matView, 1, GL_FALSE, camera->viewMatrix.AccessPast().m);
					
					if(shader->matViewInverse != -1)
						glUniformMatrix4fv(shader->matViewInverse, 1, GL_FALSE, camera->inverseViewMatrix.AccessPast().m);
					
					if(shader->matModel != -1)
						glUniformMatrix4fv(shader->matModel, 1, GL_FALSE, i->transform->m);
					
					if(shader->matModelInverse != -1)
						glUniformMatrix4fv(shader->matModelInverse, 1, GL_FALSE, i->transform->Inverse().m);
					
					if(shader->matViewModel != -1)
					{
						Matrix viewModel = camera->viewMatrix.AccessPast() * (*i->transform);
						glUniformMatrix4fv(shader->matViewModel, 1, GL_FALSE, viewModel.m);
					}
					
					if(shader->matViewModelInverse != -1)
					{
						Matrix viewModel = camera->inverseViewMatrix * i->transform->Inverse();
						glUniformMatrix4fv(shader->matViewModelInverse, 1, GL_FALSE, viewModel.m);
					}
					
					if(shader->matProjViewModel != -1)
					{
						Matrix projViewModel = camera->projectionMatrix.AccessPast() * camera->viewMatrix.AccessPast() * (*i->transform);
						glUniformMatrix4fv(shader->matProjViewModel, 1, GL_FALSE, projViewModel.m);
					}
					
					if(shader->matProjViewModelInverse != -1)
					{
						Matrix projViewModel = camera->inverseProjectionMatrix.AccessPast() * camera->inverseViewMatrix.AccessPast() * i->transform->Inverse();
						glUniformMatrix4fv(shader->matProjViewModel, 1, GL_FALSE, projViewModel.m);
					}
						
					
					// Check if we can use instancing here
					if(noCheck == 0)
					{
						if(SupportsOpenGLFeature(kOpenGLFeatureInstancing))
						{
							auto end = i + 1;
							offset = 1;
							
							while(end != objects.end())
							{
								if(end->mesh != mesh)
									break;
								
								offset ++;
								end ++;
							}
							
							if(offset >= kRNRenderingPipelineInstancingCutOff)
							{
								if(material->Shader()->imatModel != -1)
								{
									BindMaterial(material);
									DrawMeshInstanced(i, end, offset);
									
									continue;
								}
							}
							
							noCheck = offset;
							offset = 1;
						}
					}
					else
					{
						noCheck --;
					}
					
					// Render the mesh normally				
					BindMaterial(material);
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
			
			if(!camera)
				_flushCameras.push_back(previous);
		}
		
		delete[] lightcolor;
		delete[] lightpos;
	}
	
	uint32 RenderingPipeline::BindTexture(Texture *texture)
	{
		_textureUnit ++;
		_textureUnit %= _maxTextureUnits;
		
		glActiveTexture((GLenum)(GL_TEXTURE0 + _textureUnit));
		glBindTexture(GL_TEXTURE_2D, texture->Name());

		return _textureUnit;
	}
	
	void RenderingPipeline::BindMaterial(Material *material)
	{
		if(material)
		{
			material->Push();
			
			if(material != _currentMaterial)
			{
				ObjectArray *textures = material->Textures();
				Array<GLuint> *textureLocations = &material->Shader()->texlocations;
				
				if(textureLocations->Count() > 0)
				{
					machine_uint textureCount = MIN(textureLocations->Count(), textures->Count());
					
					for(machine_uint i=0; i<textureCount; i++)
					{
						GLint location = textureLocations->ObjectAtIndex(i);
						Texture *texture = (Texture *)textures->ObjectAtIndex(i);
						
						glUniform1i(location, BindTexture(texture));
					}
				}
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
			
			if(material->depthwrite != _depthWrite)
			{
				GLboolean depthwrite = (material->depthwrite) ? GL_TRUE : GL_FALSE;
				glDepthMask(depthwrite);
				
				_depthWrite = material->depthwrite;
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
		
		_currentMaterial = material;
	}
	
	void RenderingPipeline::DrawMesh(Mesh *mesh)
	{
		if(mesh && mesh != _currentMesh)
		{
			mesh->Push();
			
			MeshLODStage *stage = mesh->LODStage(0);
			
			std::tuple<Material *, MeshLODStage *> tuple = std::tuple<Material *, MeshLODStage *>(_currentMaterial, stage);
			GLuint vao = VAOForTuple(tuple);
			
			if(vao != _currentVAO)
			{
				gl::BindVertexArray(vao);
				_currentVAO = vao;
			}
			
			mesh->Pop();
		}
		
		if(mesh)
		{
			MeshLODStage *stage = mesh->LODStage(0);
			MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureIndices);
			
			glDrawElements(GL_TRIANGLES, (GLsizei)descriptor->elementCount, GL_UNSIGNED_SHORT, 0);
		}
		
		_currentMesh = mesh;
	}
	
	void RenderingPipeline::DrawMeshInstanced(std::vector<RenderingObject>::iterator begin, const std::vector<RenderingObject>::iterator& last, uint32 count)
	{
		Mesh *mesh = begin->mesh;
		MeshLODStage *stage = mesh->LODStage(0);
		MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureIndices);
		
		Material *material = begin->material;
		Shader *shader = material->Shader();
		
		if(count > _numInstancingMatrices)
		{
			_numInstancingMatrices = count;
			
			free(_instancingMatrices);
			_instancingMatrices = (Matrix *)malloc(_numInstancingMatrices * sizeof(Matrix));
		}
		
		GLuint vao = VAOForTuple(std::tuple<Material *, MeshLODStage *>(material, stage));
		if(_currentVAO != vao)
		{
			gl::BindVertexArray(vao);
			_currentVAO = vao;
		}
		
		glBindBuffer(GL_ARRAY_BUFFER, _instancingVBO);
		
		for(int i=0; i<4; i++)
		{
			glEnableVertexAttribArray(shader->imatModel + i);
			glVertexAttribPointer(shader->imatModel + i, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 16, (void *)(sizeof(float) * (i * 4)));
			gl::VertexAttribDivisor(shader->imatModel + i, 1);
		}
		
		for(uint32 i=0; begin!=last; begin++, i++)
		{
			_instancingMatrices[i] = *begin->transform;
		}
		
		glBufferData(GL_ARRAY_BUFFER, count * sizeof(Matrix), _instancingMatrices, GL_DYNAMIC_DRAW);		
		gl::DrawElementsInstanced(GL_TRIANGLES, (GLsizei)descriptor->elementCount, GL_UNSIGNED_SHORT, 0, count);
		
		for(int i=0; i<4; i++)
		{
			glDisableVertexAttribArray(shader->imatModel + i);
		}
	}
	
	GLuint RenderingPipeline::VAOForTuple(const std::tuple<Material *, MeshLODStage *>& tuple)
	{
		auto iterator = _vaos.find(tuple);
		if(iterator == _vaos.end())
		{
			GLuint vao;
			
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
		
		return iterator->second;
	}
	
	
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
		
		if(!_copyShader)
			Initialize();
		
		_time += delta;
		
		while(!_finishFrame || _pushedGroups > 0)
		{
			_frameLock->Lock();
			
			auto iterator = _frame.begin();
			if(iterator != _frame.end())
			{
				RenderingGroup group = *iterator;
				
				_frame.erase(iterator);
				_pushedGroups --;
				
				_frameLock->Unlock();
				
				DrawGroup(&group);
			}
			else
			{
				_frameLock->Unlock();
			}
		}
		
		FlushCameras();
		
		_currentMaterial = 0;
		_currentMesh = 0;
	}
	
	void RenderingPipeline::PushGroup(const RenderingGroup& group)
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
