//
//  RNTexture.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTexture.h"
#include "RNResourceCoordinator.h"
#include "RNThread.h"
#include "RNKernel.h"
#include "RNSettings.h"
#include "RNWrappingObject.h"

namespace RN
{
	RNDeclareMeta(Texture)
	RNDeclareMeta(Texture2D)
	RNDeclareMeta(Texture2DArray)
	RNDeclareMeta(TextureCubeMap)
	
	// ---------------------
	// MARK: -
	// MARK: Texture
	// ---------------------
	
	float Texture::_defaultAnisotropy = 1.0f;
	
	Texture::Texture(GLenum type, bool linear) :
		_isLinear((!Settings::GetSharedInstance()->GetBoolForKey(kRNSettingsGammaCorrectionKey)) ? true : linear),
		_width(0),
		_height(0),
		_glType(type),
		_isComplete(false),
		_hasChanged(false)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
			gl::GenTextures(1, &_name);
		});
	}
	
	Texture::~Texture()
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			gl::DeleteTextures(1, &_name);
		});
	}
	
	
	Texture *Texture::WithFile(const std::string& name, bool isLinear)
	{
		Dictionary *settings = new Dictionary();
		settings->SetObjectForKey(Number::WithBool(isLinear), RNCSTR("linear"));
		settings->Autorelease();
		
		Texture *texture = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Texture2D>(RNSTR(name.c_str()), settings);
		return texture;
	}
	
	Texture *Texture::WithFile(const std::string& name, const Parameter& parameter, bool isLinear)
	{
		WrappingObject<Parameter> *wrapper = new WrappingObject<Parameter>(parameter);
		
		Dictionary *settings = new Dictionary();
		settings->SetObjectForKey(Number::WithBool(isLinear), RNCSTR("linear"));
		settings->SetObjectForKey(wrapper->Autorelease(), RNCSTR("parameter"));
		settings->Autorelease();
		
		Texture *texture = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Texture2D>(RNSTR(name.c_str()), settings);
		return texture;
	}
	
	
	void Texture::SetSize(size_t width, size_t height)
	{}
	
	void Texture::Update()
	{
		if(_hasChanged && _isComplete)
		{
			_hasChanged = false;
			
			if(_parameter.generateMipMaps && _parameter.maxMipMaps > 0)
				gl::GenerateMipmap(_glType);
		}
	}
	
	void Texture::Bind()
	{
		gl::BindTexture(_glType, _name);
	}

	void Texture::SetParameter(const Parameter& parameter)
	{
		_parameter = parameter;
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
			
			GLenum wrapMode;
			GLenum minFilter;
			GLenum magFilter;
			
			switch(_parameter.wrapMode)
			{
				case WrapMode::Clamp:
					wrapMode = GL_CLAMP_TO_EDGE;
					break;
					
				case WrapMode::Repeat:
					wrapMode = GL_REPEAT;
					break;
			}
			
			switch(_parameter.filter)
			{
				case Filter::Linear:
					magFilter = GL_LINEAR;
					minFilter = (_parameter.maxMipMaps > 0) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
					break;
					
				case Filter::Nearest:
					minFilter = GL_NEAREST;
					magFilter = GL_NEAREST;
					break;
			}
			
			Bind();
		
			gl::TexParameteri(_glType, GL_TEXTURE_WRAP_S, wrapMode);
			gl::TexParameteri(_glType, GL_TEXTURE_WRAP_T, wrapMode);
			
			gl::TexParameteri(_glType, GL_TEXTURE_MIN_FILTER, minFilter);
			gl::TexParameteri(_glType, GL_TEXTURE_MAG_FILTER, magFilter);
			
			gl::TexParameteri(_glType, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(_parameter.maxMipMaps));
			
			if(gl::SupportsFeature(gl::Feature::AnisotropicFilter))
			{
#if GL_EXT_texture_filter_anisotropic
				float anisotropy = (_parameter.anisotropy < 1.0f) ? _defaultAnisotropy : _parameter.anisotropy;
				gl::TexParameterf(_glType, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
#endif
			}
			
			if(_parameter.depthCompare)
			{
				gl::TexParameteri(_glType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
				gl::TexParameteri(_glType, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
			}
			else
			{
				gl::TexParameteri(_glType, GL_TEXTURE_COMPARE_MODE, GL_NONE);
			}
		
			Update();
		});
	}
	
	
	
	RN_INLINE uint8 *ReadPixel(uint8 *data, Texture::Format format, uint32& r, uint32& g, uint32& b, uint32& a)
	{
		switch(format)
		{
			case Texture::Format::RGBA8888:
			{
				uint32 *pixel = (uint32 *)data;
				
				a = ((*pixel >> 24) & 0xFF);
				b = ((*pixel >> 16) & 0xFF);
				g = ((*pixel >> 8) & 0xFF);
				r = ((*pixel >> 0) & 0xFF);
				
				return (uint8 *)(pixel + 1);
				break;
			}
				
			case Texture::Format::RGBA4444:
			{
				uint16 *pixel = (uint16 *)data;
				
				r = ((*pixel >> 12) & 0xF) << 4;
				g = ((*pixel >> 8)  & 0xF) << 4;
				b = ((*pixel >> 4)  & 0xF) << 4;
				a = ((*pixel >> 0)  & 0xF) << 4;
				
				return (uint8 *)(pixel + 1);
				break;
			}
				
			case Texture::Format::RGBA5551:
			{
				uint16 *pixel = (uint16 *)data;
				
				r = ((*pixel >> 11) & 0xFF) << 3;
				g = ((*pixel >> 6)  & 0xFF) << 3;
				b = ((*pixel >> 1)  & 0xFF) << 3;
				a = ((*pixel >> 0)  & 0xFF) << 7;
				
				return (uint8 *)(pixel + 1);
				break;
			}
				
			case Texture::Format::RGB565:
			{
				uint16 *pixel = (uint16 *)data;
				
				r = ((*pixel >> 11) & 0xFF) << 3;
				g = ((*pixel >> 5)  & 0xFF) << 2;
				b = ((*pixel >> 0)  & 0xFF) << 3;
				a = 255;
				
				return (uint8 *)(pixel + 1);
				break;
			}
				
			case Texture::Format::R8:
			{
				r = *data;
				g = b = 0;
				a = 255;
				
				return (data + 1);
				break;
			}
				
			case Texture::Format::RG88:
			{
				uint16 *pixel = (uint16 *)data;
				
				r = ((*pixel >> 8)  & 0xFF);
				g = ((*pixel >> 0)  & 0xFF);
				b = 0;
				a = 255;
				
				return (uint8 *)(pixel + 1);
				break;
			}
				
			case Texture::Format::RGB888:
			{
				uint16 *pixelRG = (uint16 *)data;
				uint8 *pixelB = (uint8 *)(pixelRG + 1);
				
				r = ((*pixelRG >> 8)  & 0xFF);
				g = ((*pixelRG >> 0)  & 0xFF);
				b = *pixelB;
				a = 255;
				
				return (uint8 *)(pixelB + 1);
				break;
			}
				
			default:
				throw Exception(Exception::Type::GenericException, "");
				break;
		}
		
		return data;
	}
	
	void *Texture::ConvertData(const PixelData& data, Format target)
	{
		if(data.format == target)
			return const_cast<void *>(data.data);
		
		size_t pixel = data.width * data.height;
		
		uint8 *temp   = reinterpret_cast<uint8 *>(const_cast<void *>(data.data));
		void  *result = nullptr;
		
		// Convert data to the specified target format
		uint32 r, g, b, a;
		
		switch(target)
		{
			case Format::RGBA8888:
			{
				result = malloc(pixel * sizeof(uint32));
				
				uint32 *outPixel = (uint32 *)result;
				for(size_t i = 0; i < pixel; i ++)
				{
					temp = ReadPixel(temp, data.format, r, g, b, a);
					*outPixel ++ = (a << 24) | (b << 16) | (g << 8) | r;
				}
				
				break;
			}
				
			case Format::RGBA4444:
			{
				result = malloc(pixel * sizeof(uint16));
				
				uint16 *outPixel = (uint16 *)result;
				for(size_t i = 0; i < pixel; i ++)
				{
					temp = ReadPixel(temp, data.format, r, g, b, a);
					
					r = (r >> 4);
					g = (g >> 4);
					b = (b >> 4);
					a = (a >> 4);
					
					*outPixel ++ = (r << 12) | (g << 8) | (b << 4) | a;
				}
				
				break;
			}
				
			case Format::RGBA5551:
			{
				result = malloc(pixel * sizeof(uint16));
				
				uint16 *outPixel = (uint16 *)result;
				for(size_t i = 0; i < pixel; i ++)
				{
					temp = ReadPixel(temp, data.format, r, g, b, a);
					
					r = (r >> 3);
					g = (g >> 3);
					b = (b >> 3);
					a = (a >> 7);
					
					*outPixel ++ = (r << 11) | (g << 6) | (b << 1) | a;
				}
				
				break;
			}
				
			case Format::RGB565:
			{
				result = malloc(pixel * sizeof(uint16));
				
				uint16 *outPixel = (uint16 *)result;
				for(size_t i = 0; i < pixel; i ++)
				{
					temp = ReadPixel(temp, data.format, r, g, b, a);
					
					r = (r >> 3);
					g = (g >> 2);
					b = (b >> 3);
					
					*outPixel ++ = (r << 11) | (g << 5) | b;
				}
				
				break;
			}
				
			case Format::R8:
			{
				result = malloc(pixel * sizeof(uint8));
				
				uint8 *outPixel = (uint8 *)result;
				for(size_t i = 0; i < pixel; i ++)
				{
					temp = ReadPixel(temp, data.format, r, g, b, a);
					*outPixel ++ = r;
				}
				
				break;
			}
				
			case Format::RG88:
			{
				result = malloc(pixel * sizeof(uint16));
				
				uint16 *outPixel = (uint16 *)result;
				for(size_t i = 0; i < pixel; i ++)
				{
					temp = ReadPixel(temp, data.format, r, g, b, a);
					*outPixel ++ = (r << 8) | g;
				}
				
				break;
			}
				
			case Format::RGB888:
			{
				result = malloc(pixel * (sizeof(uint8) * 3));
				
				uint8 *outPixel = (uint8 *)result;
				for(size_t i = 0; i < pixel; i ++)
				{
					temp = ReadPixel(temp, data.format, r, g, b, a);
					
					*outPixel ++ = r;
					*outPixel ++ = g;
					*outPixel ++ = b;
				}
				
				break;
			}
				
			default:
				throw Exception(Exception::Type::TextureFormatUnsupportedException, "");
				break;
		}
		
		return result;
	}
	
	void Texture::ConvertFormatToOpenGL(Format format, bool linear, GLint& glInternalFormat, GLenum& glFormat, GLenum& glType)
	{
		switch(format)
		{
				// Integer formats
			case Format::RGBA8888:
				glFormat = GL_RGBA;
				glInternalFormat = linear ? GL_RGBA : GL_SRGB8_ALPHA8;
				glType  = GL_UNSIGNED_BYTE;
				break;
				
			case Format::RGBA4444:
				glFormat = GL_RGBA;
				glInternalFormat = linear ? GL_RGBA : GL_SRGB8_ALPHA8;
				glType  = GL_UNSIGNED_SHORT_4_4_4_4;
				break;
				
			case Format::RGBA5551:
				glFormat = GL_RGBA;
				glInternalFormat = linear ? GL_RGBA : GL_SRGB8_ALPHA8;
				glType  = GL_UNSIGNED_SHORT_5_5_5_1;
				break;
				
			case Format::RGB565:
				glFormat = GL_RGB;
				glInternalFormat = linear ? GL_RGB : GL_SRGB8;
				glType  = GL_UNSIGNED_SHORT_5_6_5;
				break;
				
#if RN_TARGET_OPENGL
			case Format::R8:
				glFormat = GL_RED;
				glInternalFormat = GL_RED;
				glType= GL_UNSIGNED_BYTE;
				break;
				
			case Format::RG88:
				glFormat = GL_RG;
				glInternalFormat = GL_RGB;
				glType= GL_UNSIGNED_BYTE;
				break;
#endif
				
			case Format::RGB888:
				glFormat = GL_RGB;
				glInternalFormat = linear ? GL_RGB : GL_SRGB8;
				glType= GL_UNSIGNED_BYTE;
				break;
				
				// Floating point formats
			case Format::RGBA32F:
				glFormat = GL_RGBA;
				glInternalFormat = GL_RGBA32F;
				glType= GL_FLOAT;
				break;
				
			case Format::R32F:
				glFormat = GL_RED;
				glInternalFormat = GL_R32F;
				glType= GL_FLOAT;
				break;
				
			case Format::RG32F:
				glFormat = GL_RG;
				glInternalFormat = GL_RG32F;
				glType= GL_FLOAT;
				break;
				
			case Format::RGB32F:
				glFormat = GL_RGB;
				glInternalFormat = GL_RGB32F;
				glType= GL_FLOAT;
				break;
				
				// Half floats
			case Format::RGBA16F:
				glFormat = GL_RGBA;
				glInternalFormat = GL_RGBA16F;
				glType= GL_HALF_FLOAT;
				break;
				
			case Format::R16F:
				glFormat = GL_RED;
				glInternalFormat = GL_R16F;
				glType= GL_HALF_FLOAT;
				break;
				
			case Format::RG16F:
				glFormat = GL_RG;
				glInternalFormat = GL_RG16F;
				glType= GL_HALF_FLOAT;
				break;
				
			case Format::RGB16F:
				glFormat = GL_RGB;
				glInternalFormat = GL_RGB16F;
				glType= GL_HALF_FLOAT;
				break;
				
			case Format::Depth24I:
				glFormat = GL_DEPTH_COMPONENT;
				glInternalFormat = GL_DEPTH_COMPONENT24;
				glType= GL_UNSIGNED_BYTE;
				break;
				
			case Format::Depth32F:
				glFormat = GL_DEPTH_COMPONENT;
				glInternalFormat = GL_DEPTH_COMPONENT32F;
				glType= GL_FLOAT;
				
			case Format::DepthStencil:
				glFormat = GL_DEPTH_STENCIL;
				glInternalFormat = GL_DEPTH24_STENCIL8;
				glType= GL_UNSIGNED_INT_24_8;
				break;
				
			default:
				throw Exception(Exception::Type::TextureFormatUnsupportedException, "");
				break;
		}
	}
	
	bool Texture::PlatformSupportsFormat(Format format)
	{
		try
		{
			GLenum glFormat;
			GLint glInternalFormat;
			GLenum glType;
			
			ConvertFormatToOpenGL(format, true, glInternalFormat, glFormat, glType);
			return true;
		}
		catch(Exception e)
		{
			return false;
		}
	}
	
	float Texture::GetMaxAnisotropyLevel()
	{
		static float max;
		static std::once_flag flag;
		
		std::call_once(flag, [&]() {
			if(gl::SupportsFeature(gl::Feature::AnisotropicFilter))
			{
#if GL_EXT_texture_filter_anisotropic
				OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
					gl::GetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
				}, true);
#endif
			}
			else
			{
				max = 1.0f;
			}
		});
		
		return max;
	}
	
	float Texture::GetDefaultAnisotropyLevel()
	{
		return _defaultAnisotropy;
	}
	
	void Texture::SetDefaultAnisotropyLevel(float level)
	{
		if(level < 1.0f || level > GetMaxAnisotropyLevel())
			throw Exception(Exception::Type::InvalidArgumentException, "Anisotropy level must be between [1.0, max anisotropy]!");
		
		_defaultAnisotropy = level;
	}
	
	
	
	// ---------------------
	// MARK: -
	// MARK: Texture2D
	// ---------------------
	
	
	Texture2D::Texture2D(Format format, bool isLinear) :
		Texture(GL_TEXTURE_2D, isLinear)
	{
		_parameter.format = format;
		
		SetParameter(_parameter);
	}
	
	Texture2D::Texture2D(const Parameter& parameter, bool isLinear) :
		Texture(GL_TEXTURE_2D, isLinear)
	{
		SetParameter(parameter);
	}
	
	void Texture2D::SetSize(size_t width, size_t height)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this, width, height] {
			GLint internalFormat;
			GLenum type, format;
			
			_width  = width;
			_height = height;
			
			ConvertFormatToOpenGL(_parameter.format, _isLinear, internalFormat, format, type);
			Bind();
			
			gl::TexImage2D(GL_TEXTURE_2D, 0, internalFormat, static_cast<GLint>(_width), static_cast<GLint>(_height), 0, format, type, nullptr);
		
			_isComplete = true;
			_hasChanged = true;
			
			Update();
		});
	}
	
	void Texture2D::SetData(const PixelData& data)
	{
		void *converted = ConvertData(data, _parameter.format);
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			
			GLint internalFormat;
			GLenum type, format;
			
			_width  = data.width;
			_height = data.height;
			
			ConvertFormatToOpenGL(_parameter.format, _isLinear, internalFormat, format, type);
			Bind();
		
			gl::PixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(data.alignment));
			gl::TexImage2D(GL_TEXTURE_2D, 0, internalFormat, static_cast<GLint>(_width), static_cast<GLint>(_height), 0, format, type, converted);
		
			_isComplete = true;
			_hasChanged = true;
			
			Update();
		}, true);
		
		if(converted != data.data)
			free(converted);
	}
	
	void Texture2D::UpdateData(const PixelData& data)
	{
		UpdateRegion(data, Rect(0, 0, _width, _height));
	}
	
	void Texture2D::UpdateRegion(const PixelData& data, const Rect& region)
	{
		void *converted = ConvertData(data, _parameter.format);
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			GLint internalFormat;
			GLenum type, format;
				
			ConvertFormatToOpenGL(_parameter.format, _isLinear, internalFormat, format, type);
			Bind();
		
			gl::PixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(data.alignment));
			gl::TexSubImage2D(GL_TEXTURE_2D, 0, region.x, region.y, region.width, region.height, format, type, converted);
		
			_hasChanged = true;
			
			Update();
		}, true);
		
		if(converted != data.data)
			free(converted);
	}
	
	void Texture2D::GetData(PixelData& data)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			GLint internalFormat;
			GLenum type, format;
			
			ConvertFormatToOpenGL(data.format, _isLinear, internalFormat, format, type);
			Bind();
			
			gl::PixelStorei(GL_PACK_ALIGNMENT, static_cast<GLint>(data.alignment));
			gl::GetTexImage(GL_TEXTURE_2D, 0, format, type, data.data);
		}, true);
	}

	
	// ---------------------
	// MARK: -
	// MARK: Texture2DArray
	// ---------------------
	
	Texture2DArray::Texture2DArray(Format format, bool isLinear) :
		Texture(GL_TEXTURE_2D_ARRAY, isLinear),
		_layer(1)
	{
		_parameter.format = format;
		
		SetParameter(_parameter);
	}
	
	Texture2DArray::Texture2DArray(const Parameter& parameter, bool isLinear) :
		Texture(GL_TEXTURE_2D_ARRAY, isLinear),
		_layer(1)
	{
		SetParameter(parameter);
	}
	
	void Texture2DArray::SetSize(size_t width, size_t height)
	{
		SetSize(width, height, _layer);
	}
	
	void Texture2DArray::SetSize(size_t width, size_t height, size_t layer)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this, width, height, layer] {
			GLint internalFormat;
			GLenum type, format;
			
			_width  = width;
			_height = height;
			_layer  = layer;
			
			ConvertFormatToOpenGL(_parameter.format, _isLinear, internalFormat, format, type);
			Bind();
			
			gl::TexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, static_cast<GLint>(_width), static_cast<GLint>(_height), static_cast<GLint>(_layer), 0, format, type, nullptr);
		
			_isComplete = true;
			_hasChanged = true;
			
			Update();
		});
	}
	
	void Texture2DArray::SetData(const PixelData& data, size_t index)
	{
		void *converted = ConvertData(data, _parameter.format);
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			GLint internalFormat;
			GLenum type, format;
			
			_width  = data.width;
			_height = data.height;
			
			ConvertFormatToOpenGL(_parameter.format, _isLinear, internalFormat, format, type);
			Bind();
			
			gl::PixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(data.alignment));
			gl::TexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, static_cast<GLint>(_width), static_cast<GLint>(_height), static_cast<GLint>(index), 0, format, type, converted);
		
			_isComplete = true;
			_hasChanged = true;
			
			Update();
		}, true);
		
		if(converted != data.data)
			free(converted);
	}
	
	void Texture2DArray::UpdateData(const PixelData& data, size_t index)
	{
		void *converted = ConvertData(data, _parameter.format);
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			GLint internalFormat;
			GLenum type, format;
			
			ConvertFormatToOpenGL(_parameter.format, _isLinear, internalFormat, format, type);
			Bind();
			
			gl::PixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(data.alignment));
			gl::TexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, static_cast<GLint>(_width), static_cast<GLint>(_height), static_cast<GLint>(index), format, type, converted);
			
			_hasChanged = true;
			
			Update();
		}, true);
		
		if(converted != data.data)
			free(converted);
	}
	
	// ---------------------
	// MARK: -
	// MARK: TextureCubeMap
	// ---------------------
	
	TextureCubeMap::TextureCubeMap(Format format, bool isLinear) :
		Texture(GL_TEXTURE_CUBE_MAP, isLinear)
	{
		_parameter.format = format;
		
		SetParameter(_parameter);
	}
	
	TextureCubeMap::TextureCubeMap(const Parameter& parameter, bool isLinear) :
		Texture(GL_TEXTURE_CUBE_MAP, isLinear)
	{
		SetParameter(parameter);
	}
	
	void TextureCubeMap::SetSize(size_t width, size_t height)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this, width, height] {
			
			GLint internalFormat;
			GLenum type, format;
			
			_width  = width;
			_height = height;
		
			ConvertFormatToOpenGL(_parameter.format, _isLinear, internalFormat, format, type);
			Bind();
			
			for(int i = 0; i < 6; i ++)
				gl::TexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, static_cast<GLint>(_width), static_cast<GLint>(_height), 0, format, type, nullptr);
			
			_isComplete = true;
			_hasChanged = true;
			
			Update();
		});
	}
	
	void TextureCubeMap::SetData(const PixelData& data, Side side)
	{
		void *converted = ConvertData(data, _parameter.format);
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			GLint internalFormat;
			GLenum type, format;
			
			_width  = data.width;
			_height = data.height;
			
			ConvertFormatToOpenGL(_parameter.format, _isLinear, internalFormat, format, type);
			
			Bind();
			gl::PixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(data.alignment));
			
			switch(side)
			{
				case Side::All:
					for(int i = 0; i < 6; i ++)
						gl::TexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, static_cast<GLint>(_width), static_cast<GLint>(_height), 0, format, type, converted);
					break;
					
				default:
					gl::TexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<int>(side), 0, internalFormat, static_cast<GLint>(_width), static_cast<GLint>(_height), 0, format, type, converted);
					break;
			}
			
			_isComplete = true;
			_hasChanged = true;
			
			Update();
		}, true);
		
		if(converted != data.data)
			free(converted);
	}
	
	void TextureCubeMap::UpdateData(const PixelData& data, Side side)
	{
		void *converted = ConvertData(data, _parameter.format);
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			
			GLint internalFormat;
			GLenum type, format;
		
			ConvertFormatToOpenGL(_parameter.format, _isLinear, internalFormat, format, type);
			Bind();
			
			gl::PixelStorei(GL_UNPACK_ALIGNMENT, static_cast<GLint>(data.alignment));
			
			switch(side)
			{
				case Side::All:
					for(int i = 0; i < 6; i ++)
							gl::TexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, static_cast<GLint>(_width), static_cast<GLint>(_height), format, type, converted);
					break;
					
				default:
					gl::TexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<int>(side), 0, 0, 0, static_cast<GLint>(_width), static_cast<GLint>(_height), format, type, converted);
					break;
			}
			
			_isComplete = true;
			_hasChanged = true;
			
			Update();
		}, true);
		
		if(converted != data.data)
			free(converted);
	}
}
