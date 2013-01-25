//
//  RNTexture.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_TEXTURE_H__
#define __RAYNE_TEXTURE_H__

#include "RNBase.h"
#include "RNObject.h"

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
		
		RNAPI virtual ~Texture();
		
		RNAPI void Bind();
		RNAPI void Unbind();
		
		GLuint Name() { return _name; }
		
		RNAPI void SetData(const void *data, uint32 width, uint32 height, Format format);
		RNAPI void UpdateData(const void *data, Format format);
		RNAPI void UpdateMipmaps();
		
		RNAPI void SetFormat(Format format);
		RNAPI void SetWrappingMode(WrapMode wrap);
		RNAPI void SetFilter(Filter filter);
		RNAPI void SetGeneratesMipmaps(bool genMipmaps);
		
		Format TextureFormat() const { return _format; }
		Filter TextureFilter() const { return _filter; }
		WrapMode TextureWrapMode() const { return _wrapMode; }
		bool GeneratesMipmaps() const { return _generateMipmaps; }
		
		static bool PlatformSupportsFormat(Format format);
		
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
		bool _isLinear;
	};
}

#endif /* __RAYNE_TEXTURE_H__ */
