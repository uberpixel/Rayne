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
#include "RNLightManager.h"
#include "RNMessage.h"
#include "RNWindow.h"

#define kRNRendererMaxVAOAge 300

#define TexturePixelWidth(t) (t->GetWidth() * t->GetScaleFactor())
#define TexturePixelHeight(t) (t->GetHeight() * t->GetScaleFactor())

namespace RN
{
	RNDefineSingleton(Renderer)
	
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
		_time = Kernel::GetSharedInstance()->GetTime();
		_mode = Mode::ModeWorld;
		
		_hdrExposure   = 1.0f;
		_hdrWhitePoint = 1.0f;
		
		_captureAge = static_cast<FrameID>(-1);
		_captureIndex = 0;
		
		_canValidatePrograms = gl::SupportsFeature(gl::Feature::ProgramValidation);
		_validatePrograms    = false;
		
		// Default OpenGL state
		_cullingEnabled   = false;
		_depthTestEnabled = false;
		_blendingEnabled  = false;
		_depthWrite       = false;
		_polygonOffsetEnabled = false;
		_scissorTest      = false;
		
		_cullMode    = GL_CCW;
		_depthFunc   = GL_LESS;
		_polygonMode = GL_FILL;
		
		_blendEquation      = GL_FUNC_ADD;
		_alphaBlendEquation = GL_FUNC_ADD;
		
		_blendSource           = GL_ONE;
		_alphaBlendSource      = GL_ONE;
		_blendDestination      = GL_ZERO;
		_alphaBlendDestination = GL_ZERO;
		
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
			gl::BlendEquation(_blendEquation);
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
		const Matrix& projectionMatrix = _currentCamera->_projectionMatrix;
		const Matrix& inverseProjectionMatrix = _currentCamera->_inverseProjectionMatrix;
		
		const Matrix& viewMatrix = _currentCamera->_viewMatrix;
		const Matrix& inverseViewMatrix = _currentCamera->_inverseViewMatrix;
		
		if(_currentProgram->frameSize != -1)
		{
			const Rect& frame = _currentCamera->GetFrame();
			gl::Uniform4f(_currentProgram->frameSize, 1.0f/frame.width/_scaleFactor, 1.0f/frame.height/_scaleFactor, frame.width * _scaleFactor, frame.height * _scaleFactor);
		}
		
		if(_currentProgram->hdrSettings != -1)
			gl::Uniform4f(_currentProgram->hdrSettings, _hdrExposure, _hdrWhitePoint, 0.0f, 0.0f);
		
		if(_currentProgram->clipPlanes != -1)
			gl::Uniform2f(_currentProgram->clipPlanes, _currentCamera->_clipNear, _currentCamera->_clipFar);
		
		
		if(_currentProgram->fogPlanes != -1)
			gl::Uniform2f(_currentProgram->fogPlanes, _currentCamera->_fogNear, 1.0f / (_currentCamera->_fogFar - _currentCamera->_fogNear));
		
		if(_currentProgram->fogColor != -1)
			gl::Uniform4fv(_currentProgram->fogColor, 1, &_currentCamera->_fogColor.r);
		
		if(_currentProgram->cameraAmbient != -1)
			gl::Uniform4fv(_currentProgram->cameraAmbient, 1, &_currentCamera->_ambient.r);
		
		if(_currentProgram->clipPlane != -1)
			gl::Uniform4f(_currentProgram->clipPlane, _currentCamera->_clipPlane.GetNormal().x, _currentCamera->_clipPlane.GetNormal().y, _currentCamera->_clipPlane.GetNormal().z, _currentCamera->_clipPlane.GetD());
		
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
			const Vector3& forward = _currentCamera->GetForward();
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
	
