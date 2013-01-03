//
//  RNTexture.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTexture.h"
#include "RNTextureLoader.h"
#include "RNThread.h"

namespace RN
{
	Texture::Texture(Format format)
	{
		glGenTextures(1, &_name);
		
		_width = _height = 0;
		_format = format;
		
		Bind();
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
		Unbind();
	}
	
	Texture::Texture(const std::string& name, Format format)
	{
		TextureLoader loader = TextureLoader(name);
		
		glGenTextures(1, &_name);
		
		_width = _height = 0;
		_format = format;
		
		Bind();
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
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
			thread->PopTexture();
			
			Texture *other = thread->CurrentTexture();
			if(other && other != this)
				glBindTexture(GL_TEXTURE_2D, other->_name);
		}
	}
	
	
	
	void Texture::SetData(const void *data, uint32 width, uint32 height, Format format)
	{		
		GLenum glType, glFormat;
		void *converted;
		
		Bind();
		WillChangeData();
		
		converted = ConvertData(data, width, height, format, _format);
		ConvertFormat(_format, &glFormat, &glType);
		
		_width  = width;
		_height = height;
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, glFormat, _width, _height, 0, glFormat, glType, converted);
		
		DidChangeData();
		Unbind();
		
		if(converted != data)
			free(converted);
	}
	
	void Texture::UpdateData(const void *data, Format format)
	{
		GLenum glType, glFormat;
		void *converted;
		
		Bind();
		WillChangeData();
		
		converted = ConvertData(data, _width, _height, format, _format);
		ConvertFormat(_format, &glFormat, &glType);
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, glFormat, glType, converted);
		
		DidChangeData();
		Unbind();
		
		if(converted != data)
			free(converted);
	}
	
	
	void Texture::ConvertFormat(Format format, GLenum *glFormat, GLenum *glType)
	{
		RN_ASSERT0(glFormat != 0 && glType != 0);
		
		switch(format)
		{
			case RGBA8888:
				*glFormat = GL_RGBA;
				*glType   = GL_UNSIGNED_BYTE;
				break;
				
			case RGBA4444:
				*glFormat = GL_RGBA;
				*glType   = GL_UNSIGNED_SHORT_4_4_4_4;
				break;
				
			case RGBA5551:
				*glFormat = GL_RGBA;
				*glType   = GL_UNSIGNED_SHORT_5_5_5_1;
				break;
				
			case RGB565:
				*glFormat = GL_RGB;
				*glType   = GL_UNSIGNED_SHORT_5_6_5;
				break;
				
			default:
				throw ErrorException(0, 0, 0); // Todo throw an actual error exception!
				break;
		}
	}
	
	void *Texture::ConvertData(const void *data, uint32 width, uint32 height, Format current, Format target)
	{
		if(current == target)
			return (void *)data;
		
		void *intermediate = 0;
		void *result = 0;
		size_t pixel = width * height;
		
		// Promote data to RGBA8888
		switch(current)
		{
			case RGBA8888:
				intermediate = (void *)data;
				break;
				
			/*case RGBA8888:
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
				
			case RGBA4444:
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
				
			case RGBA5551:
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
				
			case RGB565:
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
			case RGBA4444:
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
				
			case RGBA5551:
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
			
			case RGB565:
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
			case PVRTC2:
			case PVRTC4:
				return false;
				break;
				
			default:
				return true;
		}
	}
}
