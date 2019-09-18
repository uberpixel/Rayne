//
//  RNDDSAssetLoader.cpp
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

#include "RNDDSAssetLoader.h"
#include "RNAssetManager.h"
#include "RNBitmap.h"

namespace RN
{
	RNDefineMeta(DDSAssetLoader, AssetLoader)

	static DDSAssetLoader *__assetLoader;

	void DDSAssetLoader::Register()
	{
		uint8 magic[] = {0x44, 0x44, 0x53, 0x20};

		Config config({ Texture::GetMetaClass() });
		config.SetExtensions(Set::WithObjects({RNCSTR("dds")}));
		config.SetMagicBytes(Data::WithBytes(magic, 4), 0);
		config.supportsBackgroundLoading = true;

		__assetLoader = new DDSAssetLoader(config);

		AssetManager *coordinator = AssetManager::GetSharedInstance();
		coordinator->RegisterAssetLoader(__assetLoader);
	}

	DDSAssetLoader::DDSAssetLoader(const Config &config) :
		AssetLoader(config)
	{}

	Asset *DDSAssetLoader::Load(File *file, const LoadOptions &options)
	{
		file->Seek(4); // Skip over magic bytes
		
		DDS_HEADER ddsHeader;
		file->Read(&ddsHeader, sizeof(DDS_HEADER));
		
		bool hasHeaderDXT10 = false;
		DDS_HEADER_DXT10 ddsHeaderDXT10;
		if(ddsHeader.ddspf.dwFlags & DDS_PIXELFORMAT_FLAGS::DDS_PIXELFORMAT_FLAGS_FOURCC)
		{
			uint32 fourCCValue = ('D' << 0) + ('X' << 8) + ('1' << 16) + ('0' << 24);
			if(ddsHeader.ddspf.dwFourCC == fourCCValue)
			{
				hasHeaderDXT10 = true;
				file->Read(&ddsHeaderDXT10, sizeof(DDS_HEADER_DXT10));
			}
		}
		
		if(!hasHeaderDXT10)
		{
			throw InconsistencyException(RNSTR("DDS File " << file << " does not use DX10 header which is not supported."));
		}
		
		//Force textures to be linear/srgb even if file says something else
		Number *wrapper;
		if((wrapper = options.settings->GetObjectForKey<Number>(RNCSTR("isLinear"))))
		{
			if(wrapper->GetBoolValue())
			{
				switch(ddsHeaderDXT10.DDSFormat)
				{
					case DDS_FORMAT_BC1_UNORM_SRGB:
						ddsHeaderDXT10.DDSFormat = DDS_FORMAT_BC1_UNORM;
						break;
					case DDS_FORMAT_BC2_UNORM_SRGB:
						ddsHeaderDXT10.DDSFormat = DDS_FORMAT_BC2_UNORM;
						break;
					case DDS_FORMAT_BC3_UNORM_SRGB:
						ddsHeaderDXT10.DDSFormat = DDS_FORMAT_BC3_UNORM;
						break;
					case DDS_FORMAT_BC7_UNORM_SRGB:
						ddsHeaderDXT10.DDSFormat = DDS_FORMAT_BC7_UNORM;
						break;
						
					default:
						break;
				}
			}
			else
			{
				switch(ddsHeaderDXT10.DDSFormat)
				{
					case DDS_FORMAT_BC1_UNORM:
						ddsHeaderDXT10.DDSFormat = DDS_FORMAT_BC1_UNORM_SRGB;
						break;
					case DDS_FORMAT_BC2_UNORM:
						ddsHeaderDXT10.DDSFormat = DDS_FORMAT_BC2_UNORM_SRGB;
						break;
					case DDS_FORMAT_BC3_UNORM:
						ddsHeaderDXT10.DDSFormat = DDS_FORMAT_BC3_UNORM_SRGB;
						break;
					case DDS_FORMAT_BC7_UNORM:
						ddsHeaderDXT10.DDSFormat = DDS_FORMAT_BC7_UNORM_SRGB;
						break;
						
					default:
						break;
				}
			}
		}
		
		Texture::Format textureFormat = Texture::Format::Invalid;
		size_t bytesPerBlock = 16;
		if(hasHeaderDXT10)
		{
			switch(ddsHeaderDXT10.DDSFormat)
			{
				case DDS_FORMAT::DDS_FORMAT_BC1_UNORM:
					bytesPerBlock = 8;
					textureFormat = Texture::Format::RGBA_BC1;
					break;
				case DDS_FORMAT::DDS_FORMAT_BC2_UNORM:
					textureFormat = Texture::Format::RGBA_BC2;
					break;
				case DDS_FORMAT::DDS_FORMAT_BC3_UNORM:
					textureFormat = Texture::Format::RGBA_BC3;
					break;
				case DDS_FORMAT::DDS_FORMAT_BC4_UNORM:
					bytesPerBlock = 8;
					textureFormat = Texture::Format::RGBA_BC4;
					break;
				case DDS_FORMAT::DDS_FORMAT_BC5_UNORM:
					textureFormat = Texture::Format::RGBA_BC5;
					break;
				case DDS_FORMAT::DDS_FORMAT_BC7_UNORM:
					textureFormat = Texture::Format::RGBA_BC7;
					break;
					
				case DDS_FORMAT::DDS_FORMAT_BC1_UNORM_SRGB:
					bytesPerBlock = 8;
					textureFormat = Texture::Format::RGBA_BC1_SRGB;
					break;
				case DDS_FORMAT::DDS_FORMAT_BC2_UNORM_SRGB:
					textureFormat = Texture::Format::RGBA_BC2_SRGB;
					break;
				case DDS_FORMAT::DDS_FORMAT_BC3_UNORM_SRGB:
					textureFormat = Texture::Format::RGBA_BC3_SRGB;
					break;
				case DDS_FORMAT::DDS_FORMAT_BC7_UNORM_SRGB:
					textureFormat = Texture::Format::RGBA_BC7_SRGB;
					break;
					
				default:
					break;
			}
		}
		else
		{
			throw InconsistencyException(RNSTR("DDS File " << file << " uses unsupported texture format (supported is only BC1, BC2, BC3 and BC7)."));
			return nullptr;
		}
		
		uint32 mipMapCount = ddsHeader.dwMipMapCount;
		if(mipMapCount == 0) mipMapCount = 1;
		Texture::Descriptor descriptor = Texture::Descriptor::With2DTextureAndFormat(textureFormat, ddsHeader.dwWidth, ddsHeader.dwHeight, mipMapCount > 1);
		Texture *texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);
		
		uint8 *data = nullptr;
		
		uint32 mipIndex = 0;
		while(mipIndex < mipMapCount && file->GetOffset() < file->GetSize())
		{
			int32 mipWidth = std::max(ddsHeader.dwWidth >> mipIndex, static_cast<uint32>(1));
			int32 mipHeight = std::max(ddsHeader.dwHeight >> mipIndex, static_cast<uint32>(1));

			size_t mipDataSize = std::max(1, ((mipWidth + 3) / 4)) * std::max(1, ((mipHeight + 3) / 4)) * bytesPerBlock;
			
			if(!data) data = (uint8 *)malloc(mipDataSize);
			file->Read(data, mipDataSize);

			size_t mipBytesPerRow = std::max(1, ((mipWidth + 3) / 4)) * bytesPerBlock;
			texture->SetData(Texture::Region(0, 0, 0, mipWidth, mipHeight, 1), mipIndex, data, mipBytesPerRow, std::max(1, ((mipHeight + 3) / 4)));
			
			mipIndex += 1;
		}

		delete[] data;

		return texture->Autorelease();
	}
}
