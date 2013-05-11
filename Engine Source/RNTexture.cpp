//
//  RNTexture.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTexture.h"
#include "RNTextureLoader.h"
#include "RNThread.h"
#include "RNKernel.h"
#include "RNSettings.h"

namespace RN
{
	RNDeclareMeta(Texture)
	uint32 Texture::_defaultAnisotropy = 0;
	
	Texture::Texture(TextureParameter::Format format, bool isLinear)
	{
		glGenTextures(1, &_name);
		RN_CHECKOPENGL();
		
		_width = _height = 0;
		_depth = 1;
		
		_isLinear = (!Settings::SharedInstance()->GammaCorrection()) ? true : isLinear;
		_isCompleteTexture = false;
		_hasChanged = false;
		
		
		TextureParameter parameter;
		parameter.format = format;
		
		SetType(parameter.type);
		
		Bind();
		SetParameter(parameter);
		Unbind();
	}
	
	Texture::Texture(const TextureParameter& parameter, bool isLinear)
	{
		glGenTextures(1, &_name);
		RN_CHECKOPENGL();
		
		_width = _height = 0;
		_depth = 1;
		
		_isLinear = (!Settings::SharedInstance()->GammaCorrection()) ? true : isLinear;
		_isCompleteTexture = false;
		_hasChanged = false;
		
		SetType(parameter.type);
		
		Bind();
		SetParameter(parameter);
		Unbind();
	}
	
	Texture::Texture(const std::string& name, bool isLinear)
	{
		glGenTextures(1, &_name);
		RN_CHECKOPENGL();
		
		_width = _height = 0;
		_depth = 1;
		
		_isLinear = (!Settings::SharedInstance()->GammaCorrection()) ? true : isLinear;
		_isCompleteTexture = false;
		_hasChanged = false;
		
		TextureParameter parameter;
		
		SetType(parameter.type);
		Bind();
		
		try
		{
			TextureLoader loader = TextureLoader(name);
			parameter.format = loader.Format();
			
			SetParameter(parameter);
			SetData(loader.Data(), loader.Width(), loader.Height(), loader.Format());
		}
		catch (ErrorException e)
		{
			Unbind();
			throw e;
		}
		
		Unbind();
	}
	
	Texture::Texture(const std::string& name, const TextureParameter& parameter, bool isLinear)
	{
		glGenTextures(1, &_name);
		RN_CHECKOPENGL();
		
		_width = _height = 0;
		_depth = 1;
		
		_isLinear = (!Settings::SharedInstance()->GammaCorrection()) ? true : isLinear;
		_isCompleteTexture = false;
		_hasChanged = false;
		
		SetType(parameter.type);
		
		Bind();
		SetParameter(parameter);
		
		try
		{
			TextureLoader loader = TextureLoader(name);
			SetData(loader.Data(), loader.Width(), loader.Height(), loader.Format());
		}
		catch (ErrorException e)
		{
			Unbind();
			throw e;
		}
		
		Unbind();
	}
	
	Texture::~Texture()
	{
		glDeleteTextures(1, &_name);
	}
	
	
	Texture *Texture::WithFile(const std::string& name, bool isLinear)
	{
		Texture *texture = new Texture(name, isLinear);
		return texture->Autorelease();
	}
	
	Texture *Texture::WithFile(const std::string& name, const TextureParameter& parameter, bool isLinear)
	{
		Texture *texture = new Texture(name, parameter, isLinear);
		return texture->Autorelease();
	}
	
	
	
	void Texture::Bind()
	{
		Thread *thread = Thread::CurrentThread();
		
		if(thread->CurrentTexture() != this)
			glBindTexture(_glType, _name);
		
		thread->PushTexture(this);
	}
	
	void Texture::Unbind()
	{
		Thread *thread = Thread::CurrentThread();
		if(thread->CurrentTexture() == this)
		{
			if(_hasChanged && _isCompleteTexture)
			{
				glFlush();
				_hasChanged = false;
			}
			
			thread->PopTexture();
			
			Texture *other = thread->CurrentTexture();
			if(other && other != this)
				glBindTexture(other->_glType, other->_name);
		}
	}
	
	
	void Texture::SetDepth(uint32 depth)
	{
		_depth = depth;
	}
	
