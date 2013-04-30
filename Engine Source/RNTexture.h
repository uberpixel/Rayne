//
//  RNTexture.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TEXTURE_H__
#define __RAYNE_TEXTURE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNRect.h"

namespace RN
{
	class Camera;
	
	class TextureParameter
	{
	public:
		TextureParameter()
		{
			format = Format::RGBA8888;
			wrapMode = WrapMode::Repeat;
			filter = Filter::Linear;
			type = Type::Texture2D;
			
			depthCompare = false;
			generateMipMaps = true;
			mipMaps = 1000;
			anisotropy = 0;
		}
		
		enum class Format
		{
			RGBA8888,
			RGBA4444,
			RGBA5551,
			RGB565,
			
			R8,
			RG88,
			RGB888,
			
			// No auto conversion for the following types!
			R16F,
			RG16F,
			RGB16F,
			RGBA16F,
			
			R32F,
			RG32F,
			RGB32F,
			RGBA32F,
			
			Depth,
			DepthStencil,
			
			PVRTC4,
			PVRTC2
		};
		
		enum class WrapMode
		{
			Clamp,
			Repeat
		};
		
		enum class Filter
		{
			Linear,
			Nearest
		};

		enum class Type
		{
			Texture2D,
			Texture3D,
			Texture2DArray
		};
		
		Format format;
		Filter filter;
		WrapMode wrapMode;
		Type type;
		
		bool depthCompare;
		bool generateMipMaps;
		uint32 mipMaps;
		uint32 anisotropy;
	};
	
	class Texture : public Object
	{
	friend class Camera;
	public:
		RNAPI Texture(TextureParameter::Format format, bool isLinear=false);
		RNAPI Texture(const TextureParameter& parameter, bool isLinear=false);
		RNAPI Texture(const std::string& name, bool isLinear=false);
		RNAPI Texture(const std::string& name, const TextureParameter& parameter, bool isLinear=false);
		
		static Texture *WithFile(const std::string& name, bool isLinear=false);
		static Texture *WithFile(const std::string& name, const TextureParameter& parameter, bool isLinear=false);
		
		RNAPI virtual ~Texture();
		
		RNAPI virtual void Bind();
		RNAPI virtual void Unbind();
		
		GLuint Name() { return _name; }
		GLuint GLType() { return _glType; }
		
		RNAPI void SetDepth(uint32 depth);
		RNAPI void SetData(const void *data, uint32 width, uint32 height, TextureParameter::Format format);
		RNAPI void UpdateData(const void *data, TextureParameter::Format format);
		RNAPI void UpdateRegion(const void *data, const Rect& region, TextureParameter::Format format);
		RNAPI void UpdateMipmaps();
		
		RNAPI void SetParameter(const TextureParameter& parameter);
		RNAPI const TextureParameter& Parameter() const { return _parameter; }
		
		RNAPI static bool PlatformSupportsFormat(TextureParameter::Format format);
		RNAPI static void SetDefaultAnisotropyLevel(uint32 level);
		
		RNAPI static uint32 MaxAnisotropyLevel();
		RNAPI static uint32 DefaultAnisotropyLevel();
		
		uint32 Width() const { return _width; }
		uint32 Height() const { return _height; }
		
	protected:
		GLuint _name;
		GLenum _glType;
		uint32 _width, _height, _depth;
		
	private:
		static void *ConvertData(const void *data, uint32 width, uint32 height, TextureParameter::Format current, TextureParameter::Format target);
		static void ConvertFormat(TextureParameter::Format format, bool isLinear, GLenum *glFormat, GLint *glInternalFormat, GLenum *glType);
		void SetType(TextureParameter::Type type);
		
		TextureParameter _parameter;
		
		bool _isCompleteTexture;
		bool _hasChanged;
		bool _isLinear;
		bool _depthCompare;
		
		static uint32 _defaultAnisotropy;
		
		RNDefineConstructorlessMeta(Texture, Object)
	};
}

#endif /* __RAYNE_TEXTURE_H__ */
