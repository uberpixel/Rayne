//
//  RNTexture.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTexture.h"
#include "RNTextureLoader.h"
#include "RNThread.h"
#include "RNKernel.h"

namespace RN
{
	uint32 Texture::_defaultAnisotropy = 0;
	
	Texture::Texture(Format format, WrapMode wrap, Filter filter, bool isLinear)
	{
		glGenTextures(1, &_name);
		
		_width = _height = 0;
		_format = format;
		_isLinear = isLinear;
		_anisotropy = 0;
		
		_generateMipmaps = true;
		_isCompleteTexture = false;
		_hasChanged = false;
		
		Bind();
		
		SetFilter(filter);
		SetWrappingMode(wrap);
		SetAnisotropyLevel(_defaultAnisotropy);
		
		Unbind();
	}
	
	Texture::Texture(const std::string& name, Format format, WrapMode wrap, Filter filter, bool isLinear)
	{
		TextureLoader loader = TextureLoader(name);
		
		glGenTextures(1, &_name);
		
		_width = _height = 0;
		_format = format;
		_isLinear = isLinear;
		
		_generateMipmaps = true;
		_isCompleteTexture = false;
		_hasChanged = false;
		
		Bind();
		
		SetFilter(filter);
		SetWrappingMode(wrap);
		
		try
		{
			SetData(loader.Data(), loader.Width(), loader.Height(), loader.Format());
		}
		catch (ErrorException exception)
		{
			printf("Caught exception!");
		}
		
		
		Unbind();
	}

	Texture::~Texture()
	{
		glDeleteTextures(1, &_name);
	}
	
	
	void Texture::Bind()
	{
		Thread *thread = Thread::CurrentThread();
		
		if(thread->CurrentTexture() != this)
			glBindTexture(GL_TEXTURE_2D, _name);
		
		thread->PushTexture(this);
	}
	
	void Texture::Unbind()
	{
		Thread *thread = Thread::CurrentThread();
		if(thread->CurrentTexture() == this)
		{
			if(_hasChanged && _isCompleteTexture)
				glFlush();
			
			thread->PopTexture();
			
			Texture *other = thread->CurrentTexture();
			if(other && other != this)
				glBindTexture(GL_TEXTURE_2D, other->_name);
		}
	}
	
	
	void Texture::SetFormat(Format format)
	{
		_format = format;
	}
	
	void Texture::SetWrappingMode(WrapMode wrap)
	{
		_wrapMode = wrap;
		
		Bind();
		
		GLenum mode;
		
		switch(wrap)
		{
			case WrapModeClamp:
				mode = GL_CLAMP_TO_EDGE;
				break;
				
			case WrapModeRepeat:
				mode = GL_REPEAT;
				break;
		}
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode);
		
		_hasChanged = true;
		
		Unbind();
	}
	
	void Texture::SetFilter(Filter filter)
	{
		_filter = filter;
		
		Bind();
		
		GLenum minFilter;
		GLenum magFilter;
		
		switch(filter)
		{
			case FilterLinear:
				minFilter = GL_LINEAR;
				magFilter = GL_LINEAR;
				
				if(_generateMipmaps)
					minFilter = GL_LINEAR_MIPMAP_LINEAR;
				
				break;
				
			case FilterNearest:
				minFilter = GL_NEAREST;
				magFilter = GL_NEAREST;
				break;
		}
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
		
		_hasChanged = true;
		
		Unbind();
	}
	
	void Texture::SetGeneratesMipmaps(bool genMipmaps)
	{
		if(genMipmaps != _generateMipmaps)
		{
			if(genMipmaps)
			{
				UpdateMipmaps();
			}
			
			_generateMipmaps = genMipmaps;
			SetFilter(_filter);
		}
	}
	
	void Texture::SetAnisotropyLevel(uint32 level)
	{
		if(_anisotropy != level)
		{
			_anisotropy = level;
			
			Bind();
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, level);
			_hasChanged = true;
			
			Unbind();
		}
	}
	
	void Texture::SetData(const void *data, uint32 width, uint32 height, Format format)
	{		
		GLenum glType, glFormat;
		GLint glInternalFormat;
		void *converted;
		
		Bind();
		
		converted = ConvertData(data, width, height, format, _format);
		ConvertFormat(_format, _isLinear, &glFormat, &glInternalFormat, &glType);
		
		_width  = width;
		_height = height;
		
		_isCompleteTexture = true;
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, _width, _height, 0, glFormat, glType, converted);
		
		_hasChanged = true;
		
		UpdateMipmaps();
		Unbind();
		
		if(converted != data)
			free(converted);
	}
	
	void Texture::UpdateData(const void *data, Format format)
	{
		if(!_isCompleteTexture)
			return; // TODO: Throw an exception
		
		GLenum glType, glFormat;
		GLint glInternalFormat;
		void *converted;
		
		Bind();
		
		converted = ConvertData(data, _width, _height, format, _format);
		ConvertFormat(_format, _isLinear, &glFormat, &glInternalFormat, &glType);
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, glFormat, glType, converted);
		
		_hasChanged = true;
		
		UpdateMipmaps();		
		Unbind();
		
		if(converted != data)
			free(converted);
	}
	
	void Texture::UpdateMipmaps()
	{
		if(!_generateMipmaps || !_isCompleteTexture)
			return;
		
		Bind();
		
		glGenerateMipmap(GL_TEXTURE_2D);
		_hasChanged = true;
		
		Unbind();
	}
	
