//
//  RNRenderer.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderer.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNCamera.h"
#include "RNKernel.h"
#include "RNThreadPool.h"
#include "RNAlgorithm.h"
#include "RNSettings.h"

#define kRNRendererMaxVAOAge 300

namespace RN
{
	Renderer::Renderer()
	{
		_defaultFBO = 0;
		_defaultWidth = _defaultHeight = 0;
		_defaultWidthFactor = _defaultHeightFactor = 1.0f;
		
		_currentMaterial = 0;
		_currentProgram	 = 0;
		_currentCamera   = 0;
		_currentVAO      = 0;
		
		_scaleFactor = Kernel::GetSharedInstance()->GetActiveScaleFactor();
		_time = 0.0f;
		_mode = Mode::ModeWorld;
		
		_hdrExposure   = 1.0f;
		_hdrWhitePoint = 1.0f;
		
		// Default OpenGL state
		_cullingEnabled   = false;
		_depthTestEnabled = false;
		_blendingEnabled  = false;
		_depthWrite       = false;
		_polygonOffsetEnabled = false;
		_scissorTest      = false;
		
		_cullMode  = GL_CCW;
		_depthFunc = GL_LESS;
		
		_blendSource      = GL_ONE;
		_blendDestination = GL_ZERO;
		
		_polygonOffsetFactor = 0.0f;
		_polygonOffsetUnits  = 0.0f;
		
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDepthMask(GL_FALSE);
		
		glFrontFace(_cullMode);
		glDepthFunc(_depthFunc);
		glBlendFunc(_blendSource, _blendDestination);
		glPolygonOffset(_polygonOffsetFactor, _polygonOffsetUnits);
		
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, (GLint *)&_maxTextureUnits); RN_CHECKOPENGL();
		_textureUnit     = 0;
		_gammaCorrection = Settings::GetSharedInstance()->GetBoolForKey(kRNSettingsGammaCorrectionKey);
		
		_hasValidFramebuffer = false;
		_frameCamera = 0;
		
