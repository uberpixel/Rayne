//
//  RNTexture.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
	
	class Texture : public Object
	{
	public:
		friend class Camera;
		
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
			
			Depth24I,
			Depth32F,
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
		
		struct Parameter
		{
			Parameter()
			{
				format   = Format::RGBA8888;
				wrapMode = WrapMode::Repeat;
				filter   = Filter::Linear;
				
				depthCompare    = false;
				generateMipMaps = true;
				maxMipMaps      = 1000;
				anisotropy      = 0.0f;
			}
			
			Format format;
			Filter filter;
			WrapMode wrapMode;
			
			bool depthCompare;
			bool generateMipMaps;
			size_t maxMipMaps;
			float anisotropy;
		};
		
		struct PixelData
		{
			Format format;
			size_t alignment;
			size_t width, height;
			void *data;
		};
		
		RNAPI ~Texture() override;
		
		RNAPI static Texture *WithFile(const std::string& name, bool isLinear = false);
		RNAPI static Texture *WithFile(const std::string& name, const Parameter& parameter, bool isLinear = false);
		
		RNAPI virtual void SetSize(size_t width, size_t height);
		RNAPI void SetParameter(const Parameter& parameter);
		
		RNAPI const Parameter& GetParameter() const { return _parameter; }
		
		RNAPI GLuint GetName() { return _name; }
		RNAPI GLuint GetGLType() { return _glType; }
		
		RNAPI size_t GetWidth() const { return _width; }
		RNAPI size_t GetHeight() const { return _height; }
		
		RNAPI bool IsComplete() const { return _isComplete; }
		
		RNAPI static bool PlatformSupportsFormat(Format format);
		
		RNAPI static void SetDefaultAnisotropyLevel(float level);
		RNAPI static float GetMaxAnisotropyLevel();
		RNAPI static float GetDefaultAnisotropyLevel();
		
	protected:
		RNAPI Texture(GLenum type, bool isLinear);
		RNAPI void Bind();
		RNAPI void Update();
		
		RNAPI static void *ConvertData(const PixelData& data, Format target);
		RNAPI static void ConvertFormatToOpenGL(Format format, bool linear, GLint& glInternalFormat, GLenum& glFormat, GLenum& glType);
		
		Parameter _parameter;
		
		GLuint _name;
		GLenum _glType;
		size_t _width, _height;
		
		bool _isComplete;
		bool _hasChanged;
		bool _isLinear;
		
	private:
		static float _defaultAnisotropy;
		
		RNDefineMeta(Texture, Object)
	};
	
	class Texture2D : public Texture
	{
	public:
		RNAPI Texture2D(Format format, bool isLinear=false);
		RNAPI Texture2D(const Parameter& parameter, bool isLinear=false);
		
		RNAPI void SetSize(size_t width, size_t height) override;
		
		RNAPI void SetData(const PixelData& data);
		RNAPI void UpdateData(const PixelData& data);
		RNAPI void UpdateRegion(const PixelData& data, const Rect& region);
		
		RNAPI void GetData(PixelData& data);
		
		RNDefineMeta(Texture2D, Texture)
	};
	
	class Texture2DArray : public Texture
	{
	public:
		RNAPI Texture2DArray(Format format, bool isLinear=false);
		RNAPI Texture2DArray(const Parameter& parameter, bool isLinear=false);
		
		RNAPI void SetSize(size_t width, size_t height) override;
		RNAPI void SetSize(size_t width, size_t height, size_t layer);
		
		RNAPI void SetData(const PixelData& data, size_t index);
		RNAPI void UpdateData(const PixelData& data, size_t index);
		
	private:
		size_t _layer;
		
		RNDefineMeta(Texture2DArray, Texture)
	};
	
	class TextureCubeMap : public Texture
	{
	public:
		enum class Side : int
		{
			PositiveX,
			NegativeX,
			PositiveY,
			NegativeY,
			PostiiveZ,
			NegativeZ,
			
			All
		};
		
		RNAPI TextureCubeMap(Format format, bool isLinear=false);
		RNAPI TextureCubeMap(const Parameter& parameter, bool isLinear=false);
		
		RNAPI void SetSize(size_t width, size_t height) override;
		
		RNAPI void SetData(const PixelData& data, Side side);
		RNAPI void UpdateData(const PixelData& data, Side side);
		
		RNDefineMeta(TextureCubeMap, Texture)
	};
}

#endif /* __RAYNE_TEXTURE_H__ */