	void Renderer::ValidateState()
	{
#if RN_BUILD_DEBUG
		if(_canValidatePrograms && _validatePrograms)
		{
			GLint status, length;
			
			gl::ValidateProgram(_currentProgram->program);
			gl::GetProgramiv(_currentProgram->program, GL_VALIDATE_STATUS, &status);
			
			if(!status)
			{
				gl::GetProgramiv(_currentProgram->program, GL_INFO_LOG_LENGTH, &length);
				
				char *log = new char[length];
				gl::GetProgramInfoLog(_currentProgram->program, length, &length, static_cast<GLchar *>(log));
				
				std::string error(log);
				delete [] log;
				
				throw Exception(Exception::Type::InconsistencyException, "Shader program validation failed: " + error);
			}
		}
#endif
	}
	
	// ---------------------
	// MARK: -
	// MARK: Binding
	// ---------------------

	void Renderer::FlushAutoVAOs()
	{
		_autoVAOs.clear();
	}
	
	void Renderer::BindVAO(const std::pair<ShaderProgram *, Mesh *> &pair)
	{
		auto iterator = _autoVAOs.find(pair);
		GLuint vao;
		
		if(iterator == _autoVAOs.end())
		{
			ShaderProgram *shader = pair.first;
			Mesh *mesh = pair.second;
			
			GLuint vbo = mesh->GetVBO();
			GLuint ibo = mesh->GetIBO();
			
			OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
				gl::GenVertexArrays(1, &vao);
				gl::BindVertexArray(vao);
				
				gl::BindBuffer(GL_ARRAY_BUFFER, vbo);
				
				if(mesh->SupportsFeature(MeshFeature::Indices))
					gl::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
				
				// Vertices
				if(shader->attPosition != -1 && mesh->SupportsFeature(MeshFeature::Vertices))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(MeshFeature::Vertices);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attPosition);
					gl::VertexAttribPointer(shader->attPosition, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Normals
				if(shader->attNormal != -1 && mesh->SupportsFeature(MeshFeature::Normals))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(MeshFeature::Normals);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attNormal);
					gl::VertexAttribPointer(shader->attNormal, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Tangents
				if(shader->attTangent != -1 && mesh->SupportsFeature(MeshFeature::Tangents))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(MeshFeature::Tangents);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attTangent);
					gl::VertexAttribPointer(shader->attTangent, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Texcoord0
				if(shader->attTexcoord0 != -1 && mesh->SupportsFeature(MeshFeature::UVSet0))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(MeshFeature::UVSet0);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attTexcoord0);
					gl::VertexAttribPointer(shader->attTexcoord0, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Texcoord1
				if(shader->attTexcoord1 != -1 && mesh->SupportsFeature(MeshFeature::UVSet1))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(MeshFeature::UVSet1);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attTexcoord1);
					gl::VertexAttribPointer(shader->attTexcoord1, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Color0
				if(shader->attColor0 != -1 && mesh->SupportsFeature(MeshFeature::Color0))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(MeshFeature::Color0);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attColor0);
					gl::VertexAttribPointer(shader->attColor0, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Color1
				if(shader->attColor1 != -1 && mesh->SupportsFeature(MeshFeature::Color1))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(MeshFeature::Color1);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attColor1);
					gl::VertexAttribPointer(shader->attColor1, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Bone Weights
				if(shader->attBoneWeights != -1 && mesh->SupportsFeature(MeshFeature::BoneWeights))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(MeshFeature::BoneWeights);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attBoneWeights);
					gl::VertexAttribPointer(shader->attBoneWeights, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
				
				// Bone Indices
				if(shader->attBoneIndices != -1 && mesh->SupportsFeature(MeshFeature::BoneIndices))
				{
					const MeshDescriptor *descriptor = mesh->GetDescriptorForFeature(MeshFeature::BoneIndices);
					size_t offset = descriptor->offset;
					
					gl::EnableVertexAttribArray(shader->attBoneIndices);
					gl::VertexAttribPointer(shader->attBoneIndices, (GLsizei)descriptor->elementMember, GL_FLOAT, GL_FALSE, (GLsizei)mesh->GetStride(), (const void *)offset);
				}
			}, true);
			