	void Texture::SetParameter(const TextureParameter& parameter)
	{
		_parameter = parameter;
		
		Bind();
		
		GLenum wrapMode;
		GLenum minFilter;
		GLenum magFilter;
		
		switch(parameter.wrapMode)
		{
			case TextureParameter::WrapMode::Clamp:
				wrapMode = GL_CLAMP_TO_EDGE;
				break;
				
			case TextureParameter::WrapMode::Repeat:
				wrapMode = GL_REPEAT;
				break;
		}
		
		switch(parameter.filter)
		{
			case TextureParameter::Filter::Linear:
				minFilter = GL_LINEAR;
				magFilter = GL_LINEAR;
				
				if(parameter.mipMaps > 0)
					minFilter = GL_LINEAR_MIPMAP_LINEAR;
				
				break;
				
			case TextureParameter::Filter::Nearest:
				minFilter = GL_NEAREST;
				magFilter = GL_NEAREST;
				break;
		}
		
		glTexParameteri(_glType, GL_TEXTURE_WRAP_S, wrapMode); RN_CHECKOPENGL_AGGRESSIVE();
		glTexParameteri(_glType, GL_TEXTURE_WRAP_T, wrapMode); RN_CHECKOPENGL_AGGRESSIVE();
		
		glTexParameteri(_glType, GL_TEXTURE_MIN_FILTER, minFilter); RN_CHECKOPENGL_AGGRESSIVE();
		glTexParameteri(_glType, GL_TEXTURE_MAG_FILTER, magFilter); RN_CHECKOPENGL_AGGRESSIVE();
		
		//glTexParameteri(_glType, GL_TEXTURE_MAX_ANISOTROPY, parameter.anisotropy); RN_CHECKOPENGL_AGGRESSIVE();
		glTexParameteri(_glType, GL_TEXTURE_MAX_LEVEL, _parameter.mipMaps);
		
		if(_parameter.depthCompare)
		{
			glTexParameteri(_glType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE); RN_CHECKOPENGL_AGGRESSIVE();
			glTexParameteri(_glType, GL_TEXTURE_COMPARE_FUNC, GL_LESS); RN_CHECKOPENGL_AGGRESSIVE();
		}
		else
		{
			glTexParameteri(_glType, GL_TEXTURE_COMPARE_MODE, GL_NONE); RN_CHECKOPENGL_AGGRESSIVE();
		}
		
		RN_CHECKOPENGL();
		Unbind();
	}
	
	void Texture::SetType(TextureParameter::Type type)
	{
		switch(type)
		{
			case TextureParameter::Type::Texture2D:
				_glType = GL_TEXTURE_2D;
				break;
				
			case TextureParameter::Type::Texture3D:
				_glType = GL_TEXTURE_3D;
				break;
				
			case TextureParameter::Type::Texture2DArray:
				_glType = GL_TEXTURE_2D_ARRAY;
				break;
		}
	}
	
	void Texture::SetData(const void *data, uint32 width, uint32 height, TextureParameter::Format format)
	{
		GLenum glType, glFormat;
		GLint glInternalFormat;
		void *converted;
		
		Bind();
		
		converted = data ? ConvertData(data, width, height, format, _parameter.format) : 0;
		ConvertFormat(_parameter.format, _isLinear, &glFormat, &glInternalFormat, &glType);
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		
		switch(_glType)
		{
			case GL_TEXTURE_2D:
				glTexImage2D(_glType, 0, glInternalFormat, width, height, 0, glFormat, glType, converted);
				break;
				
			case GL_TEXTURE_2D_ARRAY:
			case GL_TEXTURE_3D:
				glTexImage3D(_glType, 0, glInternalFormat, width, height, _depth, 0, glFormat, glType, converted);
				break;
		}

		RN_CHECKOPENGL();
		
		_width  = width;
		_height = height;
		
		_isCompleteTexture = true;
		_hasChanged = true;
		
		UpdateMipmaps();
		Unbind();
		
		if(converted && converted != data)
			free(converted);
	}
	
	void Texture::UpdateData(const void *data, TextureParameter::Format format)
	{
		if(!_isCompleteTexture)
			return; // TODO: Throw an exception
		
		GLenum glType, glFormat;
		GLint glInternalFormat;
		void *converted;
		
		Bind();
		
		converted = ConvertData(data, _width, _height, format, _parameter.format);
		ConvertFormat(_parameter.format, _isLinear, &glFormat, &glInternalFormat, &glType);
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		
		switch(_glType)
		{
			case GL_TEXTURE_2D:
				glTexSubImage2D(_glType, 0, 0, 0, _width, _height, glFormat, glType, converted);
				break;
				
			case GL_TEXTURE_2D_ARRAY:
			case GL_TEXTURE_3D:
				glTexSubImage3D(_glType, 0, 0, 0, 0, _width, _height, _depth, glFormat, glType, converted);
				break;
		}
		
		RN_CHECKOPENGL();
		
		_hasChanged = true;
		
		UpdateMipmaps();
		Unbind();
		
		if(converted != data)
			free(converted);
	}
	
