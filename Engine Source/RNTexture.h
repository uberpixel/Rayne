//
//  RNTexture.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TEXTURE_H__
#define __RAYNE_TEXTURE_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Texture : public Object, public BlockingProxy
	{
	public:
		typedef enum
		{
			RGBA8888,
			RGBA4444,
			RGBA5551,
			RGB565,
			
			PVRTC4,
			PVRTC2
		} Format;
		
		Texture(Format format);
		Texture(const std::string& name, Format format);
		
		virtual ~Texture();
		
		void Bind();
		void Unbind();
		
		GLuint Name() { return _name; }
		
		void SetData(const void *data, uint32 width, uint32 height, Format format);
		void UpdateData(const void *data, Format format);
		
		static bool PlatformSupportsFormat(Format format);
		
	protected:
		GLuint _name;
		uint32 _width, _height;
		
	private:
		static void *ConvertData(const void *data, uint32 width, uint32 height, Format current, Format target);
		static void ConvertFormat(Format format, GLenum *glFormat, GLenum *glType);
		
		Format _format;
	};
}

#endif /* __RAYNE_TEXTURE_H__ */