			_autoVAOs[pair] = std::tuple<GLuint, uint32>(vao, 0);
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
		
#define IsOverriden(attribute) \
	(material->override & Material::Override::attribute || surfaceMaterial->override & Material::Override::attribute)
		
#define PickAttribute(_override, attribute) \
	(IsOverriden(_override) ? material->attribute : surfaceMaterial->attribute)
		
		if(changedShader || material != _currentMaterial)
		{
			_textureUnit = 0;
			
			const Array *textures = IsOverriden(Textures) ? material->GetTextures() : surfaceMaterial->GetTextures();
			const std::vector<GLuint>& textureLocations = program->texlocations;
			
			if(textureLocations.size() > 0)
			{
				size_t textureCount = std::min(textureLocations.size(), textures->GetCount());
				
				for(size_t i = 0; i<textureCount; i ++)
				{
					GLint location = textureLocations[i];
					
					if(location == -1)
						continue;
					
					Texture *texture = static_cast<Texture *>((*textures)[i]);
					
					gl::Uniform1i(location, BindTexture(texture));
					
					location = program->texinfolocations[i];
					if(location != -1)
						gl::Uniform4f(location, 1.0f/TexturePixelWidth(texture), 1.0f/TexturePixelHeight(texture), TexturePixelWidth(texture), TexturePixelHeight(texture));
				}
			}
		}
		
		if(surfaceMaterial != material)
		{
			Material::CullMode cullMode = IsOverriden(Culling) ? material->cullMode : surfaceMaterial->cullMode;
			if(cullMode == Material::CullMode::None)
			{
				SetCullingEnabled(false);
			}
			else
			{
				SetCullingEnabled(true);
				SetCullMode(static_cast<GLenum>(cullMode));
			}
			
			SetPolygonMode(static_cast<GLenum>(material->polygonMode));
			
			SetDepthTestEnabled(PickAttribute(Depthtest, depthTest));
			SetDepthFunction(static_cast<GLenum>(PickAttribute(DepthtestMode, depthTestMode)));
			
			SetPolygonOffsetEnabled(PickAttribute(PolygonOffset, polygonOffset));
			
			if(IsOverriden(Depthwrite))
			{
				SetDepthWriteEnabled((material->depthWrite && !(_currentCamera->_flags & Camera::Flags::NoDepthWrite)));
			}
			else
			{
				SetDepthWriteEnabled((surfaceMaterial->depthWrite && !(_currentCamera->_flags & Camera::Flags::NoDepthWrite)));
			}
			
			SetBlendingEnabled(PickAttribute(Blending, blending));
			SetPolygonOffset(PickAttribute(PolygonOffset, polygonOffsetFactor), PickAttribute(PolygonOffset, polygonOffsetUnits));
			
			if(_blendingEnabled)
			{
				if(IsOverriden(Blendmode))
				{
					SetBlendFunction(static_cast<GLenum>(material->blendSource), static_cast<GLenum>(material->blendDestination),
									 static_cast<GLenum>(material->alphaBlendSource), static_cast<GLenum>(material->alphaBlendDestination));
				}
				else
				{
					SetBlendFunction(static_cast<GLenum>(surfaceMaterial->blendSource), static_cast<GLenum>(surfaceMaterial->blendDestination),
									 static_cast<GLenum>(surfaceMaterial->alphaBlendSource), static_cast<GLenum>(surfaceMaterial->alphaBlendDestination));
				}
				
				if(IsOverriden(Blendequation))
				{
					SetBlendEquation(static_cast<GLenum>(material->blendEquation), static_cast<GLenum>(material->alphaBlendEquation));
				}
				else
				{
					SetBlendEquation(static_cast<GLenum>(material->blendEquation), static_cast<GLenum>(surfaceMaterial->alphaBlendEquation));
				}
			}
		}
		else
		{
			Material::CullMode cullMode = material->cullMode;
			
			if(cullMode == Material::CullMode::None)
			{
				SetCullingEnabled(false);
			}
			else
			{
				SetCullingEnabled(true);
				SetCullMode(static_cast<GLenum>(cullMode));
			}
			
			SetPolygonMode(static_cast<GLenum>(material->polygonMode));
			
			SetDepthTestEnabled(material->depthTest);
			SetDepthFunction(static_cast<GLenum>(material->depthTestMode));
			
			SetPolygonOffsetEnabled(material->polygonOffset);
			SetDepthWriteEnabled((material->depthWrite && !(_currentCamera->_flags & Camera::Flags::NoDepthWrite)));
			SetBlendingEnabled(material->blending);
			
			if(_polygonOffsetEnabled)
				SetPolygonOffset(material->polygonOffsetFactor, material->polygonOffsetUnits);
			
			if(_blendingEnabled)
			{
				SetBlendFunction(static_cast<GLenum>(material->blendSource), static_cast<GLenum>(material->blendDestination),
								 static_cast<GLenum>(material->alphaBlendSource), static_cast<GLenum>(material->alphaBlendDestination));
				SetBlendEquation(static_cast<GLenum>(material->blendEquation), static_cast<GLenum>(material->alphaBlendEquation));
			}
		}
		
#undef PickAttribute
#undef IsOverriden
		
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
			{
				Data *data = new Data(_data, _width * _height * 4, true, true);
				return data->Autorelease();
			}
				
