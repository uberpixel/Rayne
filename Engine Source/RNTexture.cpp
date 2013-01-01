//
//  RNTexture.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNTexture.h"

namespace RN
{
	static Texture *__CurrentTexture = 0;
	
	Texture::Texture(Format format)
	{
		glGenTextures(1, &_name);
		
		_width = _height = 0;
		_bound = false;
		_previous = 0;
		_format = format;
		
		Bind();
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		
		Unbind();
	}

	
	
	
	Texture::~Texture()
	{
		if(_bound)
			Unbind();
		
		glDeleteTextures(1, &_name);
	}
	
	
	void Texture::Bind()
	{
		if(!_bound)
		{
			_bound = true;
			_previous = __CurrentTexture;
			__CurrentTexture = this;
			
			glBindTexture(GL_TEXTURE_2D, _name);
		}
	}
	
	void Texture::Unbind()
	{
		if(_bound)
		{
			RN::Assert(__CurrentTexture == this);
			
			__CurrentTexture = _previous;
			_bound = false;
			
			if(_previous)
				glBindTexture(GL_TEXTURE_2D, _previous->_name);
		}
	}
	
	
	
	void Texture::SetData(const std::vector<uint8>& data, uint32 width, uint32 height, Format format)
	{
		RN::Assert(this == __CurrentTexture);
		
		GLenum glType, glFormat;
		std::vector<uint8> converted;
		
		converted = ConvertData(data, width, height, format, _format);
		ConvertFormat(_format, &glFormat, &glType);
		
		_width  = width;
		_height = height;
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, glFormat, width, height, 0, glFormat, glType, &converted[0]);
	}
	
	void Texture::UpdateData(const std::vector<uint8>& data, Format format)
	{
		RN::Assert(this == __CurrentTexture);
		
		GLenum glType, glFormat;
		std::vector<uint8> converted;
		
		converted = ConvertData(data, _width, _height, format, _format);
		ConvertFormat(_format, &glFormat, &glType);
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, glFormat, glType, &converted[0]);
	}
	
	
	void Texture::ConvertFormat(Format format, GLenum *glFormat, GLenum *glType)
	{
		RN::Assert(glFormat != 0 && glType != 0);
		
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
	
	std::vector<uint8> Texture::ConvertData(const std::vector<uint8>& data, uint32 width, uint32 height, Format current, Format target)
	{
		if(current == target)
			return data;
		
		std::vector<uint8> intermediate, result;
		size_t pixel = width * height;
		
		// Promote data to RGBA8888
		intermediate.resize(pixel * 4);
		
		switch(current)
		{
			case ARGB8888:
			{
				uint32 *inPixel  = (uint32 *)&data[0];
				uint32 *outPixel = (uint32 *)&intermediate[0];
				
				for(size_t i=0; i<pixel; i++, inPixel++)
                {
					uint32 r = ((*inPixel >> 16) & 0xFF);
					uint32 g = ((*inPixel >> 8)  & 0xFF);
					uint32 b = ((*inPixel >> 0)  & 0xFF);
					uint32 a = ((*inPixel >> 24) & 0xFF);
					
                    *outPixel ++ = (r << 24) | (g << 16) | (b << 8) | a;
                }
				
				break;
			}
				
			case RGBA4444:
			{
				uint16 *inPixel  = (uint16 *)&data[0];
				uint32 *outPixel = (uint32 *)&intermediate[0];
				
				for(size_t i=0; i<pixel; i++, inPixel++)
                {
					uint32 r = ((*inPixel >> 12) & 0xF) << 4;
					uint32 g = ((*inPixel >> 8)  & 0xF) << 4;
					uint32 b = ((*inPixel >> 4)  & 0xF) << 4;
					uint32 a = ((*inPixel >> 0)  & 0xF) << 4;
					
                    *outPixel ++ = (r << 24) | (g << 16) | (b << 8) | a;
                }
				
				break;
			}
				
			case RGBA5551:
			{
				uint16 *inPixel  = (uint16 *)&data[0];
				uint32 *outPixel = (uint32 *)&intermediate[0];
				
				for(size_t i=0; i<pixel; i++, inPixel++)
                {
					uint32 r = ((*inPixel >> 11) & 0xFF) << 3;
					uint32 g = ((*inPixel >> 6)  & 0xFF) << 3;
					uint32 b = ((*inPixel >> 1)  & 0xFF) << 3;
					uint32 a = ((*inPixel >> 0)  & 0xFF) << 7;
					
                    *outPixel ++ = (r << 24) | (g << 16) | (b << 8) | a;
                }
				
				break;
			}
				
			case RGB565:
			{
				uint16 *inPixel  = (uint16 *)&data[0];
				uint32 *outPixel = (uint32 *)&intermediate[0];
				
				for(size_t i=0; i<pixel; i++, inPixel++)
                {
					uint32 r = ((*inPixel >> 11) & 0xFF) << 3;
					uint32 g = ((*inPixel >> 5)  & 0xFF) << 2;
					uint32 b = ((*inPixel >> 0)  & 0xFF) << 3;
					
                    *outPixel ++ = (r << 24) | (g << 16) | (b << 8) | 255;
                }
			}
				
			default:
				throw ErrorException(0, 0, 0); // Todo throw an actual error exception!
				break;			
		}
		
		if(target == RGBA8888)
			return intermediate;
		
		// Convert data to the specified target format
		switch(target)
		{
			case RGBA4444:
			{
				result.resize(pixel * 2);
				
				uint32 *inPixel = (uint32 *)&intermediate[0];
				uint16 *outPixel = (uint16 *)&result[0];
				
				for(size_t i=0; i<pixel; i++, inPixel++)
                {
                    uint32 r = (((*inPixel >> 24) & 0xFF) >> 4);
                    uint32 g = (((*inPixel >> 16) & 0xFF) >> 4);
                    uint32 b = (((*inPixel >> 8)  & 0xFF) >> 4);
					uint32 a = (((*inPixel >> 0)  & 0xFF) >> 4);
                    
                    *outPixel ++ = (r << 12) | (g << 8) | (b << 4) | a;
                }
			}
				
			case RGBA5551:
			{
				result.resize(pixel * 2);
				
				uint32 *inPixel = (uint32 *)&intermediate[0];
				uint16 *outPixel = (uint16 *)&result[0];
				
				for(size_t i=0; i<pixel; i++, inPixel++)
                {
                    uint32 r = (((*inPixel >> 24) & 0xFF) >> 3);
                    uint32 g = (((*inPixel >> 16) & 0xFF) >> 3);
                    uint32 b = (((*inPixel >> 8)  & 0xFF) >> 3);
					uint32 a = (((*inPixel >> 0)  & 0xFF) >> 7);
                    
                    *outPixel ++ = (r << 11) | (g << 6) | (b << 1) | a;
                }
			}
				
			case RGB565:
			{
				result.resize(pixel * 2);
				
				uint32 *inPixel = (uint32 *)&intermediate[0];
				uint16 *outPixel = (uint16 *)&result[0];
				
                for(size_t i=0; i<pixel; i++, inPixel++)
                {
                    uint32 r = (((*inPixel >> 24) & 0xFF) >> 3);
                    uint32 g = (((*inPixel >> 16) & 0xFF) >> 2);
                    uint32 b = (((*inPixel >> 8)  & 0xFF) >> 3);
                    
                    *outPixel ++ = (r << 11) | (g << 5) | (b << 0);
                }
				
				break;
			}
				
			default:
				throw ErrorException(0, 0, 0); // Todo throw an actual error exception!
				break;
		}
		
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
