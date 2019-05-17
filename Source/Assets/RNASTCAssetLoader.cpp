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
		
		ASTCFormatHeader header;
		file->Read(&header, sizeof(ASTCFormatHeader));
		file->Seek(0);
		
		Texture::Format textureFormat = Texture::Format::RGBA8888SRGB;//Texture::Format::RGBA8888SRGB_ASTC_6X6;
		if(isLinear)
		{
			textureFormat = Texture::Format::RGBA8888_ASTC_6X6;
		}
		
		RN::uint32 width = header.xsize[0] + (header.xsize[1] << 8) + (header.xsize[2] << 16);
		RN::uint32 height = header.ysize[0] + (header.ysize[1] << 8) + (header.ysize[2] << 16);
		size_t xblocks = (width + header.blockdim_x - 1) / header.blockdim_x;
		size_t yblocks = (height + header.blockdim_y - 1) / header.blockdim_y;
		size_t dataSize = xblocks * yblocks << 4;
		
		bool mipMapped = ((dataSize + sizeof(ASTCFormatHeader)) < file->GetSize());
		Texture::Descriptor descriptor = Texture::Descriptor::With2DTextureAndFormat(textureFormat, width, height, mipMapped);
		Texture *texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);
		
		uint8 *data = nullptr;
		
		int mipIndex = 0;
		while(file->GetOffset() < file->GetSize())
		{
			ASTCFormatHeader header;
			file->Read(&header, sizeof(ASTCFormatHeader));
			
			RN::uint32 mipWidth = header.xsize[0] + (header.xsize[1] << 8) + (header.xsize[2] << 16);
			RN::uint32 mipHeight = header.ysize[0] + (header.ysize[1] << 8) + (header.ysize[2] << 16);
			
			RNDebug("Width: " << mipWidth << ", height: " << mipHeight);
			
			size_t xblocks = (mipWidth + header.blockdim_x - 1) / header.blockdim_x;
			size_t yblocks = (mipHeight + header.blockdim_y - 1) / header.blockdim_y;
			size_t mipDataSize = xblocks * yblocks << 4;
			
			if(!data) data = (uint8 *)malloc(mipDataSize);
			file->Read(data, mipDataSize);

			//size_t mipBytesPerRow = mipWidth / header.blockdim_x * 16;
			//texture->SetData(mipIndex, data, mipBytesPerRow);
			mipIndex += 1;
		}

		delete[] data;

		return texture->Autorelease();
	}
}
