//
//  RNRendererBackend.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRendererBackend.h"
#include "RNRendererFrontend.h"
#include "RNKernel.h"
#include "RNOpenGL.h"

namespace RN
{
	RendererBackend::RendererBackend(RendererFrontend *frontend)
	{
		RN_ASSERT0(frontend != 0);
		
		// Some general state variables
		_defaultFBO      = 0;
		_currentMaterial = 0;
		_currentMesh     = 0;
		_currentVAO      = 0;
		
		_frontend    = frontend;
		_lastFrame   = 0;
		_lastFrameID = 0;
		
		_cullingEnabled   = false;
		_depthTestEnabled = false;
		_blendingEnabled  = false;
		_depthWrite       = false;
		
		_cullMode  = GL_CCW;
		_depthFunc = GL_LESS;
		
		_blendSource      = GL_ONE;
		_blendDestination = GL_ZERO;
		
		// Setup framebuffer copy stuff
		_copyShader = 0;
		
		_copyVertices[0] = Vector4(0.0f, 1.0f, 0.0f, 1.0f);
		_copyVertices[1] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		_copyVertices[2] = Vector4(1.0f, 0.0f, 1.0f, 0.0f);
		_copyVertices[3] = Vector4(0.0f, 0.0f, 0.0f, 0.0f);

		_copyIndices[0] = 0;
		_copyIndices[1] = 3;
		_copyIndices[2] = 1;
		_copyIndices[3] = 2;
		_copyIndices[4] = 1;
		_copyIndices[5] = 3;
	}
	
	RendererBackend::~RendererBackend()
	{
		if(_lastFrame)
			delete _lastFrame;
		
		if(_copyShader)
		{
			_copyShader->Release();
				
			glDeleteBuffers(1, &_copyVBO);
			glDeleteBuffers(1, &_copyIBO);
			
			gl::DeleteVertexArrays(1, &_copyVAO);
		}
	}
	
	void RendererBackend::InitializeFramebufferCopy()
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
		
		glEnableVertexAttribArray(_copyShader->position);
		glVertexAttribPointer(_copyShader->position,  2, GL_FLOAT, GL_FALSE, 16, (const void *)0);
		
		glEnableVertexAttribArray(_copyShader->texcoord0);
		glVertexAttribPointer(_copyShader->texcoord0, 2, GL_FLOAT, GL_FALSE, 16, (const void *)8);
	}
	
	
	void RendererBackend::SetDefaultFBO(GLuint fbo)
	{
		_defaultFBO = fbo;
	}
	
	void RendererBackend::SetDefaultFrame(uint32 width, uint32 height)
	{
		_defaultWidth  = width;
		_defaultHeight = height;
		
		_copyProjection.MakeProjectionOrthogonal(0.0f, width, 0.0f, height, -1.0f, 1.0f);
	}
	
	void RendererBackend::DrawFrame()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, _defaultFBO);
		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			return;
		
		if(!_copyShader)
			InitializeFramebufferCopy();
		
		std::vector<RenderingGroup> *frame = 0;
		uint32 frameID = _frontend->CommittedFrame(&frame);
		
		if(frameID == _lastFrameID)
		{
			if(_lastFrame)
			{
				PrepareFrame(frame);
			}
			
			return;
		}
		
		if(_lastFrame)
			delete _lastFrame;
		

		PrepareFrame(frame);
		
		_lastFrame = frame;
		_lastFrameID = frameID;
	}
	
	void RendererBackend::PrepareFrame(std::vector<RenderingGroup> *frame)
	{
		_drawLock.Lock();
		
		_currentMesh = 0;
		_currentMaterial = 0;
		
		try
		{
			// Draw all rendering groups
			for(auto iterator=frame->begin(); iterator!=frame->end(); iterator++)
			{
				RenderingGroup *group = &(*iterator);
				DrawGroup(group);
			}
			
			FlushCameras();
			
			_currentMaterial = 0;
			_currentMesh = 0;
		}
		catch (ErrorException e)
		{
			_drawLock.Unlock();
			throw e;
		}
		
		_drawLock.Unlock();
	}
	
	void RendererBackend::FlushCameras()
	{
#if GL_EXT_debug_marker
		glPushGroupMarkerEXT(0, "Flushing cameras");
#endif
		// Flush the cameras to the screen
#if defined(GL_DRAW_FRAMEBUFFER) && defined(GL_READ_FRAMEBUFFER)
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _defaultFBO);
#else
		glBindFramebuffer(GL_FRAMEBUFFER, _defaultFBO);
