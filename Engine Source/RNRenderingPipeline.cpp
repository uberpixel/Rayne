//
//  RNRenderingPipeline.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderingPipeline.h"

namespace RN
{
	RenderingPipeline::RenderingPipeline()
	{
		// Some general state variables
		_defaultFBO = 0;
		_defaultWidth = _defaultHeight = 0;
		
		_currentMaterial = 0;
		_currentMesh     = 0;
		_currentVAO      = 0;
		
		_finishFrame = 0;
		
		_cullingEnabled   = false;
		_depthTestEnabled = false;
		_blendingEnabled  = false;
		_depthWrite       = false;
		
		_cullMode  = GL_CCW;
		_depthFunc = GL_LESS;
		
		_blendSource      = GL_ONE;
		_blendDestination = GL_ZERO;
		
		_hasValidFramebuffer = false;
		_activeTextureUnits = 0;
		
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
		
		_frameLock->Release();
	}
	
	void RenderingPipeline::InitializeFramebufferCopy()
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
		// Flush the cameras to the screen
		// If we are using OpenGL 3.0+ we can use glBlitFramebuffer() to effeciently copy the frambeuffers content around
		// however, this can only be used when the camera renders without any kind of Material attached to it.
		
#if defined(GL_DRAW_FRAMEBUFFER) && defined(GL_READ_FRAMEBUFFER)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _defaultFBO);
#else
		glBindFramebuffer(GL_FRAMEBUFFER, _defaultFBO);
#endif
		
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glViewport(0, 0, _defaultWidth, _defaultHeight);
		
		for(auto iterator=_flushCameras.begin(); iterator!=_flushCameras.end(); iterator++)
		{
			Camera *camera  = *iterator;
			
#if defined(GL_DRAW_FRAMEBUFFER) && defined(GL_READ_FRAMEBUFFER)
			if(!camera->Material() && gl::BlitFramebuffer)
			{
				const Rect frame = camera->Frame();
				
				glBindFramebuffer(GL_READ_FRAMEBUFFER, camera->Framebuffer());
				gl::BlitFramebuffer(0, 0, frame.width, frame.height, frame.x, frame.y, frame.width, frame.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			}
			else
			{
				FlushCamera(0, camera);
			}
#else
			FlushCamera(0, camera);
#endif
		}
		
#if GL_EXT_debug_marker
		glPopGroupMarkerEXT();
#endif
		
		_flushCameras.clear();
	}
	
	void RenderingPipeline::FlushCamera(Camera *target, Camera *source)
	{
		source->Push();
		
		Texture *texture = source->Target();
		Material *material = source->Material();
		
		Shader *shader = material ? material->Shader() : _copyShader;
		
		const Rect frame = source->Frame();
		
		if(material)
			BindMaterial(material);
		
		if(!target || target->Depthbuffer() == 0)
		{
			if(_depthTestEnabled)
			{
				glDisable(GL_DEPTH_TEST);
				_depthTestEnabled = false;
			}
		}
		
		glUseProgram(shader->program);
		
		if(_currentVAO != _copyVAO)
		{
			gl::BindVertexArray(_copyVAO);
			_currentVAO = _copyVAO;
			
			glBindBuffer(GL_ARRAY_BUFFER, _copyVBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _copyIBO);
		}
		
		glEnableVertexAttribArray(shader->position);
		glVertexAttribPointer(shader->position,  2, GL_FLOAT, GL_FALSE, 16, (const void *)0);
		
		glEnableVertexAttribArray(shader->texcoord0);
		glVertexAttribPointer(shader->texcoord0, 2, GL_FLOAT, GL_FALSE, 16, (const void *)8);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture->Name());
		glUniform1i(shader->targetmap, 0);
		
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
		
		glDisableVertexAttribArray(shader->position);
		glDisableVertexAttribArray(shader->texcoord0);
		
