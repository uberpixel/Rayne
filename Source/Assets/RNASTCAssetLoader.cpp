//
//  RNASTCAssetLoader.cpp
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <png.h>

#include "../Debug/RNLogger.h"
#include "../Objects/RNDictionary.h"
#include "../Objects/RNSet.h"
#include "../Rendering/RNRenderer.h"

#include "RNASTCAssetLoader.h"
#include "RNAssetManager.h"
#include "RNBitmap.h"

namespace RN
{
	RNDefineMeta(ASTCAssetLoader, AssetLoader)

	static ASTCAssetLoader *__assetLoader;

	void ASTCAssetLoader::Register()
	{
		uint8 magic[] = {0x13, 0xab, 0xa1, 0x5c};

		Config config({Texture::GetMetaClass()});
		config.SetExtensions(Set::WithObjects({RNCSTR("astc")}));
		config.SetMagicBytes(Data::WithBytes(magic, 4), 0);
		config.supportsBackgroundLoading = true;

		__assetLoader = new ASTCAssetLoader(config);

		AssetManager *coordinator = AssetManager::GetSharedInstance();
		coordinator->RegisterAssetLoader(__assetLoader);
	}

	ASTCAssetLoader::ASTCAssetLoader(const Config &config) :
		AssetLoader(config)
	{}