		MessageCenter::GetSharedInstance()->AddObserver(kRNWindowScaleFactorChanged, [this](Message *message) {
			_scaleFactor = Kernel::GetSharedInstance()->GetActiveScaleFactor();
		}, this);
	}
	
	Renderer::~Renderer()
	{
		MessageCenter::GetSharedInstance()->RemoveObserver(this);
	}
	
	void Renderer::SetDefaultFBO(GLuint fbo)
	{
		_defaultFBO = fbo;
		_hasValidFramebuffer = false;
	}
	
	void Renderer::SetDefaultFrame(uint32 width, uint32 height)
	{
		_defaultWidth  = width;
		_defaultHeight = height;
	}
	
	void Renderer::SetDefaultFactor(float width, float height)
	{
		_defaultWidthFactor = width;
		_defaultHeightFactor = height;
	}
	
	void Renderer::SetMode(Mode mode)
	{
		_mode = mode;
	}
	
	// ---------------------
	// MARK: -
	// MARK: Additional helper
	// ---------------------
	
	void Renderer::UpdateShaderData()
	{
		const Matrix& projectionMatrix = _currentCamera->projectionMatrix;
		const Matrix& inverseProjectionMatrix = _currentCamera->inverseProjectionMatrix;
		
		const Matrix& viewMatrix = _currentCamera->viewMatrix;
		const Matrix& inverseViewMatrix = _currentCamera->inverseViewMatrix;
		
		if(_currentProgram->frameSize != -1)
		{
			const Rect& frame = _currentCamera->GetFrame();
			glUniform4f(_currentProgram->frameSize, 1.0f/frame.width/_scaleFactor, 1.0f/frame.height/_scaleFactor, frame.width * _scaleFactor, frame.height * _scaleFactor);
		}
		
		if(_currentProgram->hdrSettings != -1)
			glUniform4f(_currentProgram->hdrSettings, _hdrExposure, _hdrWhitePoint, 0.0f, 0.0f);
		
		if(_currentProgram->clipPlanes != -1)
			glUniform2f(_currentProgram->clipPlanes, _currentCamera->clipnear, _currentCamera->clipfar);
		
		
		if(_currentProgram->fogPlanes != -1)
			glUniform2f(_currentProgram->fogPlanes, _currentCamera->fognear, 1.0f/(_currentCamera->fogfar-_currentCamera->fognear));
		
		if(_currentProgram->fogColor != -1)
			glUniform4fv(_currentProgram->fogColor, 1, &_currentCamera->fogcolor.r);
		
		if(_currentProgram->clipPlane != -1)
			glUniform4fv(_currentProgram->clipPlane, 1, &_currentCamera->clipplane.x);
		
		if(_currentProgram->matProj != -1)
			glUniformMatrix4fv(_currentProgram->matProj, 1, GL_FALSE, projectionMatrix.m);
		
		if(_currentProgram->matProjInverse != -1)
			glUniformMatrix4fv(_currentProgram->matProjInverse, 1, GL_FALSE, inverseProjectionMatrix.m);
		
		if(_currentProgram->matView != -1)
			glUniformMatrix4fv(_currentProgram->matView, 1, GL_FALSE, viewMatrix.m);
		
		if(_currentProgram->matViewInverse != -1)
			glUniformMatrix4fv(_currentProgram->matViewInverse, 1, GL_FALSE, inverseViewMatrix.m);
		
		if(_currentProgram->matProjView != -1)
		{
			Matrix projectionViewMatrix = projectionMatrix * viewMatrix;
			glUniformMatrix4fv(_currentProgram->matProjView, 1, GL_FALSE, projectionViewMatrix.m);
		}
		
		if(_currentProgram->matProjViewInverse != -1)
		{
			Matrix inverseProjectionViewMatrix = inverseProjectionMatrix * inverseViewMatrix;
			glUniformMatrix4fv(_currentProgram->matProjViewInverse, 1, GL_FALSE, inverseProjectionViewMatrix.m);
		}
		
		if(_currentProgram->viewPosition != -1)
		{
			const Vector3& position = _currentCamera->GetWorldPosition();
			glUniform3fv(_currentProgram->viewPosition, 1, &position.x);
		}
	}
	
	void Renderer::RelinquishMesh(Mesh *mesh)
	{
		for(auto i=_autoVAOs.begin(); i!=_autoVAOs.end();)
		{
			if(std::get<1>(i->first) == mesh)
			{
				GLuint vao = std::get<0>(i->second);
				if(vao == _currentVAO)
					BindVAO(0);
				
				glDeleteVertexArrays(1, &vao);
				
				i = _autoVAOs.erase(i);
				continue;
			}
			
			i ++;
		}
	}
	
	void Renderer::RelinquishProgram(ShaderProgram *program)
	{
		for(auto i=_autoVAOs.begin(); i!=_autoVAOs.end();)
		{
			if(std::get<0>(i->first) == program)
			{
				GLuint vao = std::get<0>(i->second);
				if(vao == _currentVAO)
					BindVAO(0);
				
				glDeleteVertexArrays(1, &vao);
				
				i = _autoVAOs.erase(i);
				continue;
			}
			
			i ++;
		}
	}
		
	// ---------------------
	// MARK: -
	// MARK: Binding
	// ---------------------
	
	void Renderer::BindVAO(const std::tuple<ShaderProgram *, Mesh *>& tuple)
	{
		auto iterator = _autoVAOs.find(tuple);
		GLuint vao;
		
		if(iterator == _autoVAOs.end())
		{
			ShaderProgram *shader = std::get<0>(tuple);
			Mesh *mesh = std::get<1>(tuple);
			
			GLuint vbo = mesh->GetVBO();
			GLuint ibo = mesh->GetIBO();
			
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
			
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			
			if(mesh->SupportsFeature(kMeshFeatureIndices))
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			
			// Vertices
			if(shader->attPosition != -1 && mesh->SupportsFeature(kMeshFeatureVertices))
			{
				MeshDescriptor *descriptor = mesh->GetDescriptor(kMeshFeatureVertices);
				size_t offset = mesh->OffsetForFeature(kMeshFeatureVertices);
				
				glEnableVertexAttribArray(shader->attPosition);
				glVertexAttribPointer(shader->attPosition, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
			}
			
			// Normals
			if(shader->attNormal != -1 && mesh->SupportsFeature(kMeshFeatureNormals))
			{
				MeshDescriptor *descriptor = mesh->GetDescriptor(kMeshFeatureNormals);
				size_t offset = mesh->OffsetForFeature(kMeshFeatureNormals);
				
				glEnableVertexAttribArray(shader->attNormal);
				glVertexAttribPointer(shader->attNormal, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
			}
			
			// Tangents
			if(shader->attTangent != -1 && mesh->SupportsFeature(kMeshFeatureTangents))
			{
				MeshDescriptor *descriptor = mesh->GetDescriptor(kMeshFeatureTangents);
				size_t offset = mesh->OffsetForFeature(kMeshFeatureTangents);
				
				glEnableVertexAttribArray(shader->attTangent);
				glVertexAttribPointer(shader->attTangent, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
			}
			
			// Texcoord0
			if(shader->attTexcoord0 != -1 && mesh->SupportsFeature(kMeshFeatureUVSet0))
			{
				MeshDescriptor *descriptor = mesh->GetDescriptor(kMeshFeatureUVSet0);
				size_t offset = mesh->OffsetForFeature(kMeshFeatureUVSet0);
				
				glEnableVertexAttribArray(shader->attTexcoord0);
				glVertexAttribPointer(shader->attTexcoord0, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
			}
			
			// Texcoord1
			if(shader->attTexcoord1 != -1 && mesh->SupportsFeature(kMeshFeatureUVSet1))
			{
				MeshDescriptor *descriptor = mesh->GetDescriptor(kMeshFeatureUVSet1);
				size_t offset = mesh->OffsetForFeature(kMeshFeatureUVSet1);
				
				glEnableVertexAttribArray(shader->attTexcoord1);
				glVertexAttribPointer(shader->attTexcoord1, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
			}
			
			// Color0
			if(shader->attColor0 != -1 && mesh->SupportsFeature(kMeshFeatureColor0))
			{
				MeshDescriptor *descriptor = mesh->GetDescriptor(kMeshFeatureColor0);
				size_t offset = mesh->OffsetForFeature(kMeshFeatureColor0);
				
				glEnableVertexAttribArray(shader->attColor0);
				glVertexAttribPointer(shader->attColor0, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
			}
			
			// Color1
			if(shader->attColor1 != -1 && mesh->SupportsFeature(kMeshFeatureColor1))
			{
				MeshDescriptor *descriptor = mesh->GetDescriptor(kMeshFeatureColor1);
				size_t offset = mesh->OffsetForFeature(kMeshFeatureColor1);
				
				glEnableVertexAttribArray(shader->attColor1);
				glVertexAttribPointer(shader->attColor1, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
			}
			
			// Bone Weights
			if(shader->attBoneWeights != -1 && mesh->SupportsFeature(kMeshFeatureBoneWeights))
			{
				MeshDescriptor *descriptor = mesh->GetDescriptor(kMeshFeatureBoneWeights);
				size_t offset = mesh->OffsetForFeature(kMeshFeatureBoneWeights);
				
				glEnableVertexAttribArray(shader->attBoneWeights);
				glVertexAttribPointer(shader->attBoneWeights, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
			}
			
			// Bone Indices
			if(shader->attBoneIndices != -1 && mesh->SupportsFeature(kMeshFeatureBoneIndices))
			{
				MeshDescriptor *descriptor = mesh->GetDescriptor(kMeshFeatureBoneIndices);
				size_t offset = mesh->OffsetForFeature(kMeshFeatureBoneIndices);
				
				glEnableVertexAttribArray(shader->attBoneIndices);
				glVertexAttribPointer(shader->attBoneIndices, descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
			}
			
			_autoVAOs[tuple] = std::tuple<GLuint, uint32>(vao, 0);
			_currentVAO = vao;
			
			return;
		}
		
		uint32& age = std::get<1>(iterator->second);
		
		vao = std::get<0>(iterator->second);
		age = 0;
		
		BindVAO(vao);
	}
	
	void Renderer::BindMaterial(Material *material, ShaderProgram *program)
	{
		bool changedShader = (program != _currentProgram);
		UseShader(program);
		
		Material *surfaceMaterial = _currentCamera->GetMaterial();
		if(!surfaceMaterial)
			surfaceMaterial = material;
		
		if(changedShader || material != _currentMaterial)
		{
			_textureUnit = 0;
			
			const Array& textures = !(surfaceMaterial->override & Material::OverrideTextures)? material->GetTextures() : surfaceMaterial->GetTextures();
			const std::vector<GLuint>& textureLocations = program->texlocations;
			
			if(textureLocations.size() > 0)
			{
				size_t textureCount = MIN(textureLocations.size(), textures.GetCount());
				
				for(size_t i=0; i<textureCount; i++)
				{
					GLint location = textureLocations[i];
					Texture *texture = textures.GetObjectAtIndex<Texture>(i);
					
					glUniform1i(location, BindTexture(texture));
					
					location = program->texinfolocations[i];
					if(location != -1)
						glUniform4f(location, 1.0f/static_cast<float>(texture->GetWidth()), 1.0f/static_cast<float>(texture->GetHeight()), texture->GetWidth(), texture->GetHeight());
				}
			}
		}

#define PickAttribute(_override, attribute) (material->override & Material::_override) ? material->attribute : surfaceMaterial->attribute
		
		SetCullingEnabled(PickAttribute(OverrideCulling, culling));
		SetCullMode(PickAttribute(OverrideCullmode, cullmode));

		SetDepthTestEnabled(PickAttribute(OverrideDepthtest, depthtest));
		SetDepthFunction(PickAttribute(OverrideDepthtestMode, depthtestmode));
		
		SetPolygonOffsetEnabled(PickAttribute(OverridePolygonOffset, polygonOffset));
		SetPolygonOffset(PickAttribute(OverridePolygonOffset, polygonOffsetFactor), PickAttribute(OverridePolygonOffset, polygonOffsetUnits));
		
		if(material->override & Material::OverrideDepthwrite)
		{
			SetDepthWriteEnabled((material->depthwrite && _currentCamera->AllowsDepthWrite()));
		}
		else
		{
			SetDepthWriteEnabled((surfaceMaterial->depthwrite && _currentCamera->AllowsDepthWrite()));
		}
		
		SetBlendingEnabled(PickAttribute(OverrideBlending, blending));
		
		if(material->override & Material::OverrideBlendmode)
		{
			SetBlendFunction(material->blendSource, material->blendDestination);
		}
		else
		{
			SetBlendFunction(surfaceMaterial->blendSource, surfaceMaterial->blendDestination);
		}
		
#undef PickAttribute
		
		_currentMaterial = material;
	}
	
	// ---------------------
	// MARK: -
	// MARK: Frame handling
	// ---------------------
	
	void Renderer::BeginFrame(float delta)
	{
		_time += delta;
		
		_renderedLights   = 0;
		_renderedVertices = 0;
		
		for(auto i=_autoVAOs.begin(); i!=_autoVAOs.end();)
		{
			uint32& age = std::get<1>(i->second);
			
			if((++ age) > kRNRendererMaxVAOAge)
			{
				i = _autoVAOs.erase(i);
				continue;
			}
			
			i ++;
		}
	}
	
	void Renderer::FinishFrame()
	{
		if(!_hasValidFramebuffer)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, _defaultFBO);
			
			if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				_flushCameras.clear();
				_flushedCameras.clear();
				
				return;
			}
			
			_hasValidFramebuffer = true;
		}
		
		SetScissorEnabled(false);
		
		glBindFramebuffer(GL_FRAMEBUFFER, _defaultFBO);
		
		glViewport(0, 0, _defaultWidth * _scaleFactor, _defaultHeight * _scaleFactor);
		glClear(GL_COLOR_BUFFER_BIT);
		
		for(auto iterator=_flushCameras.begin(); iterator!=_flushCameras.end(); iterator++)
		{
			Camera *camera = iterator->first;
			Shader *shader = iterator->second;
			
			FlushCamera(camera, shader);
		}
		
		_flushCameras.clear();
		_flushedCameras.clear();
		
		_debugFrameUI.clear();
		_debugFrameWorld.clear();
	}
	
	void Renderer::FlushCamera(Camera *camera, Shader *drawShader)
	{
		_currentCamera = camera;
		_textureUnit   = 0;
		
		SetDepthTestEnabled(false);
		SetCullMode(GL_CCW);
		
		if(camera->UseBlending())
		{
			SetBlendingEnabled(true);
			SetBlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			SetBlendingEnabled(false);
		}
		
		uint32 type = ShaderProgram::TypeNormal;		

		if(_gammaCorrection && drawShader->SupportsProgramOfType(ShaderProgram::TypeGammaCorrection))
			type = ShaderProgram::TypeGammaCorrection;
		
		ShaderProgram *program = drawShader->GetProgramOfType(type);
		
		UseShader(program);
		UpdateShaderData();
		
		uint32 targetmaps = MIN((uint32)program->targetmaplocations.size(), camera->GetRenderTargetCount());
		if(targetmaps >= 1)
		{
			Texture *texture = camera->GetRenderTarget(0);
			GLuint location  = program->targetmaplocations.front();
			
			glUniform1i(location, BindTexture(texture));
			
			location = program->targetmapinfolocations.front();
			if(location != -1)
				glUniform4f(location, 1.0f/static_cast<float>(texture->GetWidth()), 1.0f/static_cast<float>(texture->GetHeight()), texture->GetWidth(), texture->GetHeight());
		}
	}
	
	// Camera gets drawn into stage
	void Renderer::DrawCameraStage(Camera *camera, Camera *stage)
	{
		Material *material = stage->GetMaterial();
		ShaderProgram *program = material->GetShader()->GetProgramWithLookup(material->GetLookup() + ShaderLookup(ShaderProgram::TypeNormal));
		
		_currentCamera = stage;
		_textureUnit = 0;
		
		SetDepthTestEnabled(false);
		SetBlendingEnabled(false);
		SetCullMode(GL_CCW);
		
		BindMaterial(material, program);
		UpdateShaderData();
		
		material->ApplyUniforms(program);
		
		uint32 targetmaps = MIN((uint32)program->targetmaplocations.size(), stage->GetRenderTargetCount());
		for(uint32 i=0; i<targetmaps; i++)
		{
			Texture *texture = camera->GetRenderTarget(i);
			GLuint location  = program->targetmaplocations[i];
			
			glUniform1i(location, BindTexture(texture));
			
			location = program->targetmapinfolocations[i];
			if(location != -1)
				glUniform4f(location, 1.0f/static_cast<float>(texture->GetWidth()), 1.0f/static_cast<float>(texture->GetHeight()), texture->GetWidth(), texture->GetHeight());
		}
		
		if(program->depthmap != -1)
		{
			Texture *depthmap = camera->GetStorage()->GetDepthTarget();
			if(depthmap)
			{
				glUniform1i(program->depthmap, BindTexture(depthmap));
				
				if(program->depthmapinfo != -1)
					glUniform4f(program->depthmapinfo, 1.0f/static_cast<float>(depthmap->GetWidth()), 1.0f/static_cast<float>(depthmap->GetHeight()), depthmap->GetWidth(), depthmap->GetHeight());
			}
		}
	}
	
	// ---------------------
	// MARK: -
	// MARK: Rendering
	// ---------------------
	
	void Renderer::BeginCamera(Camera *camera)
	{
		RN_ASSERT(_frameCamera == 0, "");
		_frameCamera = camera;
		
		_currentProgram  = 0;
		_currentMaterial = 0;
		_currentCamera   = 0;
		_currentVAO      = 0;
		_textureUnit     = 0;
	}
	
	void Renderer::DrawCamera(Camera *camera, Camera *source, uint32 skyCubeMeshes)
	{
		if(!source)
		{
			// Sort the objects
			if(!(camera->GetFlags() & Camera::FlagNoSorting))
			{
				auto begin = _frame.begin();
				std::advance(begin, skyCubeMeshes);
			
				ParallelSort(begin, _frame.end(), [](const RenderingObject& a, const RenderingObject& b) {
					const Material *materialA = a.material;
					const Material *materialB = b.material;
					
					if(materialA->blending != materialB->blending)
					{
						if(!materialB->blending)
							return false;
						
						return true;
					}
					
					if(materialA->discard != materialB->discard)
					{
						if(!materialB->discard)
							return false;
						
						return true;
					}
					
					if(materialA->GetShader() != materialB->GetShader())
					{
						return (materialA->GetShader() < materialB->GetShader());
					}
					
					return a.mesh < b.mesh;
				});
			}

			Material *surfaceMaterial = camera->GetMaterial();
			if(!surfaceMaterial)
			{
				switch(_mode)
				{
					case Mode::ModeWorld:
						_frame.insert(_frame.end(), _debugFrameWorld.begin(), _debugFrameWorld.end());
						break;
						
					case Mode::ModeUI:
						_frame.insert(_frame.end(), _debugFrameUI.begin(), _debugFrameUI.end());
						break;
				}
			}
		}
		
		_currentCamera = camera;
	}
	
	void Renderer::FinishCamera()
	{
		Camera *previous = _frameCamera;
		Camera *camera = _frameCamera;
		
		// Skycube
		Model *skyCube = camera->GetSkyCube();
		uint32 skyCubeMeshes = 0;
		
		Matrix cameraRotation;
		cameraRotation.MakeRotate(camera->GetWorldRotation());
		
		if(skyCube)
		{
			skyCubeMeshes = skyCube->GetMeshCount(0);
			
			for(uint32 j=0; j<skyCubeMeshes; j++)
			{
				RenderingObject object;
				
				object.mesh = skyCube->GetMeshAtIndex(0, j);
				object.material = skyCube->GetMaterialAtIndex(0, j);
				object.transform = &cameraRotation;
				object.skeleton = 0;
				
				_frame.insert(_frame.begin(), std::move(object));
			}
		}
		
		ParallelSort(_directionalLights.begin(), _directionalLights.end(), [](Light *a, Light *b) {
			return (a->Shadow());
        });
		
		// Render loop
		DrawCamera(camera, 0, skyCubeMeshes);
		Camera *lastPipeline = camera;
		
		if(camera->GetFlags() & Camera::FlagForceFlush)
		{
			if(_flushedCameras.find(camera) == _flushedCameras.end())
			{
				_flushCameras.push_back(std::pair<Camera *, Shader *>(camera, camera->GetDrawFramebufferShader()));
				_flushedCameras.insert(camera);
			}
		}
		
		auto pipelines = camera->GetPostProcessingPipelines();
		
		for(PostProcessingPipeline *pipeline : pipelines)
		{
			for(auto j=pipeline->stages.begin(); j!=pipeline->stages.end(); j++)
			{
				Camera *stage = j->GetCamera();
				
				switch(j->GetMode())
				{
					case RenderStage::Mode::ReRender:
					case RenderStage::Mode::ReRender_NoRemoval:
						DrawCamera(stage, 0, skyCubeMeshes);
						break;
						
					case RenderStage::Mode::ReUseConnection:
					case RenderStage::Mode::ReUseConnection_NoRemoval:
						DrawCamera(stage, j->GetConnection(), skyCubeMeshes);
						break;
						
					case RenderStage::Mode::ReUseCamera:
					case RenderStage::Mode::ReUseCamera_NoRemoval:
						DrawCamera(stage, camera, skyCubeMeshes);
						break;
						
					case RenderStage::Mode::ReUsePipeline:
					case RenderStage::Mode::ReUsePipeline_NoRemoval:
						DrawCamera(stage, lastPipeline, skyCubeMeshes);
						break;
						
					case RenderStage::Mode::ReUsePreviousStage:
					case RenderStage::Mode::ReUsePreviousStage_NoRemoval:
						DrawCamera(stage, previous, skyCubeMeshes);
						break;
				}
				
				previous = stage;
				
				if(previous->GetFlags() & Camera::FlagForceFlush)
				{
					if(_flushedCameras.find(camera) == _flushedCameras.end())
					{
						_flushCameras.push_back(std::pair<Camera *, Shader *>(previous, camera->GetDrawFramebufferShader()));
						_flushedCameras.insert(previous);
					}
				}
			}
			
			lastPipeline = previous;
		}
		
		if(!(previous->GetFlags() & Camera::FlagHidden) && _flushedCameras.find(camera) == _flushedCameras.end())
		{
			_flushCameras.push_back(std::pair<Camera *, Shader *>(previous, camera->GetDrawFramebufferShader()));
			_flushedCameras.insert(previous);
		}
		
		// Cleanup of the frame
		_frameCamera = 0;
		
		_frame.clear();
		_pointLights.clear();
		_spotLights.clear();
		_directionalLights.clear();
	}
	
	
	
	void Renderer::RenderObject(RenderingObject object)
	{
		_frame.push_back(std::move(object));
	}
	
	void Renderer::RenderDebugObject(RenderingObject object, Mode mode)
	{
		switch(mode)
		{
			case Mode::ModeWorld:
				_debugFrameWorld.push_back(std::move(object));
				break;
				
			case Mode::ModeUI:
				_debugFrameUI.push_back(std::move(object));
				break;
		}		
	}
	
	void Renderer::RenderLight(Light *light)
	{
		switch(light->GetType())
		{
			case Light::Type::PointLight:
				_pointLights.push_back(light);
				break;
				
			case Light::Type::SpotLight:
				_spotLights.push_back(light);
				break;
				
			case Light::Type::DirectionalLight:
				_directionalLights.push_back(light);
				break;
				
			default:
				break;
		}
	}
}
