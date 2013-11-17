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
	
	
	
	class Texture : public Object
	{
	friend class Camera;
	public:
		class Parameter
		{
		public:
			Parameter()
			{
				format = Format::RGBA8888;
				wrapMode = WrapMode::Repeat;
				filter = Filter::Linear;
				type = Type::Texture2D;
				
				depthCompare = false;
				generateMipMaps = true;
				mipMaps = 1000;
				anisotropy = 0.0f;
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
			
			enum class Type
			{
				Texture2D,
				Texture3D,
				Texture2DArray,
				TextureCube
			};
			
			Format format;
			Filter filter;
			WrapMode wrapMode;
			Type type;
			
			bool depthCompare;
			bool generateMipMaps;
			uint32 mipMaps;
			float anisotropy;
		};
		
		RNAPI Texture(Texture::Parameter::Format format, bool isLinear=false);
		RNAPI Texture(const Texture::Parameter& parameter, bool isLinear=false);
		RNAPI Texture(const std::string& name, bool isLinear=false);
		RNAPI Texture(const std::string& name, const Texture::Parameter& parameter, bool isLinear=false);
		
		static Texture *WithFile(const std::string& name, bool isLinear=false);
		static Texture *WithFile(const std::string& name, const Texture::Parameter& parameter, bool isLinear=false);
		
		RNAPI ~Texture() override;
		
		RNAPI virtual void Bind();
		RNAPI virtual void Unbind();
		
		GLuint GetName() { return _name; }
		GLuint GetGLType() { return _glType; }
		
		RNAPI void GetData(void *ptr, Parameter::Format format);
		
		RNAPI void SetDepth(uint32 depth);
		RNAPI void SetData(const void *data, uint32 width, uint32 height, Parameter::Format format);
		RNAPI void UpdateData(const void *data, Parameter::Format format);
		RNAPI void UpdateRegion(const void *data, const Rect& region, Parameter::Format format);
		RNAPI void UpdateMipmaps();
		
		RNAPI void SetParameter(const Parameter& parameter);
		RNAPI const Parameter& GetParameter() const { return _parameter; }
		
		RNAPI static bool PlatformSupportsFormat(Texture::Parameter::Format format);
		RNAPI static void SetDefaultAnisotropyLevel(float level);
		
		RNAPI static float GetMaxAnisotropyLevel();
		RNAPI static float GetDefaultAnisotropyLevel();
		
		uint32 GetWidth() const { return _width; }
		uint32 GetHeight() const { return _height; }
		
	protected:
		GLuint _name;
		GLenum _glType;
		uint32 _width, _height, _depth;
		
	private:
		static void *ConvertData(const void *data, uint32 width, uint32 height, Texture::Parameter::Format current, Parameter::Format target);
		static void ConvertFormat(Parameter::Format format, bool isLinear, GLenum *glFormat, GLint *glInternalFormat, GLenum *glType);
		void SetType(Parameter::Type type);
		
		Parameter _parameter;
		
		bool _isCompleteTexture;
		bool _hasChanged;
		bool _isLinear;
		bool _depthCompare;
		
		static float _defaultAnisotropy;
		
		RNDefineMeta(Texture, Object)
	};
}

#endif /* __RAYNE_TEXTURE_H__ */