#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8 GL_RGBA
#define GL_SRGB8        GL_RGB
	
#define RN_ADDED_GL_SRGB
#endif
	
	void Texture::ConvertFormat(Format format, bool isLinear, GLenum *glFormat, GLint *glInternalFormat, GLenum *glType)
	{
		RN_ASSERT0(glFormat != 0 && glType != 0);
		
		switch(format)
		{
			case FormatRGBA8888:
				*glFormat = GL_RGBA;
				*glInternalFormat = isLinear ? GL_RGBA : GL_SRGB8_ALPHA8;
				*glType   = GL_UNSIGNED_BYTE;
				break;
				
			case FormatRGBA4444:
				*glFormat = GL_RGBA;
				*glInternalFormat = isLinear ? GL_RGBA : GL_SRGB8_ALPHA8;
				*glType   = GL_UNSIGNED_SHORT_4_4_4_4;
				break;
				
			case FormatRGBA5551:
				*glFormat = GL_RGBA;
				*glInternalFormat = isLinear ? GL_RGBA : GL_SRGB8_ALPHA8;
				*glType   = GL_UNSIGNED_SHORT_5_5_5_1;
				break;
				
			case FormatRGB565:
				*glFormat = GL_RGB;
				*glInternalFormat = isLinear ? GL_RGB : GL_SRGB8;
				*glType   = GL_UNSIGNED_SHORT_5_6_5;
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
	
	void *Texture::ConvertData(const void *data, uint32 width, uint32 height, Format current, Format target)
	{
		if(current == target)
			return (void *)data;
		
		void *intermediate = 0;
		void *result = 0;
		size_t pixel = width * height;
		
		RN_ASSERT0(width > 0 && height > 0);
		RN_ASSERT0(pixel > 0);
		
		// Promote data to RGBA8888
		switch(current)
		{
			case FormatRGBA8888:
				intermediate = (void *)data;
				break;
				
			/*case FormatRGBA8888:
			{
				intermediate = malloc(pixel * sizeof(uint32));
				
				uint32 *inPixel  = (uint32 *)data;
				uint32 *outPixel = (uint32 *)intermediate;
				
				for(size_t i=0; i<pixel; i++, inPixel++)
				{
					uint32 r = ((*inPixel >> 24) & 0xFF);
					uint32 g = ((*inPixel >> 16) & 0xFF);
					uint32 b = ((*inPixel >> 8)  & 0xFF);
					uint32 a = ((*inPixel >> 0)  & 0xFF);
					
					*outPixel ++ = (a << 24) | (b << 16) | (g << 8) | r;
				}
				
				break;
			}*/
				
			case FormatRGBA4444:
			{
				intermediate = malloc(pixel * sizeof(uint32));
				
				uint16 *inPixel  = (uint16 *)data;
				uint32 *outPixel = (uint32 *)intermediate;
				
				for(size_t i=0; i<pixel; i++, inPixel++)
				{
					uint32 r = ((*inPixel >> 12) & 0xF) << 4;
					uint32 g = ((*inPixel >> 8)  & 0xF) << 4;
					uint32 b = ((*inPixel >> 4)  & 0xF) << 4;
					uint32 a = ((*inPixel >> 0)  & 0xF) << 4;
					
					*outPixel ++ = (a << 24) | (b << 16) | (g << 8) | r;
				}
				
				break;
			}
				
			case FormatRGBA5551:
			{
				intermediate = malloc(pixel * sizeof(uint32));
				
				uint16 *inPixel  = (uint16 *)data;
				uint32 *outPixel = (uint32 *)intermediate;
				
				for(size_t i=0; i<pixel; i++, inPixel++)
				{
					uint32 r = ((*inPixel >> 11) & 0xFF) << 3;
					uint32 g = ((*inPixel >> 6)  & 0xFF) << 3;
					uint32 b = ((*inPixel >> 1)  & 0xFF) << 3;
					uint32 a = ((*inPixel >> 0)  & 0xFF) << 7;
					
					*outPixel ++ = (a << 24) | (b << 16) | (g << 8) | r;
				}
				
				break;
			}
				
			case FormatRGB565:
			{
				intermediate = malloc(pixel * sizeof(uint32));
				
				uint16 *inPixel  = (uint16 *)data;
				uint32 *outPixel = (uint32 *)intermediate;
				
				for(size_t i=0; i<pixel; i++, inPixel++)
				{
					uint32 r = ((*inPixel >> 11) & 0xFF) << 3;
					uint32 g = ((*inPixel >> 5)  & 0xFF) << 2;
					uint32 b = ((*inPixel >> 0)  & 0xFF) << 3;
					uint32 a = 255;
					
				   *outPixel ++ = (a << 24) | (b << 16) | (g << 8) | r;
				}
			}
				
			default:
				throw ErrorException(0, 0, 0); // Todo throw an actual error exception!
				break;
		}
		
		// Convert data to the specified target format
		switch(target)
		{
			case FormatRGBA4444:
			{
				result = malloc(pixel * sizeof(uint16));
				
				uint32 *inPixel  = (uint32 *)intermediate;
				uint16 *outPixel = (uint16 *)result;
				
				for(size_t i=0; i<pixel; i++, inPixel++)
				{
					uint32 r = (((*inPixel >> 0)  & 0xFF) >> 4);
					uint32 g = (((*inPixel >> 8)  & 0xFF) >> 4);
					uint32 b = (((*inPixel >> 16) & 0xFF) >> 4);
					uint32 a = (((*inPixel >> 24) & 0xFF) >> 4);
					
					*outPixel ++ = (r << 12) | (g << 8) | (b << 4) | a;
				}
				
				break;
			}
				
			case FormatRGBA5551:
			{
				result = malloc(pixel * sizeof(uint16));
				
				uint32 *inPixel = (uint32 *)intermediate;
				uint16 *outPixel = (uint16 *)result;
				
				for(size_t i=0; i<pixel; i++, inPixel++)
				{
					uint32 r = (((*inPixel >> 0)  & 0xFF) >> 3);
					uint32 g = (((*inPixel >> 8)  & 0xFF) >> 3);
					uint32 b = (((*inPixel >> 16) & 0xFF) >> 3);
					uint32 a = (((*inPixel >> 24) & 0xFF) >> 7);
					
					*outPixel ++ = (r << 11) | (g << 6) | (b << 1) | a;
				}
				
				break;
			}
			
			case FormatRGB565:
			{
				result = malloc(pixel * sizeof(uint16));
				
				uint32 *inPixel = (uint32 *)intermediate;
				uint16 *outPixel = (uint16 *)result;
				
				for(size_t i=0; i<pixel; i++, inPixel++)
				{
					uint32 r = (((*inPixel >> 0) & 0xFF) >> 3);
					uint32 g = (((*inPixel >> 8) & 0xFF) >> 2);
					uint32 b = (((*inPixel >> 16)  & 0xFF) >> 3);
					
					*outPixel ++ = (r << 11) | (g << 5) | (b << 0);
				}
				
				break;
			}
				
			default:
				throw ErrorException(0, 0, 0); // Todo throw an actual error exception!
				break;
		}
		
		if(intermediate != data)
			free(intermediate);
		
		return result;
	}
	
	
	bool Texture::PlatformSupportsFormat(Format format)
	{
		switch(format)
		{
			case FormatPVRTC2:
			case FormatPVRTC4:
				return false;
				break;
				
			default:
				return true;
		}
	}
	
	uint32 Texture::MaxAnisotropyLevel()
	{
		GLint max;
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
		
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