	Asset *ASTCAssetLoader::Load(File *file, const LoadOptions &options)
	{
		bool isLinear = false;
		Number *wrapper = options.settings->GetObjectForKey<Number>(RNCSTR("isLinear"));
		if(wrapper) isLinear = wrapper->GetBoolValue();

		struct MipLevel
		{
			Texture::Region region;
			size_t offset;
			size_t size;
			size_t bytesPerRow;
			size_t numberOfRows;
		};

		std::vector<MipLevel> mipLevels;
		Texture::Descriptor descriptor;
		size_t bufferSize = 0;

		while(file->GetOffset() < file->GetSize())
		{
			ASTCFormatHeader mipHeader;
			if(file->Read(&mipHeader, sizeof(ASTCFormatHeader)) != sizeof(ASTCFormatHeader))
				throw InconsistencyException(RNSTR("ASTC file " << file << " has a truncated header."));

			if(mipHeader.blockdim_z != 1)
				throw InconsistencyException(RNSTR("ASTC file " << file << " requires 2D blocks (block depth 1); volumetric ASTC blocks are not supported."));

			RN::uint32 mipWidth = mipHeader.xsize[0] + (mipHeader.xsize[1] << 8) + (mipHeader.xsize[2] << 16);
			RN::uint32 mipHeight = mipHeader.ysize[0] + (mipHeader.ysize[1] << 8) + (mipHeader.ysize[2] << 16);
			RN::uint32 mipDepth = mipHeader.zsize[0] + (mipHeader.zsize[1] << 8) + (mipHeader.zsize[2] << 16);

			if(mipWidth == 0 || mipHeight == 0 || mipDepth == 0 || mipHeader.blockdim_x == 0 || mipHeader.blockdim_y == 0)
				throw InconsistencyException(RNSTR("ASTC file " << file << " has invalid image or block dimensions."));

			size_t xblocks = (mipWidth + mipHeader.blockdim_x - 1) / mipHeader.blockdim_x;
			size_t yblocks = (mipHeight + mipHeader.blockdim_y - 1) / mipHeader.blockdim_y;
			size_t remainingBytes = file->GetSize() - file->GetOffset();
			if(remainingBytes / 16 / xblocks / yblocks < mipDepth)
				throw InconsistencyException(RNSTR("ASTC file " << file << " has truncated mip data."));
			size_t mipDataSize = xblocks * yblocks * mipDepth * 16;

			if(mipLevels.empty())
			{
				Texture::Format textureFormat = Texture::Format::Invalid;
				if(isLinear)
				{
					if(mipHeader.blockdim_x == 4 && mipHeader.blockdim_y == 4)
						textureFormat = Texture::Format::RGBA_ASTC_4X4;
					else if(mipHeader.blockdim_x == 5 && mipHeader.blockdim_y == 4)
						textureFormat = Texture::Format::RGBA_ASTC_5X4;
					else if(mipHeader.blockdim_x == 5 && mipHeader.blockdim_y == 5)
						textureFormat = Texture::Format::RGBA_ASTC_5X5;
					else if(mipHeader.blockdim_x == 6 && mipHeader.blockdim_y == 5)
						textureFormat = Texture::Format::RGBA_ASTC_6X5;
					else if(mipHeader.blockdim_x == 6 && mipHeader.blockdim_y == 6)
						textureFormat = Texture::Format::RGBA_ASTC_6X6;
					else if(mipHeader.blockdim_x == 8 && mipHeader.blockdim_y == 5)
						textureFormat = Texture::Format::RGBA_ASTC_8X5;
					else if(mipHeader.blockdim_x == 8 && mipHeader.blockdim_y == 6)
						textureFormat = Texture::Format::RGBA_ASTC_8X6;
					else if(mipHeader.blockdim_x == 8 && mipHeader.blockdim_y == 8)
						textureFormat = Texture::Format::RGBA_ASTC_8X8;
					else if(mipHeader.blockdim_x == 10 && mipHeader.blockdim_y == 5)
						textureFormat = Texture::Format::RGBA_ASTC_10X5;
					else if(mipHeader.blockdim_x == 10 && mipHeader.blockdim_y == 6)
						textureFormat = Texture::Format::RGBA_ASTC_10X6;
					else if(mipHeader.blockdim_x == 10 && mipHeader.blockdim_y == 8)
						textureFormat = Texture::Format::RGBA_ASTC_10X8;
					else if(mipHeader.blockdim_x == 10 && mipHeader.blockdim_y == 10)
						textureFormat = Texture::Format::RGBA_ASTC_10X10;
					else if(mipHeader.blockdim_x == 12 && mipHeader.blockdim_y == 10)
						textureFormat = Texture::Format::RGBA_ASTC_12X10;
					else if(mipHeader.blockdim_x == 12 && mipHeader.blockdim_y == 12)
						textureFormat = Texture::Format::RGBA_ASTC_12X12;
				}
				else
				{
					if(mipHeader.blockdim_x == 4 && mipHeader.blockdim_y == 4)
						textureFormat = Texture::Format::RGBA_ASTC_4X4_SRGB;
					else if(mipHeader.blockdim_x == 5 && mipHeader.blockdim_y == 4)
						textureFormat = Texture::Format::RGBA_ASTC_5X4_SRGB;
					else if(mipHeader.blockdim_x == 5 && mipHeader.blockdim_y == 5)
						textureFormat = Texture::Format::RGBA_ASTC_5X5_SRGB;
					else if(mipHeader.blockdim_x == 6 && mipHeader.blockdim_y == 5)
						textureFormat = Texture::Format::RGBA_ASTC_6X5_SRGB;
					else if(mipHeader.blockdim_x == 6 && mipHeader.blockdim_y == 6)
						textureFormat = Texture::Format::RGBA_ASTC_6X6_SRGB;
					else if(mipHeader.blockdim_x == 8 && mipHeader.blockdim_y == 5)
						textureFormat = Texture::Format::RGBA_ASTC_8X5_SRGB;
					else if(mipHeader.blockdim_x == 8 && mipHeader.blockdim_y == 6)
						textureFormat = Texture::Format::RGBA_ASTC_8X6_SRGB;
					else if(mipHeader.blockdim_x == 8 && mipHeader.blockdim_y == 8)
						textureFormat = Texture::Format::RGBA_ASTC_8X8_SRGB;
					else if(mipHeader.blockdim_x == 10 && mipHeader.blockdim_y == 5)
						textureFormat = Texture::Format::RGBA_ASTC_10X5_SRGB;
					else if(mipHeader.blockdim_x == 10 && mipHeader.blockdim_y == 6)
						textureFormat = Texture::Format::RGBA_ASTC_10X6_SRGB;
					else if(mipHeader.blockdim_x == 10 && mipHeader.blockdim_y == 8)
						textureFormat = Texture::Format::RGBA_ASTC_10X8_SRGB;
					else if(mipHeader.blockdim_x == 10 && mipHeader.blockdim_y == 10)
						textureFormat = Texture::Format::RGBA_ASTC_10X10_SRGB;
					else if(mipHeader.blockdim_x == 12 && mipHeader.blockdim_y == 10)
						textureFormat = Texture::Format::RGBA_ASTC_12X10_SRGB;
					else if(mipHeader.blockdim_x == 12 && mipHeader.blockdim_y == 12)
						textureFormat = Texture::Format::RGBA_ASTC_12X12_SRGB;
				}

				descriptor = Texture::Descriptor::With2DTextureAndFormat(textureFormat, mipWidth, mipHeight, false);
				descriptor.type = mipDepth > 1 ? Texture::Type::Type3D : Texture::Type::Type2D;
				descriptor.depth = mipDepth;
			}

			mipLevels.push_back({Texture::Region(0, 0, 0, mipWidth, mipHeight, mipDepth), file->GetOffset(), mipDataSize, xblocks * 16, yblocks});
			bufferSize = std::max(bufferSize, mipDataSize);
			file->Seek(mipLevels.back().offset + mipDataSize);
		}

		if(mipLevels.empty()) return nullptr;
		descriptor.mipMaps = static_cast<uint32>(mipLevels.size());
		Texture *texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);
		ScopeGuard textureGuard([&]() {
			SafeRelease(texture);
		});
		std::unique_ptr<uint8[]> data(new uint8[bufferSize]);
		for(uint32 mipIndex = 0; mipIndex < descriptor.mipMaps; mipIndex++)
		{
			const MipLevel &mip = mipLevels[mipIndex];
			file->Seek(mip.offset);
			if(file->Read(data.get(), mip.size) != mip.size)
				throw InconsistencyException(RNSTR("ASTC file " << file << " has truncated mip data."));

			texture->SetData(mip.region, mipIndex, data.get(), mip.bytesPerRow, mip.numberOfRows);
		}

		textureGuard.Commit();
		return texture->Autorelease();
	}
} // namespace RN