#endif
		
		glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glViewport(0, 0, _defaultWidth, _defaultHeight);
		
		for(auto iterator=_flushCameras.begin(); iterator!=_flushCameras.end(); iterator++)
		{
			Camera  *camera  = *iterator;
			FlushCamera(camera);
			
#if defined(GL_DRAW_FRAMEBUFFER) && defined(GL_READ_FRAMEBUFFER)
			if(!camera->Material())
			{
				const Rect frame = camera->Frame();
				
				glBindFramebuffer(GL_READ_FRAMEBUFFER, camera->Framebuffer());
				glBlitFramebuffer(0, 0, frame.width, frame.height, frame.x, frame.y, frame.width, frame.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			}
			else
			{
				FlushCamera(camera);
			}
#else
			FlushCamera(camera);
#endif
		}
		
#if GL_EXT_debug_marker
		glPopGroupMarkerEXT();
#endif
		
		_flushCameras.clear();
	}
	
	void RendererBackend::FlushCamera(Camera *camera)
	{
		camera->Push();
		
		Texture *texture = camera->Target();
		Material *material = camera->Material();
		
		Shader *shader = material ? material->Shader() : _copyShader;
		
		const Rect frame = camera->Frame();
		
		if(material)
			BindMaterial(material);
		
		glUseProgram(shader->program);
		glUniformMatrix4fv(shader->matProj, 1, GL_FALSE, _copyProjection.m);
		
		glBindBuffer(GL_ARRAY_BUFFER, _copyVBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _copyIBO);
		
		if(_currentVAO != _copyVAO)
		{
			gl::BindVertexArray(_copyVAO);
			_currentVAO = _copyVAO;
		}
		
		glEnableVertexAttribArray(shader->position);
		glVertexAttribPointer(shader->position,  2, GL_FLOAT, GL_FALSE, 16, (const void *)0);
		
		glEnableVertexAttribArray(shader->texcoord0);
		glVertexAttribPointer(shader->texcoord0, 2, GL_FLOAT, GL_FALSE, 16, (const void *)8);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture->Name());
		glUniform1i(shader->targetmap, 0);
		
		Matrix matrix;
		matrix.MakeScale(Vector3(frame.width, frame.height, 0.0f));
		
		glUniformMatrix4fv(shader->matModel, 1, GL_FALSE, matrix.m);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
		
		glDisableVertexAttribArray(shader->position);
		glDisableVertexAttribArray(shader->texcoord0);
		
		camera->Pop();
	}
	
	void RendererBackend::DrawGroup(RenderingGroup *group)
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
					RenderingIntent *intent = &(*iterator);
					Material *material = intent->material;
					Shader *shader = material->Shader();
					
					intent->Push();
					
					glUseProgram(shader->program);
					
					// Bind the material
					BindMaterial(material);
					
					// Make the matrices available
					if(shader->matProj != -1)
						glUniformMatrix4fv(shader->matProj, 1, GL_FALSE, camera->ProjectionMatrix().m);
					
					if(shader->matProjInverse != -1)
						glUniformMatrix4fv(shader->matProjInverse, 1, GL_FALSE, camera->InverseProjectionMatrix().m);
					
					if(shader->matView != -1)
						glUniformMatrix4fv(shader->matView, 1, GL_FALSE, camera->ViewMatrix().m);
					
					if(shader->matViewInverse != -1)
						glUniformMatrix4fv(shader->matViewInverse, 1, GL_FALSE, camera->InverseViewMatrix().m);
					
					if(shader->matModel != -1)
						glUniformMatrix4fv(shader->matModel, 1, GL_FALSE, intent->transform.m);
					
					if(shader->matModelInverse != -1)
						glUniformMatrix4fv(shader->matModelInverse, 1, GL_FALSE, intent->transform.Inverse().m);
					
					if(shader->matProjViewModel != -1)
					{
						Matrix projViewModel = camera->ProjectionMatrix() * camera->ViewMatrix() * intent->transform;
						glUniformMatrix4fv(shader->matProjViewModel, 1, GL_FALSE, projViewModel.m);
					}
					
					if(shader->matProjViewModelInverse != -1)
					{
						Matrix projViewModel = camera->InverseProjectionMatrix() * camera->InverseViewMatrix() * intent->transform.Inverse();
						glUniformMatrix4fv(shader->matProjViewModel, 1, GL_FALSE, projViewModel.m);
					}
					
					// Draw the mesh
					DrawMesh(intent->mesh);
					intent->Pop();
				}
			}
			else
			{
				if(previous)
				{
					// Flush the previous camera into the camera
					FlushCamera(previous);
				}
			}
			
			previous = camera;
			
			camera->Unbind();
			camera = camera->Stage();
			
			if(!camera)
				_flushCameras.push_back(previous);
		}
	}
	
	void RendererBackend::BindMaterial(Material *material)
	{
		if(material == _currentMaterial)
			return;
		
		if(material)
		{
			material->Push();
			
			ObjectArray *textures = material->Textures();
			Array<GLint> *textureLocations = material->TextureLocations();
			
			if(textureLocations->Count() > 0)
			{
				for(machine_uint i=0; i<textureLocations->Count(); i++)
				{
					GLint location = textureLocations->ObjectAtIndex(i);
					if(location == -1)
						break;
					
					Texture *texture = (Texture *)textures->ObjectAtIndex(i);
					
					glActiveTexture((GLenum)(GL_TEXTURE0 + i));
					glBindTexture(GL_TEXTURE_2D, texture->Name());
					glUniform1i(location, (GLint)i);
				}
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
	
	void RendererBackend::DrawMesh(Mesh *mesh)
	{		
		if(mesh && mesh != _currentMesh)
		{
			mesh->Push();
			
			MeshLODStage *stage = mesh->LODStage(0);

			std::tuple<Material *, MeshLODStage *> tuple = std::tuple<Material *, MeshLODStage *>(_currentMaterial, stage);
			GLuint vao = VAOForTuple(tuple);
			
			glBindBuffer(GL_ARRAY_BUFFER, stage->VBO());
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, stage->IBO());
			
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
	
	GLuint RendererBackend::VAOForTuple(const std::tuple<Material *, MeshLODStage *>& tuple)
	{
		auto iterator = _vaos.find(tuple);
		if(iterator == _vaos.end())
		{
			GLuint vao;
			
			Material *material  = std::get<0>(tuple);
			MeshLODStage *stage = std::get<1>(tuple);
			
			Shader *shader = material->Shader();
			
			glBindBuffer(GL_ARRAY_BUFFER, stage->VBO());
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, stage->IBO());
			
			
			gl::GenVertexArrays(1, &vao);
			gl::BindVertexArray(vao);
			
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
}