	void Texture::UpdateRegion(const void *data, const Rect& region, TextureParameter::Format format)
	{
		if(!_isCompleteTexture)
			return; // TODO: Throw an exception
		
		GLenum glType, glFormat;
		GLint glInternalFormat;
		void *converted;
		
		Bind();
		
		converted = ConvertData(data, region.width, region.height, format, _parameter.format);
		ConvertFormat(_parameter.format, _isLinear, &glFormat, &glInternalFormat, &glType);
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		
		switch(_glType)
		{
			case GL_TEXTURE_2D:
				glTexSubImage2D(_glType, 0, region.x, region.y, region.width, region.height, glFormat, glType, converted);;
				break;
		}
		
		RN_CHECKOPENGL();
		
		_hasChanged = true;
		
		UpdateMipmaps();
		Unbind();
		
		if(converted != data)
			free(converted);
	}
	
	void Texture::UpdateMipmaps()
	{
		if(!_isCompleteTexture || !_parameter.generateMipMaps)
			return;
		
		Bind();
		
		glGenerateMipmap(_glType);
		_hasChanged = true;
		
		Unbind();
	}
	
#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8 GL_RGBA
#define GL_SRGB8        GL_RGB
	
#define RN_ADDED_GL_SRGB
#endif
	
	void Texture::ConvertFormat(TextureParameter::Format format, bool isLinear, GLenum *glFormat, GLint *glInternalFormat, GLenum *glType)
	{
		RN_ASSERT0(glFormat != 0 && glInternalFormat != 0 && glType != 0);
		
		switch(format)
		{
				// Integer formats
			case TextureParameter::Format::RGBA8888:
				*glFormat = GL_RGBA;
				*glInternalFormat = isLinear ? GL_RGBA : GL_SRGB8_ALPHA8;
				*glType   = GL_UNSIGNED_BYTE;
				break;
				
			case TextureParameter::Format::RGBA4444:
				*glFormat = GL_RGBA;
				*glInternalFormat = isLinear ? GL_RGBA : GL_SRGB8_ALPHA8;
				*glType   = GL_UNSIGNED_SHORT_4_4_4_4;
				break;
				
			case TextureParameter::Format::RGBA5551:
				*glFormat = GL_RGBA;
				*glInternalFormat = isLinear ? GL_RGBA : GL_SRGB8_ALPHA8;
				*glType   = GL_UNSIGNED_SHORT_5_5_5_1;
				break;
				
			case TextureParameter::Format::RGB565:
				*glFormat = GL_RGB;
				*glInternalFormat = isLinear ? GL_RGB : GL_SRGB8;
				*glType   = GL_UNSIGNED_SHORT_5_6_5;
				break;
				
#if RN_TARGET_OPENGL
			case TextureParameter::Format::R8:
				*glFormat = GL_RED;
				*glInternalFormat = GL_RED;
				*glType = GL_UNSIGNED_BYTE;
				break;
				
			case TextureParameter::Format::RG88:
				*glFormat = GL_RG;
				*glInternalFormat = GL_RGB;
				*glType = GL_UNSIGNED_BYTE;
				break;
#endif
				
			case TextureParameter::Format::RGB888:
				*glFormat = GL_RGB;
				*glInternalFormat = isLinear ? GL_RGB : GL_SRGB8;
				*glType = GL_UNSIGNED_BYTE;
				break;
				
				// Floating point formats
			case TextureParameter::Format::RGBA32F:
				*glFormat = GL_RGBA;
				*glInternalFormat = GL_RGBA32F;
				*glType = GL_FLOAT;
				break;
				
			case TextureParameter::Format::R32F:
				*glFormat = GL_RED;
				*glInternalFormat = GL_R32F;
				*glType = GL_FLOAT;
				break;
				
			case TextureParameter::Format::RG32F:
				*glFormat = GL_RG;
				*glInternalFormat = GL_RG32F;
				*glType = GL_FLOAT;
				break;
				
			case TextureParameter::Format::RGB32F:
				*glFormat = GL_RGB;
				*glInternalFormat = GL_RGB32F;
				*glType = GL_FLOAT;
				break;
				
				// Half floats
			case TextureParameter::Format::RGBA16F:
				*glFormat = GL_RGBA;
				*glInternalFormat = GL_RGBA16F;
				*glType = GL_HALF_FLOAT;
				break;
				
			case TextureParameter::Format::R16F:
				*glFormat = GL_RED;
				*glInternalFormat = GL_R16F;
				*glType = GL_HALF_FLOAT;
				break;
				
			case TextureParameter::Format::RG16F:
				*glFormat = GL_RG;
				*glInternalFormat = GL_RG16F;
				*glType = GL_HALF_FLOAT;
				break;
				
			case TextureParameter::Format::RGB16F:
				*glFormat = GL_RGB;
				*glInternalFormat = GL_RGB16F;
				*glType = GL_HALF_FLOAT;
				break;
				
			case TextureParameter::Format::Depth:
				*glFormat = GL_DEPTH_COMPONENT;
				*glInternalFormat = GL_DEPTH_COMPONENT24;
				*glType = GL_UNSIGNED_BYTE;
				break;
				
			case TextureParameter::Format::DepthStencil:
				*glFormat = GL_DEPTH_STENCIL;
				*glInternalFormat = GL_DEPTH24_STENCIL8;
				*glType = GL_UNSIGNED_INT_24_8;
				break;
				
			default:
				throw ErrorException(0, 0, 0); // Todo throw an actual error exception!
				break;
		}
	}
	
#ifdef RN_ADDED_GL_SRGB
#undef GL_SRGB8_ALPHA8
#undef GL_SRGB8
#undef RN_ADDED_GL_SRGB
#endif
	
