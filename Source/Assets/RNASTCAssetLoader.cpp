//
//  RNASTCAssetLoader.cpp
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <png.h>

#include "../Objects/RNSet.h"
#include "../Objects/RNDictionary.h"
#include "../Rendering/RNRenderer.h"
#include "../Debug/RNLogger.h"

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

		Config config({ Texture::GetMetaClass() });
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
		Number *wrapper;
		if((wrapper = options.settings->GetObjectForKey<Number>(RNCSTR("isLinear"))))
			isLinear = wrapper->GetBoolValue();
		
		Texture *texture = nullptr;
		uint8 *data = nullptr;
		
		int mipIndex = 0;
		while(file->GetOffset() < file->GetSize())
		{
			ASTCFormatHeader mipHeader;
			file->Read(&mipHeader, sizeof(ASTCFormatHeader));
			
			RN::uint32 mipWidth = mipHeader.xsize[0] + (mipHeader.xsize[1] << 8) + (mipHeader.xsize[2] << 16);
			RN::uint32 mipHeight = mipHeader.ysize[0] + (mipHeader.ysize[1] << 8) + (mipHeader.ysize[2] << 16);
			
			size_t xblocks = (mipWidth + mipHeader.blockdim_x - 1) / mipHeader.blockdim_x;
			size_t yblocks = (mipHeight + mipHeader.blockdim_y - 1) / mipHeader.blockdim_y;
			size_t mipDataSize = xblocks * yblocks << 4;
			
			if(!texture)
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
				
				bool mipMapped = ((mipDataSize + sizeof(ASTCFormatHeader)) < file->GetSize());
				Texture::Descriptor descriptor = Texture::Descriptor::With2DTextureAndFormat(textureFormat, mipWidth, mipHeight, mipMapped);
				texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);
			}
			
			if(!data) data = (uint8 *)malloc(mipDataSize);
			file->Read(data, mipDataSize);

			size_t mipBytesPerRow = mipDataSize / mipHeight;
			texture->SetData(mipIndex, data, mipBytesPerRow);
			mipIndex += 1;
		}

		delete[] data;

		return texture->Autorelease();
	}
}