			case Format::RGB888:
			{
				size_t size = _width * _height;
				uint8 *temp = new uint8[size * 3];
				
				uint32 *pixel = reinterpret_cast<uint32 *>(_data);
				uint32 *end = pixel + size;
				
				uint8 *data = temp;
				
				while(pixel != end)
				{
					*temp ++ = (*pixel >> 24) & 0xff;
					*temp ++ = (*pixel >> 16) & 0xff;
					*temp ++ = (*pixel >> 8) & 0xff;
					
					pixel ++;
				}
				
				
				return (new Data(data, size * 3, true, true))->Autorelease();
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
		_time = Kernel::GetSharedInstance()->GetTime();
		
		_renderedLights   = 0;
		_renderedVertices = 0;
		
		for(auto i = _autoVAOs.begin(); i != _autoVAOs.end();)
		{
			uint32& age = std::get<1>(i->second);
			
			if((++ age) > kRNRendererMaxVAOAge)
			{
				GLuint vao = std::get<0>(i->second);
				
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
		SetPolygonMode(GL_FILL);
		
		if(camera->_flags & Camera::Flags::BlendedBlitting)
		{
			SetBlendingEnabled(true);
			SetBlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			SetBlendingEnabled(false);
		}
		
		uint32 type = ShaderProgram::Type::Normal;

		if(_gammaCorrection && drawShader->SupportsProgramOfType(ShaderProgram::Type::GammaCorrection))
			type = ShaderProgram::Type::GammaCorrection;
		
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
				gl::Uniform4f(location, 1.0f/TexturePixelWidth(texture), 1.0f/TexturePixelHeight(texture), TexturePixelWidth(texture), TexturePixelHeight(texture));
		}
	}
	
	// Camera gets drawn into stage
	void Renderer::DrawCameraStage(Camera *camera, Camera *stage)
	{
		Material *material = stage->GetMaterial();
		Shader *shader = material->GetShader();
		
		ShaderLookup lookup = material->GetLookup();
		
		LightManager *lightManager = _frameCamera->GetLightManager();
		
		bool wantsLighting = (material->GetLighting() && lightManager && shader->SupportsProgramOfType(ShaderProgram::Type::Lighting));
		
		if(wantsLighting)
		{
			lookup.type |= ShaderProgram::Type::Lighting;
			lookup.lightDirectionalCount = lightManager->GetDirectionalLightCount();
			
			//TODO: fix
			if(lightManager->GetPointLightCount() > 0)
				lookup.lightPointSpotCount = 1;//lightPointSpotCount;
				
			lightManager->AdjustShaderLookup(shader, lookup);
		}
		
		ShaderProgram *program = shader->GetProgramWithLookup(lookup);
		
		_currentCamera = stage;
		_textureUnit = 0;
		
		SetDepthTestEnabled(false);
		SetBlendingEnabled(false);
		SetCullMode(GL_CCW);
		
		
		BindMaterial(material, program);
		
		SetPolygonMode(GL_FILL);
		UpdateShaderData();
		
		material->ApplyUniforms(program);
		
		if(wantsLighting)
			lightManager->UpdateProgram(this, program);
		
		uint32 targetmaps = std::min((uint32)program->targetmaplocations.size(), stage->GetRenderTargetCount());
		for(uint32 i=0; i<targetmaps; i++)
		{
			Texture *texture = camera->GetRenderTarget(i);
			GLuint location  = program->targetmaplocations[i];
			
			gl::Uniform1i(location, BindTexture(texture));
			
			location = program->targetmapinfolocations[i];
			if(location != -1)
				gl::Uniform4f(location, 1.0f/TexturePixelWidth(texture), 1.0f/TexturePixelHeight(texture), TexturePixelWidth(texture), TexturePixelHeight(texture));
		}
		
		if(program->depthmap != -1)
		{
			Texture *depthmap = camera->GetStorage()->GetDepthTarget();
			if(depthmap)
			{
				gl::Uniform1i(program->depthmap, BindTexture(depthmap));
				
				if(program->depthmapinfo != -1)
					gl::Uniform4f(program->depthmapinfo, 1.0f/TexturePixelWidth(depthmap), 1.0f/TexturePixelHeight(depthmap), TexturePixelWidth(depthmap), TexturePixelHeight(depthmap));
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
			if(!(camera->_flags & Camera::Flags::NoSorting))
			{
				auto begin = _frame.begin();
				std::advance(begin, skyCubeMeshes);
				
				RN::Vector3 position = camera->GetWorldPosition();
			
				for(auto i = begin; i != _frame.end(); i ++)
				{
					RenderingObject &object = *i;
					
					const Material *material = object.material;
					bool alpha = (material->blending);
					
					if(alpha)
					{
						object.distance = position.GetDistance(Vector3(object.transform->m[12], object.transform->m[13], object.transform->m[14]));
						object.flags |= 1 << 5;
					}
				}
				
				std::sort(begin, _frame.end(), [](const RenderingObject& a, const RenderingObject& b) {
					
					bool drawALate = (a.flags & RenderingObject::DrawLate);
					bool drawBLate = (b.flags & RenderingObject::DrawLate);
					
					if(drawALate != drawBLate)
						return drawBLate;
					
					bool alphaA = (a.flags & (1 << 5));
					bool alphaB = (b.flags & (1 << 5));
					
					if(alphaA != alphaB)
						return alphaB;
					
					if(alphaA)
						return (b.distance < a.distance);
					
					
					const Material *materialA = a.material;
					const Material *materialB = b.material;
					
					if(materialA->_shader != materialB->_shader)
						return materialA->_shader < materialB->_shader;
					
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
		Model *skyCube = camera->GetSky();
		size_t skyCubeMeshes = 0;
		
		Matrix cameraRotation;
		cameraRotation = Matrix::WithRotation(camera->GetWorldRotation());
		
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
		
		if(camera->_flags & Camera::Flags::ForceFlush)
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
				
				if(previous->_flags & Camera::Flags::ForceFlush)
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
		
		if(!(previous->GetFlags() & Camera::Flags::NoFlush) && _flushedCameras.find(camera) == _flushedCameras.end())
		{
			_flushCameras.push_back(std::pair<Camera *, Shader *>(previous, camera->GetBlitShader()));
			_flushedCameras.insert(previous);
		}
		
		LightManager *lightManager = _frameCamera->GetLightManager();
		if(lightManager)
		{
			lightManager->ClearLights();
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
