//
//  RNRenderer.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <png.h>
#include "RNRenderer.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNCamera.h"
#include "RNKernel.h"
#include "RNThreadPool.h"
#include "RNAlgorithm.h"
#include "RNSettings.h"
#include "RNLogging.h"
#include "RNOpenGLQueue.h"

#define kRNRendererMaxVAOAge 300

namespace RN
{
	RNDeclareSingleton(Renderer)
	
	Renderer::Renderer()
	{
		MakeShared();
		
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
		
		_captureAge = static_cast<FrameID>(-1);
		_captureIndex = 0;
		
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
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			gl::GetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, (GLint *)&_maxTextureUnits);
		}, true);
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
			gl::Disable(GL_CULL_FACE);
			gl::Disable(GL_DEPTH_TEST);
			gl::Disable(GL_BLEND);
			gl::Disable(GL_POLYGON_OFFSET_FILL);
			gl::DepthMask(GL_FALSE);
			
			gl::FrontFace(_cullMode);
			gl::DepthFunc(_depthFunc);
			gl::BlendFunc(_blendSource, _blendDestination);
			gl::PolygonOffset(_polygonOffsetFactor, _polygonOffsetUnits);
			
			gl::GenBuffers(2, _capturePBO);
			_captureBufferSize[0] = 0;
			_captureBufferSize[1] = 0;
		});
		
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
			gl::Uniform4f(_currentProgram->frameSize, 1.0f/frame.width/_scaleFactor, 1.0f/frame.height/_scaleFactor, frame.width * _scaleFactor, frame.height * _scaleFactor);
		}
		
		if(_currentProgram->hdrSettings != -1)
			gl::Uniform4f(_currentProgram->hdrSettings, _hdrExposure, _hdrWhitePoint, 0.0f, 0.0f);
		
		if(_currentProgram->clipPlanes != -1)
			gl::Uniform2f(_currentProgram->clipPlanes, _currentCamera->clipnear, _currentCamera->clipfar);
		
		
		if(_currentProgram->fogPlanes != -1)
			gl::Uniform2f(_currentProgram->fogPlanes, _currentCamera->fognear, 1.0f/(_currentCamera->fogfar-_currentCamera->fognear));
		
		if(_currentProgram->fogColor != -1)
			gl::Uniform4fv(_currentProgram->fogColor, 1, &_currentCamera->fogcolor.r);
		
		if(_currentProgram->cameraAmbient != -1)
			gl::Uniform4fv(_currentProgram->cameraAmbient, 1, &_currentCamera->ambient.x);
		
		if(_currentProgram->clipPlane != -1)
			gl::Uniform4fv(_currentProgram->clipPlane, 1, &_currentCamera->clipplane.x);
		
		if(_currentProgram->matProj != -1)
			gl::UniformMatrix4fv(_currentProgram->matProj, 1, GL_FALSE, projectionMatrix.m);
		
		if(_currentProgram->matProjInverse != -1)
			gl::UniformMatrix4fv(_currentProgram->matProjInverse, 1, GL_FALSE, inverseProjectionMatrix.m);
		
		if(_currentProgram->matView != -1)
			gl::UniformMatrix4fv(_currentProgram->matView, 1, GL_FALSE, viewMatrix.m);
		
		if(_currentProgram->matViewInverse != -1)
			gl::UniformMatrix4fv(_currentProgram->matViewInverse, 1, GL_FALSE, inverseViewMatrix.m);
		
		if(_currentProgram->matProjView != -1)
		{
			Matrix projectionViewMatrix = projectionMatrix * viewMatrix;
			gl::UniformMatrix4fv(_currentProgram->matProjView, 1, GL_FALSE, projectionViewMatrix.m);
		}
		
		if(_currentProgram->matProjViewInverse != -1)
		{
			Matrix inverseProjectionViewMatrix = inverseProjectionMatrix * inverseViewMatrix;
			gl::UniformMatrix4fv(_currentProgram->matProjViewInverse, 1, GL_FALSE, inverseProjectionViewMatrix.m);
		}
		
		if(_currentProgram->viewPosition != -1)
		{
			const Vector3& position = _currentCamera->GetWorldPosition();
			gl::Uniform3fv(_currentProgram->viewPosition, 1, &position.x);
		}
		
		if(_currentProgram->viewNormal != -1)
		{
			const Vector3& forward = _currentCamera->Forward();
			gl::Uniform3fv(_currentProgram->viewNormal, 1, &forward.x);
		}
	}
	
	void Renderer::RelinquishMesh(Mesh *mesh)
	{
		for(auto i=_autoVAOs.begin(); i!=_autoVAOs.end();)
		{
			if(std::get<1>(i->first) == mesh)
			{
				GLuint vao = std::get<0>(i->second);
				
				OpenGLQueue::GetSharedInstance()->SubmitCommand([this, vao] {
					if(vao == _currentVAO)
						BindVAO(0);
					
					gl::DeleteVertexArrays(1, &vao);
				});
				
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
				
				OpenGLQueue::GetSharedInstance()->SubmitCommand([this, vao] {
					if(vao == _currentVAO)
						BindVAO(0);
					
					gl::DeleteVertexArrays(1, &vao);
				});
				
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

	void Renderer::FlushAutoVAOs()
	{
		_autoVAOs.clear();
	}
	
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
			
			OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
				gl::GenVertexArrays(1, &vao);
				gl::BindVertexArray(vao);
				
				gl::BindBuffer(GL_ARRAY_BUFFER, vbo);
				
				if(mesh->SupportsFeature(kMeshFeatureIndices))
					gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
				
				// Vertices
				if(shader->attPosition != -1 && mesh->SupportsFeature(kMeshFeatureVertices))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(kMeshFeatureVertices);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attPosition);
					gl::VertexAttribPointer(shader->attPosition, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Normals
				if(shader->attNormal != -1 && mesh->SupportsFeature(kMeshFeatureNormals))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(kMeshFeatureNormals);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attNormal);
					gl::VertexAttribPointer(shader->attNormal, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Tangents
				if(shader->attTangent != -1 && mesh->SupportsFeature(kMeshFeatureTangents))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(kMeshFeatureTangents);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attTangent);
					gl::VertexAttribPointer(shader->attTangent, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Texcoord0
				if(shader->attTexcoord0 != -1 && mesh->SupportsFeature(kMeshFeatureUVSet0))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(kMeshFeatureUVSet0);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attTexcoord0);
					gl::VertexAttribPointer(shader->attTexcoord0, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Texcoord1
				if(shader->attTexcoord1 != -1 && mesh->SupportsFeature(kMeshFeatureUVSet1))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(kMeshFeatureUVSet1);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attTexcoord1);
					gl::VertexAttribPointer(shader->attTexcoord1, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Color0
				if(shader->attColor0 != -1 && mesh->SupportsFeature(kMeshFeatureColor0))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(kMeshFeatureColor0);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attColor0);
					gl::VertexAttribPointer(shader->attColor0, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Color1
				if(shader->attColor1 != -1 && mesh->SupportsFeature(kMeshFeatureColor1))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(kMeshFeatureColor1);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attColor1);
					gl::VertexAttribPointer(shader->attColor1, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Bone Weights
				if(shader->attBoneWeights != -1 && mesh->SupportsFeature(kMeshFeatureBoneWeights))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(kMeshFeatureBoneWeights);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attBoneWeights);
					gl::VertexAttribPointer(shader->attBoneWeights, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Bone Indices
				if(shader->attBoneIndices != -1 && mesh->SupportsFeature(kMeshFeatureBoneIndices))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(kMeshFeatureBoneIndices);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attBoneIndices);
					gl::VertexAttribPointer(shader->attBoneIndices, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
			}, true);
			
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
				size_t textureCount = std::min(textureLocations.size(), textures.GetCount());
				
				for(size_t i=0; i<textureCount; i++)
				{
					GLint location = textureLocations[i];
					Texture *texture = textures.GetObjectAtIndex<Texture>(i);
					
					gl::Uniform1i(location, BindTexture(texture));
					
					location = program->texinfolocations[i];
					if(location != -1)
						gl::Uniform4f(location, 1.0f/static_cast<float>(texture->GetWidth()), 1.0f/static_cast<float>(texture->GetHeight()), texture->GetWidth(), texture->GetHeight());
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
	// MARK: FrameCapture
	// ---------------------
	
	FrameCapture::FrameCapture()
	{}
	
	FrameCapture::~FrameCapture()
	{
		delete [] _data;
	}
	
	void CopyPNGData(png_structp pngPointer, png_bytep pngData, png_size_t length)
	{
		Data *data = reinterpret_cast<Data *>(png_get_io_ptr(pngPointer));
		data->Append(pngData, length);
	}
	
	Data *FrameCapture::GetData(Format format)
	{
		switch(format)
		{
			case Format::RGBA8888:
				return Data::WithBytes(_data, _width * _height * 4);
				break;
				
			case Format::RGB888:
			{
				size_t size = _width * _height;
				uint8 *temp = new uint8[size * 3];
				Data *data = new Data(temp, size, true, true);
				
				uint32 *pixel = reinterpret_cast<uint32 *>(_data);
				uint32 *end = pixel + size;
				
				while(pixel != end)
				{
					*temp ++ = (*pixel >> 24) & 0xff;
					*temp ++ = (*pixel >> 16) & 0xff;
					*temp ++ = (*pixel >> 8) & 0xff;
					
					pixel ++;
				}
				
				return data->Autorelease();
			}
				
			case Format::PNG:
			{
				Data *data = new Data();
				
				png_structp pngPointer = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
				png_infop pngInfo      = png_create_info_struct(pngPointer);
				
				png_set_IHDR(pngPointer, pngInfo, static_cast<uint32>(_width), static_cast<uint32>(_height), 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
				
				uint32 bytesPerRow = static_cast<uint32>(_width * 4);
				png_byte **rows = reinterpret_cast<png_byte **>(png_malloc(pngPointer, _height * sizeof(png_byte *)));
				
				uint32 *pixel = reinterpret_cast<uint32 *>(_data);
				
				for(size_t y = 0; y < _height; y ++)
				{
					rows[y] = reinterpret_cast<png_byte *>(png_malloc(pngPointer, bytesPerRow));
					
					std::copy(pixel, pixel + _width, reinterpret_cast<uint32 *>(rows[y]));
					pixel += _width;
				}
				
				png_set_write_fn(pngPointer, data, CopyPNGData, nullptr);
				png_set_rows(pngPointer, pngInfo, rows);
				png_write_png(pngPointer, pngInfo, PNG_TRANSFORM_IDENTITY, nullptr);
				
				for(size_t y = 0; y < _height; y ++)
					png_free(pngPointer, rows[y]);
				
				png_free(pngPointer, rows);
				png_destroy_write_struct(&pngPointer, &pngInfo);
				
				return data->Autorelease();
			}
		}
	}
	
	void Renderer::FullfilPromises(void *data)
	{
		FrameCapture *capture = new FrameCapture();
		capture->_width  = std::get<0>(_captureSize);
		capture->_height = std::get<1>(_captureSize);
		capture->_frame  = _captureAge;
		
		size_t size = capture->_width * capture->_height * 4;
		capture->_data = new uint8[size];
		
		std::vector<std::function<void (FrameCapture *)>> *requests = new std::vector<std::function<void (FrameCapture *)>>();
		std::swap(_capturePromises, *requests);
		
		size_t rowBytes = capture->_width * 4;
		
		uint8 *sourceRow = reinterpret_cast<uint8 *>(data);
		uint8 *targetRow = capture->_data + (rowBytes * (capture->_height - 1));
		
		for(size_t i = 0; i < capture->_height; i ++)
		{
			std::copy(sourceRow, sourceRow + rowBytes, targetRow);
			
			targetRow -= rowBytes;
			sourceRow += rowBytes;
		}
		
		RN::ThreadPool::GetSharedInstance()->AddTask([=] {
			
			size_t size = requests->size();
			for(size_t i = 0; i < size; i ++)
			{
				requests->at(i)(capture);
			}
			
			delete requests;
			capture->Release();
		});
	}
	
	void Renderer::CaptureFrame()
	{
		_captureIndex = (_captureIndex + 1) % 2;
		size_t previous = (_captureIndex + 1) % 2;
		
		if(!_capturePromises.empty())
		{
			size_t bufferSize = _defaultWidth * _defaultHeight * _scaleFactor * _scaleFactor * 4;
			
			gl::BindBuffer(GL_PIXEL_PACK_BUFFER, _capturePBO[_captureIndex]);
			
			if(bufferSize > _captureBufferSize[_captureIndex])
			{
				gl::BufferData(GL_PIXEL_PACK_BUFFER, bufferSize, nullptr, GL_STATIC_READ);
				_captureBufferSize[_captureIndex] = bufferSize;
			}
			
			gl::ReadBuffer(GL_FRONT);
			gl::ReadPixels(0, 0, _defaultWidth * _scaleFactor, _defaultHeight * _scaleFactor, GL_RGBA, GL_UNSIGNED_BYTE, 0);
			
			FrameID current = Kernel::GetSharedInstance()->GetCurrentFrame();
			
			if(_captureAge == (current - 1))
			{
				gl::BindBuffer(GL_PIXEL_PACK_BUFFER, _capturePBO[previous]);
				
				void *buffer = gl::MapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
				if(buffer)
				{
					FullfilPromises(buffer);
					gl::UnmapBuffer(GL_PIXEL_PACK_BUFFER);
				}
			}
			
			_captureSize = std::make_pair(_defaultWidth * _scaleFactor, _defaultHeight * _scaleFactor);
			_captureAge = current;
			
			gl::BindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		}
	}
	
	void Renderer::RequestFrameCapture(std::function<void (FrameCapture *)> &&capture)
	{
		_capturePromises.push_back(std::move(capture));
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
				GLuint vao = std::get<1>(i->second);
				
				OpenGLQueue::GetSharedInstance()->SubmitCommand([vao] {
					gl::DeleteVertexArrays(1, &vao);
				});
				
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
			OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			
				gl::BindFramebuffer(GL_FRAMEBUFFER, _defaultFBO);
				
				if(gl::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				{
					_flushCameras.clear();
					_flushedCameras.clear();
					
					_debugFrameUI.clear();
					_debugFrameWorld.clear();
					
					RNDebug("Skipping frame while waiting for complete FBO to arrive");
					return;
				}
				
				_hasValidFramebuffer = true;
			}, true);
			
			if(!_hasValidFramebuffer)
				return;
		}
		
		std::vector<std::pair<Camera *, Shader *>> cameras;
		std::swap(cameras, _flushCameras);
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this, cameras] {
			SetScissorEnabled(false);
			
			gl::BindFramebuffer(GL_FRAMEBUFFER, _defaultFBO);
			
			gl::Viewport(0, 0, _defaultWidth * _scaleFactor, _defaultHeight * _scaleFactor);
			gl::Clear(GL_COLOR_BUFFER_BIT);
			
			for(auto iterator = cameras.begin(); iterator != cameras.end(); iterator++)
			{
				Camera *camera = iterator->first;
				Shader *shader = iterator->second;
				
				FlushCamera(camera, shader);
			}
			
			if(!_capturePromises.empty())
				CaptureFrame();
		});
		
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
		
		uint32 targetmaps = std::min((uint32)program->targetmaplocations.size(), camera->GetRenderTargetCount());
		if(targetmaps >= 1)
		{
			Texture *texture = camera->GetRenderTarget(0);
			GLuint location  = program->targetmaplocations.front();
			
			gl::Uniform1i(location, BindTexture(texture));
			
			location = program->targetmapinfolocations.front();
			if(location != -1)
				gl::Uniform4f(location, 1.0f/static_cast<float>(texture->GetWidth()), 1.0f/static_cast<float>(texture->GetHeight()), texture->GetWidth(), texture->GetHeight());
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
		
		uint32 targetmaps = std::min((uint32)program->targetmaplocations.size(), stage->GetRenderTargetCount());
		for(uint32 i=0; i<targetmaps; i++)
		{
			Texture *texture = camera->GetRenderTarget(i);
			GLuint location  = program->targetmaplocations[i];
			
			gl::Uniform1i(location, BindTexture(texture));
			
			location = program->targetmapinfolocations[i];
			if(location != -1)
				gl::Uniform4f(location, 1.0f/static_cast<float>(texture->GetWidth()), 1.0f/static_cast<float>(texture->GetHeight()), texture->GetWidth(), texture->GetHeight());
		}
		
		if(program->depthmap != -1)
		{
			Texture *depthmap = camera->GetStorage()->GetDepthTarget();
			if(depthmap)
			{
				gl::Uniform1i(program->depthmap, BindTexture(depthmap));
				
				if(program->depthmapinfo != -1)
					gl::Uniform4f(program->depthmapinfo, 1.0f/static_cast<float>(depthmap->GetWidth()), 1.0f/static_cast<float>(depthmap->GetHeight()), depthmap->GetWidth(), depthmap->GetHeight());
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
	
	void Renderer::DrawCamera(Camera *camera, Camera *source, size_t skyCubeMeshes)
	{
		if(!source)
		{
			// Sort the objects
			if(!(camera->GetFlags() & Camera::FlagNoSorting))
			{
				auto begin = _frame.begin();
				std::advance(begin, skyCubeMeshes);
			
				ParallelSort(begin, _frame.end(), [](const RenderingObject& a, const RenderingObject& b) {
					
					bool drawALate = (a.flags & RenderingObject::DrawLate);
					bool drawBLate = (b.flags & RenderingObject::DrawLate);
					
					if(drawALate && drawALate != drawBLate)
						return false;
					
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
		size_t skyCubeMeshes = 0;
		
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
		
		// Render loop
		DrawCamera(camera, 0, skyCubeMeshes);
		Camera *lastPipeline = camera;
		
		if(camera->GetFlags() & Camera::FlagForceFlush)
		{
			if(_flushedCameras.find(camera) == _flushedCameras.end())
			{
				_flushCameras.push_back(std::pair<Camera *, Shader *>(camera, camera->GetBlitShader()));
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
						_flushCameras.push_back(std::pair<Camera *, Shader *>(previous, camera->GetBlitShader()));
						_flushedCameras.insert(previous);
					}
				}
			}
			
			lastPipeline = previous;
		}
		
		if(!(previous->GetFlags() & Camera::FlagHidden) && _flushedCameras.find(camera) == _flushedCameras.end())
		{
			_flushCameras.push_back(std::pair<Camera *, Shader *>(previous, camera->GetBlitShader()));
			_flushedCameras.insert(previous);
		}
		
		// Cleanup of the frame
		_frameCamera = 0;
		
		_frame.clear();
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
}
