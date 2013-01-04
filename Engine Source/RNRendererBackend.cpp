//
//  RNRendererBackend.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRendererBackend.h"
#include "RNRendererFrontend.h"
#include "RNKernel.h"

namespace RN
{
	RendererBackend::RendererBackend(RendererFrontend *frontend)
	{
		RN_ASSERT0(frontend != 0);
		
		_defaultCamera   = 0;
		_currentMaterial = 0;
		_currentMesh     = 0;
		
		_frontend = frontend;
		_lastFrame   = 0;
		_lastFrameID = 0;
		
		_texture2DEnabled = false;
		_cullingEnabled   = false;
		_depthTestEnabled = false;
		_blendingEnabled  = false;
		_depthWrite = false;
		
		_cullMode = GL_CCW;
		_depthFunc = GL_LESS;
		
		_blendSource = GL_ONE;
		_blendDestination = GL_ZERO;
	}
	
	RendererBackend::~RendererBackend()
	{
		if(_lastFrame)
			delete _lastFrame;
		
		if(_defaultCamera)
			delete _defaultCamera;
	}
	
	void RendererBackend::SetDefaultCamera(Camera *camera)
	{
		_drawLock.Lock();
		
		_defaultCamera->Release();
		_defaultCamera = camera;
		_defaultCamera->Retain();
		
		_drawLock.Unlock();
	}
	
	Camera *RendererBackend::DefaultCamera()
	{
		return _defaultCamera;
	}
	
	
	
	void RendererBackend::DrawFrame()
	{
		std::vector<RenderingIntent> *frame = 0;
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
	
	void RendererBackend::PrepareFrame(std::vector<RenderingIntent> *frame)
	{
		_drawLock.Lock();
		
		_currentMesh = 0;
		_currentMaterial = 0;
		
		try
		{
			DrawFrame(frame);
		}
		catch (ErrorException e)
		{
			_drawLock.Unlock();
			throw e;
		}
		
		_drawLock.Unlock();
	}
	
	void RendererBackend::DrawFrame(std::vector<RenderingIntent> *frame)
	{
		if(!_defaultCamera)
			_defaultCamera = new Camera(0, Vector2(1024.0f, 768.0f));
		
		_defaultCamera->Bind();
		_defaultCamera->PrepareForRendering();
		
		Camera *camera = _defaultCamera;
		
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
		
		_defaultCamera->Unbind();
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
				if(!_texture2DEnabled)
				{
					glEnable(GL_TEXTURE_2D);
					_texture2DEnabled = true;
				}
				
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
			else if(_texture2DEnabled)
			{
				glDisable(GL_TEXTURE_2D);
				_texture2DEnabled = false;
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
			Shader *shader = _currentMaterial->Shader();
			
			glBindBuffer(GL_ARRAY_BUFFER, stage->VBO());
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, stage->IBO());
			
			// Vertex
			if(shader->position != -1)
			{
				if(stage->SupportsFeature(kMeshFeatureVertices))
				{
					MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureVertices);
					
					glEnableVertexAttribArray(shader->position);
					glVertexAttribPointer(shader->position, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)stage->OffsetForFeature(kMeshFeatureVertices));
				}
				else
				{
					glDisableVertexAttribArray(shader->position);
				}
			}
			
			// Texcoord0
			if(shader->texcoord0 != -1)
			{
				if(stage->SupportsFeature(kMeshFeatureUVSet0))
				{
					MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureUVSet0);
					
					glEnableVertexAttribArray(shader->texcoord0);
					glVertexAttribPointer(shader->texcoord0, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)stage->OffsetForFeature(kMeshFeatureUVSet0));
				}
				else
				{
					glDisableVertexAttribArray(shader->texcoord0);
				}
			}
			
			// Texcoord1
			if(shader->texcoord1 != -1)
			{
				if(stage->SupportsFeature(kMeshFeatureUVSet1))
				{
					MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureUVSet1);
					
					glEnableVertexAttribArray(shader->texcoord1);
					glVertexAttribPointer(shader->texcoord1, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)stage->OffsetForFeature(kMeshFeatureUVSet1));
				}
				else
				{
					glDisableVertexAttribArray(shader->texcoord1);
				}
			}
			
			// Color0
			if(shader->color0 != -1)
			{
				if(stage->SupportsFeature(kMeshFeatureColor0))
				{
					MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureColor0);
					
					glEnableVertexAttribArray(shader->color0);
					glVertexAttribPointer(shader->color0, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)stage->OffsetForFeature(kMeshFeatureColor0));
				}
				else
				{
					glDisableVertexAttribArray(shader->color0);
				}
			}
			
			// Color1
			if(shader->color1 != -1)
			{
				if(stage->SupportsFeature(kMeshFeatureColor1))
				{
					MeshDescriptor *descriptor = stage->Descriptor(kMeshFeatureColor1);
					
					glEnableVertexAttribArray(shader->color1);
					glVertexAttribPointer(shader->color1, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)stage->Stride(), (const void *)stage->OffsetForFeature(kMeshFeatureColor1));
				}
				else
				{
					glDisableVertexAttribArray(shader->color1);
				}
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
}