	RN_INLINE uint8 *ReadPixel(uint8 *data, TextureParameter::Format format, uint32 *r, uint32 *g, uint32 *b, uint32 *a)
	{
		RN_ASSERT0(r && g && b && a);
		
		switch(format)
		{
			case TextureParameter::Format::RGBA8888:
			{
				uint32 *pixel = (uint32 *)data;
				
				*a = ((*pixel >> 24) & 0xFF);
				*b = ((*pixel >> 16) & 0xFF);
				*g = ((*pixel >> 8) & 0xFF);
				*r = ((*pixel >> 0) & 0xFF);
				
				return (uint8 *)(pixel + 1);
				break;
			}
				
			case TextureParameter::Format::RGBA4444:
			{
				uint16 *pixel = (uint16 *)data;
				
				*r = ((*pixel >> 12) & 0xF) << 4;
				*g = ((*pixel >> 8)  & 0xF) << 4;
				*b = ((*pixel >> 4)  & 0xF) << 4;
				*a = ((*pixel >> 0)  & 0xF) << 4;
				
				return (uint8 *)(pixel + 1);
				break;
			}
				
			case TextureParameter::Format::RGBA5551:
			{
				uint16 *pixel = (uint16 *)data;
				
				*r = ((*pixel >> 11) & 0xFF) << 3;
				*g = ((*pixel >> 6)  & 0xFF) << 3;
				*b = ((*pixel >> 1)  & 0xFF) << 3;
				*a = ((*pixel >> 0)  & 0xFF) << 7;
				
				return (uint8 *)(pixel + 1);
				break;
			}
				
			case TextureParameter::Format::RGB565:
			{
				uint16 *pixel = (uint16 *)data;
				
				*r = ((*pixel >> 11) & 0xFF) << 3;
				*g = ((*pixel >> 5)  & 0xFF) << 2;
				*b = ((*pixel >> 0)  & 0xFF) << 3;
				*a = 255;
				
				return (uint8 *)(pixel + 1);
				break;
			}
				
			case TextureParameter::Format::R8:
			{
				*r = *data;
				*g = *b = 0;
				*a = 255;
				
				return (data + 1);
				break;
			}
				
			case TextureParameter::Format::RG88:
			{
				uint16 *pixel = (uint16 *)data;
				
				*r = ((*pixel >> 8)  & 0xFF);
				*g = ((*pixel >> 0)  & 0xFF);
				*b = 0;
				*a = 255;
				
				return (uint8 *)(pixel + 1);
				break;
			}
				
			case TextureParameter::Format::RGB888:
			{
				uint16 *pixelRG = (uint16 *)data;
				uint8 *pixelB = (uint8 *)(pixelRG + 1);
				
				*r = ((*pixelRG >> 8)  & 0xFF);
				*g = ((*pixelRG >> 0)  & 0xFF);
				*b = *pixelB;
				*a = 255;
				
				return (uint8 *)(pixelB + 1);
				break;
			}
				
			default:
				throw ErrorException(0, 0, 0);
				break;
		}
		
		return data;
	}
	
