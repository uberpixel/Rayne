//
//  RNTexture.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_TEXTURE_H_
#define __RAYNE_TEXTURE_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Assets/RNAsset.h"
#include "RNGPUResource.h"

namespace RN
{
	class Texture : public Asset
	{
	public:
		enum class Format
		{
			Invalid,

			RGBA8888,
			RGB10A2,

			R8,
			RG88,

			R16F,
			RG16F,
			RGBA16F,

			R32F,
			RG32F,
			RGBA32F,

			Depth24I,
			Depth32F,
			Stencil8,
			Depth24Stencil8,
			Depth32FStencil8
		};

		struct Descriptor
		{
			enum class Type
			{
				Type1D,
				Type1DArray,
				Type2D,
				Type2DArray,
				TypeCube,
				TypeCubeArray,
				Type3D
			};

			RN_OPTIONS(UsageHint, uint32,
					   ShaderRead = (1 << 0),
					   ShaderWrite = (1 << 1),
					   RenderTarget = (1 << 2));

			Descriptor() :
				type(Type::Type2D),
				usageOptions(GPUResource::UsageOptions::ReadWrite),
				usageHint(UsageHint::ShaderRead),
				width(1),
				height(1),
				depth(1),
				mipMaps(1),
				_format(nullptr)
			{
				SetFormat(__TranslateFormat(Format::RGBA8888));
			}
			~Descriptor()
			{
				SafeRelease(_format);
			}

			static Descriptor With2DTextureAndFormat(Format format, uint32 width, uint32 height, bool mipMapped)
			{
				Descriptor descriptor;
				descriptor.width = width;
				descriptor.height = height;
				descriptor.SetFormat(__TranslateFormat(format));

				if(mipMapped)
					descriptor.CalculateMipMapCount();

				return descriptor;
			}

			void CalculateMipMapCount()
			{
				mipMaps = 1 + static_cast<uint32>(floor(log2(std::max(static_cast<double>(width), static_cast<double>(height)))));
			}

			void SetFormat(Format format)
			{
				SetFormat(__TranslateFormat(format));
			}

			void SetFormat(const String *format)
			{
				SafeRelease(_format);
				_format = SafeCopy(format);
			}

			const String *GetFormat() const { return _format; }

			Type type;
			GPUResource::UsageOptions usageOptions;
			UsageHint usageHint;
			uint32 width;
			uint32 height;
			uint32 depth;
			uint32 mipMaps;

		private:
			static const String *__TranslateFormat(Format format);

			String *_format;
		};

		struct Region
		{
			Region(uint32 tx, uint32 ty, uint32 tz, uint32 twidth, uint32 theight, uint32 tdepth) :
				x(tx),
				y(ty),
				z(tz),
				width(twidth),
				height(theight),
				depth(tdepth)
			{}

			static Region With1D(uint32 x, uint32 w)
			{
				return Region(x, 0, 0, w, 1, 1);
			}

			static Region With2D(uint32 x, uint32 y, uint32 w, uint32 h)
			{
				return Region(x, y, 0, w, h, 1);
			}

			static Region With3D(uint32 x, uint32 y, uint32 z, uint32 w, uint32 h, uint32 d)
			{
				return Region(x, y, z, w, h, d);
			}

			struct
			{
				uint32 x;
				uint32 y;
				uint32 z;

				uint32 width;
				uint32 height;
				uint32 depth;
			};
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
			Parameter() :
				wrapMode(WrapMode::Repeat),
				filter(Filter::Linear),
				depthCompare(false),
				anisotropy(1)
			{}

			bool operator== (const Parameter &other) const
			{
				return (filter == other.filter && wrapMode == other.wrapMode && depthCompare == other.depthCompare && anisotropy == other.anisotropy);
			}

			Filter filter;
			WrapMode wrapMode;

			bool depthCompare;
			uint32 anisotropy;
		};

		RNAPI static Texture *WithName(const String *name);

		RNAPI virtual void SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) = 0;
		RNAPI virtual void SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) = 0;
		RNAPI virtual void SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow) = 0;

		RNAPI virtual void SetParameter(const Parameter &parameter);

		RNAPI virtual void GenerateMipMaps() = 0;

		const Descriptor &GetDescriptor() const RN_NOEXCEPT { return _descriptor; }
		const Parameter &GetParameter() const RN_NOEXCEPT { return _parameter; }

	protected:
		Texture(const Descriptor &descriptor);

		Descriptor _descriptor;
		Parameter _parameter;

		RNDeclareMeta(Texture)
	};

	RNExceptionType(InvalidTextureFormat)
}


#endif /* __RAYNE_TEXTURE_H_ */
