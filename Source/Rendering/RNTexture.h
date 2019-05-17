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
			RGBA8888SRGB,
			BGRA8888SRGB,
			
			RGB888SRGB,
			BGR888SRGB,

			RGBA8888,
			BGRA8888,
			RGB10A2,

			R8,
			RG88,
			RGB888,

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
			Stencil8,
			Depth24Stencil8,
			Depth32FStencil8,
			
			RGBA8888SRGB_ASTC_6X6,
			RGBA8888_ASTC_6X6,

			Invalid
		};

		enum class Type
		{
			Type1D,
			Type1DArray,
			Type2D,
			Type2DMS,
			Type2DArray,
			TypeCube,
			TypeCubeArray,
			Type3D
		};

		RN_OPTIONS(UsageHint, uint32,
			ShaderRead = (1 << 0),
			ShaderWrite = (1 << 1),
			RenderTarget = (1 << 2));

		struct Descriptor
		{
			Descriptor() :
				type(Type::Type2D),
				accessOptions(GPUResource::AccessOptions::ReadWrite),
				usageHint(UsageHint::ShaderRead),
				width(1),
				height(1),
				depth(1),
				mipMaps(1),
				format(Format::RGBA8888SRGB),
				sampleCount(1),
				sampleQuality(0),
				preferredClearColor(Color::White()),
				preferredClearDepth(1.0f),
				preferredClearStencil(0)
			{

			}

			static Descriptor With2DTextureAndFormat(Format format, uint32 width, uint32 height, bool mipMapped)
			{
				Descriptor descriptor;
				descriptor.width = width;
				descriptor.height = height;
				descriptor.format = format;

				if(mipMapped)
					descriptor.CalculateMipMapCount();

				return descriptor;
			}

			static Descriptor With2DRenderTargetFormat(Format format, uint32 width, uint32 height)
			{
				Descriptor descriptor;
				descriptor.type = Type::Type2D;
				descriptor.width = width;
				descriptor.height = height;
				descriptor.format = format;
				descriptor.usageHint = UsageHint::ShaderRead | UsageHint::RenderTarget;
				descriptor.accessOptions = GPUResource::AccessOptions::Private;

				return descriptor;
			}

			static Descriptor With2DRenderTargetFormatAndMSAA(Format format, uint32 width, uint32 height, uint8 sampleCount, uint8 sampleQuality = 0)
			{
				Descriptor descriptor;
				descriptor.type = Type::Type2DMS;
				descriptor.width = width;
				descriptor.height = height;
				descriptor.format = format;
				descriptor.usageHint = UsageHint::ShaderRead | UsageHint::RenderTarget;
				descriptor.accessOptions = GPUResource::AccessOptions::Private;
				descriptor.sampleCount = sampleCount;
				descriptor.sampleQuality = sampleQuality;

				return descriptor;
			}

			void CalculateMipMapCount()
			{
				mipMaps = 1 + static_cast<uint32>(floor(log2(std::max(static_cast<double>(width), static_cast<double>(height)))));
			}

			uint32 GetWidthForMipMapLevel(uint8 mipmapLevel) const
			{
				int newWidth = width/std::pow(2, mipmapLevel);
				newWidth -= newWidth%2;
				return std::max(newWidth, 1);
			}

			uint32 GetHeightForMipMapLevel(uint8 mipmapLevel) const
			{
				int newHeight = height/std::pow(2, mipmapLevel);
				newHeight -= newHeight%2;
				return std::max(newHeight, 1);
			}

			Type type;
			GPUResource::AccessOptions accessOptions;
			UsageHint usageHint;
			uint32 width;
			uint32 height;
			uint32 depth;
			uint32 mipMaps;
			Format format;
			uint8 sampleCount; //TODO: Should be verified against the values supported by the hardware. Should be possible to querie the supported values.
			uint8 sampleQuality; //TODO: Should be verified against the values supported by the hardware. Should be possible to querie the supported values.

			Color preferredClearColor;
			float preferredClearDepth;
			uint8 preferredClearStencil;
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

		enum class ColorChannel
		{
			Red,
			Green,
			Blue,
			Alpha
		};

		RNAPI static Texture *WithName(const String *name, const Dictionary *settings = nullptr);
		RNAPI static Texture *WithDescriptor(const Descriptor &descriptor);

		RNAPI virtual void StartStreamingData(const Region &region){};
		RNAPI virtual void StopStreamingData(){};
		
		RNAPI virtual void SetData(uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) = 0;
		RNAPI virtual void SetData(const Region &region, uint32 mipmapLevel, const void *bytes, size_t bytesPerRow) = 0;
		RNAPI virtual void SetData(const Region &region, uint32 mipmapLevel, uint32 slice, const void *bytes, size_t bytesPerRow) = 0;
		RNAPI virtual void GetData(void *bytes, uint32 mipmapLevel, size_t bytesPerRow) const = 0;

		RNAPI virtual void GenerateMipMaps() = 0;
		RNAPI virtual bool HasColorChannel(ColorChannel channel) const;

		const Descriptor &GetDescriptor() const RN_NOEXCEPT { return _descriptor; }

	protected:
		RNAPI Texture(const Descriptor &descriptor);
		RNAPI ~Texture();

		Descriptor _descriptor;

		__RNDeclareMetaInternal(Texture)
	};

	RNExceptionType(InvalidTextureFormat)
}


#endif /* __RAYNE_TEXTURE_H_ */
