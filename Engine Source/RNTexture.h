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
		
		Texture(Format format, WrapMode wrap=WrapModeRepeat, Filter filter=FilterLinear);
		Texture(const std::string& name, Format format, WrapMode wrap=WrapModeRepeat, Filter filter=FilterLinear);
		
		virtual ~Texture();
		
		void Bind();
		void Unbind();
		
		GLuint Name() { return _name; }
		
		void SetData(const void *data, uint32 width, uint32 height, Format format);
		void UpdateData(const void *data, Format format);
		void UpdateMipmaps();
		
		void SetFormat(Format format);
		void SetWrappingMode(WrapMode wrap);
		void SetFilter(Filter filter);
		void SetGeneratesMipmaps(bool genMipmaps);
		
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
		static void ConvertFormat(Format format, GLenum *glFormat, GLenum *glType);
		
		bool _isCompleteTexture;
		bool _generateMipmaps;
		
		Format _format;
		Filter _filter;
		WrapMode _wrapMode;
	};
}

#endif /* __RAYNE_TEXTURE_H__ */