	void *Texture::ConvertData(const void *data, uint32 width, uint32 height, TextureParameter::Format current, TextureParameter::Format target)
	{
		if(current == target)
			return (void *)data;
		
		RN_ASSERT0(data && width > 0 && height > 0);
		
		size_t pixel = width * height;
		
		uint8 *temp = (uint8 *)data;
		void  *result = 0;
		
		// Convert data to the specified target format
		switch(target)
		{
			case TextureParameter::Format::RGBA8888:
			{
				result = malloc(pixel * sizeof(uint32));
				
				uint32 *outPixel = (uint32 *)result;
				for(size_t i=0; i<pixel; i++)
				{
					uint32 r, g, b, a;
					temp = ReadPixel(temp, current, &r, &g, &b, &a);
					
					*outPixel ++ = (a << 24) | (b << 16) | (g << 8) | r;
				}
				
				break;
			}
				
			case TextureParameter::Format::RGBA4444:
			{
				result = malloc(pixel * sizeof(uint16));
				
				uint16 *outPixel = (uint16 *)result;
				for(size_t i=0; i<pixel; i++)
				{
					uint32 r, g, b, a;
					temp = ReadPixel(temp, current, &r, &g, &b, &a);
					
					r = (r >> 4);
					g = (g >> 4);
					b = (b >> 4);
					a = (a >> 4);
					
					*outPixel ++ = (r << 12) | (g << 8) | (b << 4) | a;
				}
				
				break;
			}
				
			case TextureParameter::Format::RGBA5551:
			{
				result = malloc(pixel * sizeof(uint16));
				
				uint16 *outPixel = (uint16 *)result;
				for(size_t i=0; i<pixel; i++)
				{
					uint32 r, g, b, a;
					temp = ReadPixel(temp, current, &r, &g, &b, &a);
					
					r = (r >> 3);
					g = (g >> 3);
					b = (b >> 3);
					a = (a >> 7);
					
					*outPixel ++ = (r << 11) | (g << 6) | (b << 1) | a;
				}
				
				break;
			}
				
			case TextureParameter::Format::RGB565:
			{
				result = malloc(pixel * sizeof(uint16));
				
				uint16 *outPixel = (uint16 *)result;
				for(size_t i=0; i<pixel; i++)
				{
					uint32 r, g, b, a;
					temp = ReadPixel(temp, current, &r, &g, &b, &a);
					
					r = (r >> 3);
					g = (g >> 2);
					b = (b >> 3);
					
					*outPixel ++ = (r << 11) | (g << 5) | b;
				}
				
				break;
			}
				
			case TextureParameter::Format::R8:
			{
				result = malloc(pixel * sizeof(uint8));
				
				uint8 *outPixel = (uint8 *)result;
				for(size_t i=0; i<pixel; i++)
				{
					uint32 r, g, b, a;
					temp = ReadPixel(temp, current, &r, &g, &b, &a);
					
					*outPixel ++ = r;
				}
				
				break;
			}
				
			case TextureParameter::Format::RG88:
			{
				result = malloc(pixel * sizeof(uint16));
				
				uint16 *outPixel = (uint16 *)result;
				for(size_t i=0; i<pixel; i++)
				{
					uint32 r, g, b, a;
					temp = ReadPixel(temp, current, &r, &g, &b, &a);
					
					*outPixel ++ = (r << 8) | g;
				}
				
				break;
			}
				
			case TextureParameter::Format::RGB888:
			{
				result = malloc(pixel * (sizeof(uint8) * 3));
				
				uint8 *outPixel = (uint8 *)result;
				for(size_t i=0; i<pixel; i++)
				{
					uint32 r, g, b, a;
					temp = ReadPixel(temp, current, &r, &g, &b, &a);
					
					*outPixel ++ = r;
					*outPixel ++ = g;
					*outPixel ++ = b;
				}
				
				break;
			}
				
			default:
				throw ErrorException(0, 0, 0); // Todo throw an actual error exception!
				break;
		}
		
		return result;
	}
	
	
	bool Texture::PlatformSupportsFormat(TextureParameter::Format format)
	{
		try
		{
			GLenum glFormat;
			GLint glInternalFormat;
			GLenum glType;
			
			ConvertFormat(format, true, &glFormat, &glInternalFormat, &glType);
			return true;
		}
		catch(ErrorException e)
		{
			return false;
		}
	}
	
	uint32 Texture::MaxAnisotropyLevel()
	{
		GLint max;
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max);
		
		return max;
	}
	
	uint32 Texture::DefaultAnisotropyLevel()
	{
		return _defaultAnisotropy;
	}
	
	void Texture::SetDefaultAnisotropyLevel(uint32 level)
	{
		_defaultAnisotropy = level;
	}
}