		source->Pop();
	}
	
	void RenderingPipeline::DrawGroup(RenderingGroup *group)
	{
		Camera *previous = 0;
		Camera *camera = group->camera;
		std::vector<RenderingIntent> *frame = &group->intents;
		
		while(camera)
		{
			camera->Bind();
			camera->PrepareForRendering();
			
			if(!(camera->flags & Camera::FlagDrawTarget))
			{
				std::vector<RenderingIntent>::iterator iterator;
				for(iterator=frame->begin(); iterator!=frame->end(); iterator++)
				{
					iterator->Push();
					
					Model *model = iterator->model;
					
					uint32 count = model->Meshes();
					for(uint32 i=0; i<count; i++)
					{
						Mesh *mesh = model->MeshAtIndex(i);
						Material *material = model->MaterialForMesh(mesh);
						
						Shader *shader = material->Shader();
						
						glUseProgram(shader->program);
						
						// Bind the material
						BindMaterial(material);
						
						// Update the built-in uniforms
						if(shader->time != -1)
							glUniform1f(shader->time, _time);
						
						if(shader->matProj != -1)
							glUniformMatrix4fv(shader->matProj, 1, GL_FALSE, camera->ProjectionMatrix().m);
						
						if(shader->matProjInverse != -1)
							glUniformMatrix4fv(shader->matProjInverse, 1, GL_FALSE, camera->InverseProjectionMatrix().m);
						
						if(shader->matView != -1)
							glUniformMatrix4fv(shader->matView, 1, GL_FALSE, camera->ViewMatrix().m);
						
						if(shader->matViewInverse != -1)
							glUniformMatrix4fv(shader->matViewInverse, 1, GL_FALSE, camera->InverseViewMatrix().m);
						
						if(shader->matModel != -1)
							glUniformMatrix4fv(shader->matModel, 1, GL_FALSE, iterator->transform.m);
						
						if(shader->matModelInverse != -1)
							glUniformMatrix4fv(shader->matModelInverse, 1, GL_FALSE, iterator->transform.Inverse().m);
						
						if(shader->matProjViewModel != -1)
						{
							Matrix projViewModel = camera->ProjectionMatrix() * camera->ViewMatrix() * iterator->transform;
							glUniformMatrix4fv(shader->matProjViewModel, 1, GL_FALSE, projViewModel.m);
						}
						
						if(shader->matProjViewModelInverse != -1)
						{
							Matrix projViewModel = camera->InverseProjectionMatrix() * camera->InverseViewMatrix() * iterator->transform.Inverse();
							glUniformMatrix4fv(shader->matProjViewModel, 1, GL_FALSE, projViewModel.m);
						}
						
						// Draw the mesh
						DrawMesh(mesh);
					}
					
					iterator->Pop();
				}
			}
			else
			{
				if(previous)
				{
					// Flush the previous camera into the camera
					FlushCamera(camera, previous);
				}
			}
			
			previous = camera;
			
			camera->Unbind();
			camera = camera->Stage();
			
			if(!camera)
				_flushCameras.push_back(previous);
		}
	}
	
	void RenderingPipeline::BindMaterial(Material *material)
	{
		if(material == _currentMaterial)
			return;
		
		for(machine_uint i=0; i<_activeTextureUnits; i++)
		{
			glActiveTexture((GLenum)(GL_TEXTURE0 + i));
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		
		_activeTextureUnits = 0;
		
		if(material)
		{
			material->Push();
			
			ObjectArray *textures = material->Textures();
			Array<GLuint> *textureLocations = &material->Shader()->texlocations;
			
			if(textureLocations->Count() > 0)
			{
				for(machine_uint i=0; i<textureLocations->Count(); i++)
				{
					GLint location = textureLocations->ObjectAtIndex(i);
					Texture *texture = (Texture *)textures->ObjectAtIndex(i);
					
					glActiveTexture((GLenum)(GL_TEXTURE0 + i));
					glBindTexture(GL_TEXTURE_2D, texture->Name());
					glUniform1i(location, (GLint)i);
				}
				
				_activeTextureUnits = textureLocations->Count();
			}
			
			if(!_currentMaterial || material->culling != _currentMaterial->culling)
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
			
			if(!_currentMaterial || (material->depthtest != _currentMaterial->depthtest || material->depthtestmode != _depthFunc))
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
			
			if(!_currentMaterial || (material->blending != _currentMaterial->blending || material->blendSource != _blendSource || material->blendDestination != _blendDestination))
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
			if(shader->position != -1 && stage->SupportsFeature(kMeshFeatureVertices))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureVertices);
				size_t offset = stage->OffsetForFeature(kMeshFeatureVertices);
				
				glEnableVertexAttribArray(shader->position);
				glVertexAttribPointer(shader->position, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}
			
			// Normals
			if(shader->normal != -1 && stage->SupportsFeature(kMeshFeatureNormals))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureNormals);
				size_t offset = stage->OffsetForFeature(kMeshFeatureNormals);
				
				glEnableVertexAttribArray(shader->normal);
				glVertexAttribPointer(shader->normal, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}
			
			// Tangents
			if(shader->tangent != -1 && stage->SupportsFeature(kMeshFeatureTangents))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureTangents);
				size_t offset = stage->OffsetForFeature(kMeshFeatureTangents);
				
				glEnableVertexAttribArray(shader->tangent);
				glVertexAttribPointer(shader->tangent, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}
			
			// Texcoord0
			if(shader->texcoord0 != -1 && stage->SupportsFeature(kMeshFeatureUVSet0))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureUVSet0);
				size_t offset = stage->OffsetForFeature(kMeshFeatureUVSet0);
				
				glEnableVertexAttribArray(shader->texcoord0);
				glVertexAttribPointer(shader->texcoord0, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}
			
			// Texcoord1
			if(shader->texcoord1 != -1 && stage->SupportsFeature(kMeshFeatureUVSet1))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureUVSet1);
				size_t offset = stage->OffsetForFeature(kMeshFeatureUVSet1);
				
				glEnableVertexAttribArray(shader->texcoord1);
				glVertexAttribPointer(shader->texcoord1, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}
			
			// Color0
			if(shader->color0 != -1 && stage->SupportsFeature(kMeshFeatureColor0))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureColor0);
				size_t offset = stage->OffsetForFeature(kMeshFeatureColor0);
				
				glEnableVertexAttribArray(shader->color0);
				glVertexAttribPointer(shader->color0, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}
			
			// Color1
			if(shader->color1 != -1 && stage->SupportsFeature(kMeshFeatureColor1))
			{
				MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureColor1);
				size_t offset = stage->OffsetForFeature(kMeshFeatureColor1);
				
				glEnableVertexAttribArray(shader->color1);
				glVertexAttribPointer(shader->color1, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)offset);
			}
			
			_vaos[tuple] = vao;
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
				return;
			
			_frame.clear();
			_hasValidFramebuffer = true;
		}
		
		if(!_copyShader)
			InitializeFramebufferCopy();
		
		_time += delta;

		while(1)
		{
			_frameLock->Lock();
			
			auto iterator = _frame.begin();
			if(iterator != _frame.end())
			{
				RenderingGroup group = *iterator;
				_frame.erase(iterator);
				_frameLock->Unlock();
				
				DrawGroup(&group);
			}
			else
			{
				_frameLock->Unlock();
				
				if(_finishFrame)
					break;
			}
		}
		
		FlushCameras();
		
		_finishFrame = false;
		_currentMaterial = 0;
		_currentMesh = 0;
	}
	
	void RenderingPipeline::PushGroup(const RenderingGroup& group)
	{
		_frameLock->Lock();
		_frame.push_back(group);
		_frameLock->Unlock();
	}
	
	void RenderingPipeline::FinishFrame()
	{
		_finishFrame = true;
	}
}
