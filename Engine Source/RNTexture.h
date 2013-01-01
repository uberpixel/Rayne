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
	class Texture : public Object
	{
	public:
		typedef enum
		{
			ARGB8888,
			RGBA8888,
			RGBA4444,
			RGBA5551,
			RGB565,
			
			PVRTC4,
			PVRTC2			
		} Format;
		
		Texture(Format format);
		
		virtual ~Texture();
		
		void Bind();
		void Unbind();
		
		GLuint Name() { return _name; }
		
		void SetData(const std::vector<uint8>& data, uint32 width, uint32 height, Format format);
		void UpdateData(const std::vector<uint8>& data, Format format);
		
		static bool PlatformSupportsFormat(Format format);
		
		class Proxy : public BlockingProxy
		{
		public:
			Proxy(Texture *texture)
			{
				_texture = texture;
			}
			
			GLuint Name() const
			{
				return _texture->Name();
			}
			
		private:
			Texture *_texture;
		};
		
		Proxy *ProxyObject()
		{
			return &_proxy;
		}
		
	protected:
		GLuint _name;
		uint32 _width, _height;
		
	private:
		static std::vector<uint8> ConvertData(const std::vector<uint8>& data, uint32 width, uint32 height, Format current, Format target);
		static void ConvertFormat(Format format, GLenum *glFormat, GLenum *glType);
		
		Format _format;
		Proxy _proxy;
		
		bool _bound;
		Texture *_previous;
	};
}

#endif /* __RAYNE_TEXTURE_H__ */
