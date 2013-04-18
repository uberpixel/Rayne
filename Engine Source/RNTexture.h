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
		typedef enum
		{
			FormatRGBA8888,
			FormatRGBA4444,
			FormatRGBA5551,
			FormatRGB565,
			
			FormatR8,
			FormatRG88,
			FormatRGB888,
			
			// No auto conversion for the following types!
			FormatRGBA16F,
			FormatR16F,
			FormatRG16F,
			FormatRGB16F,
			
			FormatRGBA32F,
			FormatR32F,
			FormatRG32F,
			FormatRGB32F,
			
			FormatDepth,
			FormatDepthStencil,
			
			FormatPVRTC4,
			FormatPVRTC2
		} Format;
		
		typedef enum
		{
			WrapModeClamp,
			WrapModeRepeat
		} WrapMode;
		
		typedef enum
		{
			FilterLinear,
			FilterNearest
		} Filter;
		
		RNAPI Texture(Format format, WrapMode wrap=WrapModeRepeat, Filter filter=FilterLinear, bool isLinear=false);
		RNAPI Texture(const std::string& name, Format format, WrapMode wrap=WrapModeRepeat, Filter filter=FilterLinear, bool isLinear=false);
		
		static Texture *WithFile(const std::string& name, Format format, WrapMode wrap=WrapModeRepeat, Filter filter=FilterLinear, bool isLinear=false);
		
		RNAPI virtual ~Texture();
		
		RNAPI void Bind();
		RNAPI void Unbind();
		
		GLuint Name() { return _name; }
		
		RNAPI void SetData(const void *data, uint32 width, uint32 height, Format format);
		RNAPI void UpdateData(const void *data, Format format);
		RNAPI void UpdateRegion(const void *data, Format format, Rect region);
		RNAPI void UpdateMipmaps();
		
		RNAPI void SetWrappingMode(WrapMode wrap);
		RNAPI void SetFilter(Filter filter);
		RNAPI void SetLinear(bool linear);
		RNAPI void SetGeneratesMipmaps(bool genMipmaps);
		RNAPI void SetAnisotropyLevel(uint32 level);
		
		Format TextureFormat() const { return _format; }
		Filter TextureFilter() const { return _filter; }
		WrapMode TextureWrapMode() const { return _wrapMode; }
		bool IsLinear() const { return _isLinear; }
		bool GeneratesMipmaps() const { return _generateMipmaps; }
		uint32 AnisotropyLevel() const { return _anisotropy; }
		
		RNAPI static bool PlatformSupportsFormat(Format format);
		RNAPI static uint32 MaxAnisotropyLevel();
		RNAPI static uint32 DefaultAnisotropyLevel();
		RNAPI static void SetDefaultAnisotropyLevel(uint32 level);
		
		uint32 Width() const { return _width; }
		uint32 Height() const { return _height; }
		
	protected:
		GLuint _name;
		uint32 _width, _height;
		
	private:
		static void *ConvertData(const void *data, uint32 width, uint32 height, Format current, Format target);
		static void ConvertFormat(Format format, bool isLinear, GLenum *glFormat, GLint *glInternalFormat, GLenum *glType);
		
		bool _isCompleteTexture;
		bool _generateMipmaps;
		bool _hasChanged;
		
		Format _format;
		Filter _filter;
		WrapMode _wrapMode;
		uint32 _anisotropy;
		bool _isLinear;
		
		static uint32 _defaultAnisotropy;
		
		RNDefineConstructorlessMeta(Texture, Object)
	};
}

#endif /* __RAYNE_TEXTURE_H__ */
