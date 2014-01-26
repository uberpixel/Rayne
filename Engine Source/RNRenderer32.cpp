//
//  RNRenderer32.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderer32.h"
#include "RNThreadPool.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNLightManager.h"
#include "RNOpenGLQueue.h"

namespace RN
{
	Renderer32::Renderer32()
	{
		// Setup framebuffer copy stuff
		_copyVertices[0] = Vector4(-1.0f, -1.0f, 0.0f, 0.0f);
		_copyVertices[1] = Vector4(1.0f, -1.0f,  1.0f, 0.0f);
		_copyVertices[2] = Vector4(-1.0f, 1.0f,  0.0f, 1.0f);
		_copyVertices[3] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
			gl::GenVertexArrays(1, &_copyVAO);
			gl::BindVertexArray(_copyVAO);
			
			gl::GenBuffers(1, &_copyVBO);
			
			gl::BindBuffer(GL_ARRAY_BUFFER, _copyVBO);
			gl::BufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), _copyVertices, GL_STREAM_DRAW);
			
			gl::BindVertexArray(0);
		});
	}
	
	Renderer32::~Renderer32()
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			gl::DeleteBuffers(1, &_copyVBO);
			gl::DeleteVertexArrays(1, &_copyVAO);
		});
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
		OpenGLQueue::GetSharedInstance()->SubmitCommand([=] {
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
		});
	}
	
	void Renderer32::DrawCameraStage(Camera *camera, Camera *stage)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([=] {
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
		});
	}
	
	// ---------------------
	// MARK: -
	// MARK: Rendering
	// ---------------------
	
	void Renderer32::DrawCamera(Camera *camera, Camera *source, size_t skyCubeMeshes)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			Renderer::DrawCamera(camera, source, skyCubeMeshes);
			
			bool changedCamera = true;
			bool changedShader;
			bool changedMaterial;
			
			SetDepthWriteEnabled(true);
			camera->PrepareForRendering(this);
			
			if(_currentMaterial)
				SetDepthWriteEnabled(_currentMaterial->depthwrite);
			
			bool wantsFog = (_currentCamera->GetFlags() & Camera::Flags::UseFog);
			bool wantsClipPlane = (_currentCamera->GetFlags() & Camera::Flags::UseClipPlanes);
			
			Matrix identityMatrix;
			
			if(!source && !(camera->GetFlags() & Camera::Flags::NoRender))
			{
				Material *surfaceMaterial = camera->GetMaterial();
				LightManager *lightManager = camera->GetLightManager();
				
				size_t lightDirectionalCount = 0;
				size_t lightPointSpotCount = 0;
				
				if(lightManager)
				{
					lightManager->CreateLightLists();
					
					lightPointSpotCount   = lightManager->GetSpotlightCount() + lightManager->GetPointLightCount();
					lightDirectionalCount = lightManager->GetDirectionalLightCount();
					
					_renderedLights += lightDirectionalCount + lightPointSpotCount;
				}
				
				// Update the shader
				const Matrix& projectionMatrix = camera->GetProjectionMatrix();
				const Matrix& inverseProjectionMatrix = camera->GetInverseProjectionMatrix();
				
				const Matrix& viewMatrix = camera->GetViewMatrix();
				const Matrix& inverseViewMatrix = camera->GetInverseViewMatrix();
				
				Matrix projectionViewMatrix = projectionMatrix * viewMatrix;
				Matrix inverseProjectionViewMatrix = inverseProjectionMatrix * inverseViewMatrix;
				
				size_t objectsCount = _frame.size();
				size_t i = (camera->GetFlags() & Camera::Flags::NoSky) ? skyCubeMeshes : 0;
				
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
					if(wantsInstancing)
					{
						if(!shader->SupportsProgramOfType(ShaderProgram::TypeInstanced))
							continue;
					}
					
					// Grab the correct shader program
					std::vector<ShaderDefine> defines;
					ShaderLookup lookup = material->GetLookup();
					ShaderProgram *program = 0;
					
					bool wantsDiscard = material->discard;
					if(surfaceMaterial && !(material->override & Material::OverrideDiscard))
						wantsDiscard = surfaceMaterial->discard;
					
					if(object.skeleton && shader->SupportsProgramOfType(ShaderProgram::TypeAnimated))
						lookup.type |= ShaderProgram::TypeAnimated;
					
					if(wantsFog && shader->SupportsProgramOfType(ShaderProgram::TypeFog))
						lookup.type |= ShaderProgram::TypeFog;
					
					if(wantsClipPlane && shader->SupportsProgramOfType(ShaderProgram::TypeClipPlane))
						lookup.type |= ShaderProgram::TypeClipPlane;
					
					if(wantsInstancing)
						lookup.type |= ShaderProgram::TypeInstanced;
					
					if(wantsDiscard && shader->SupportsProgramOfType(ShaderProgram::TypeDiscard))
						lookup.type |= ShaderProgram::TypeDiscard;
					
					if(material->GetTextures().GetCount() > 0 && shader->SupportsProgramOfType(ShaderProgram::TypeDiffuse))
						lookup.type |= ShaderProgram::TypeDiffuse;
					
					if(surfaceMaterial)
					{
						lookup = lookup + surfaceMaterial->GetLookup();
					}
					
					bool wantsLighting = (material->lighting && lightManager && shader->SupportsProgramOfType(ShaderProgram::TypeLighting));
					
					// The surface material can only override lighting if the shader supports lighting
					if(wantsLighting && surfaceMaterial)
						wantsLighting = surfaceMaterial->lighting;
					
					if(wantsLighting)
					{
						lookup.type |= ShaderProgram::TypeLighting;
						lookup.lightDirectionalCount = lightDirectionalCount;
						
						//TODO: fix
						if(lightPointSpotCount > 0)
							lookup.lightPointSpotCount = 1;//lightPointSpotCount;
						
						lightManager->AdjustShaderLookup(shader, lookup);
					}
					
					program = shader->GetProgramWithLookup(lookup);
					
					RN_ASSERT(program, "");
					
					changedShader   = (_currentProgram != program);
					changedMaterial = (_currentMaterial != material);
					
					BindMaterial(material, program);
					
					// Update the shader data
					if(changedShader || changedCamera)
					{
						UpdateShaderData();
						changedCamera = false;
					}
					
					if(changedShader || changedMaterial)
					{
						if(wantsLighting)
							lightManager->UpdateProgram(this, program);
						
						gl::Uniform4fv(program->ambient, 1, &material->ambient->r);
						gl::Uniform4fv(program->diffuse, 1, &material->diffuse->r);
						gl::Uniform4fv(program->emissive, 1, &material->emissive->r);
						gl::Uniform4fv(program->specular, 1, &material->specular->r);
						
						if(program->discardThreshold != -1)
						{
							float threshold = material->discardThreshold;
							
							if(surfaceMaterial && !(material->override & Material::OverrideDiscardThreshold))
								threshold = surfaceMaterial->discardThreshold;
							
							gl::Uniform1f(program->discardThreshold, threshold);
						}
						
						material->ApplyUniforms(program);
					}
					
					if(wantsInstancing)
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
				
				if(lightManager)
				{
					lightManager->ClearLights();
				}
			}
			else if(source)
			{
				DrawCameraStage(source, camera);
			}
		}, true);
	}
	
	void Renderer32::DrawMesh(Mesh *mesh, uint32 offset, uint32 count)
	{
		bool usesIndices = mesh->SupportsFeature(kMeshFeatureIndices);
		const MeshDescriptor *descriptor = usesIndices ? mesh->GetDescriptorForFeature(kMeshFeatureIndices) : mesh->GetDescriptorForFeature(kMeshFeatureVertices);
		
		BindVAO(std::tuple<ShaderProgram *, Mesh *>(_currentProgram, mesh));
		
		GLsizei glCount = static_cast<GLsizei>(usesIndices ? mesh->GetIndicesCount() : mesh->GetVerticesCount());
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
		
		BindVAO(0);
	}
	
	void Renderer32::DrawMeshInstanced(const RenderingObject& object)
	{
		Mesh *mesh = object.mesh;
		const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(kMeshFeatureIndices);
		
		BindVAO(std::tuple<ShaderProgram *, Mesh *>(_currentProgram, mesh));
		RN_ASSERT(_currentProgram->instancingData != -1, "");
		
		uint32 dataUnit = BindTexture(GL_TEXTURE_BUFFER, object.instancingData);
		uint32 indicesUnit = BindTexture(GL_TEXTURE_BUFFER, object.instancingIndices);
		
		gl::Uniform1i(_currentProgram->instancingData, dataUnit);
		gl::Uniform1i(_currentProgram->instancingIndices, indicesUnit);
		
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
			
			gl::DrawElementsInstanced(mesh->GetMode(), (GLsizei)mesh->GetIndicesCount(), type, 0, (GLsizei)object.count);
		}
		else
		{
			descriptor = mesh->GetDescriptorForFeature(kMeshFeatureVertices);
			gl::DrawArraysInstanced(mesh->GetMode(), 0, (GLsizei)mesh->GetVerticesCount(), (GLsizei)object.count);
		}
		
		BindVAO(0);
	}

	void Renderer32::BeginFrame(float delta)
	{
		if(!_hasValidFramebuffer)
		{
			OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
				gl::GenVertexArrays(1, &_copyVAO);
				gl::BindVertexArray(_copyVAO);
				gl::BindBuffer(GL_ARRAY_BUFFER, _copyVBO);
				gl::BindVertexArray(0);

				FlushAutoVAOs();
			});
		}

		Renderer::BeginFrame(delta);
	}
}
