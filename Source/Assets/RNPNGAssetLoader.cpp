//
//  RNPNGAssetLoader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <png.h>

#include "../Objects/RNSet.h"
#include "../Objects/RNDictionary.h"
#include "../Rendering/RNRenderer.h"

#include "RNPNGAssetLoader.h"
#include "RNAssetManager.h"
#include "RNBitmap.h"

namespace RN
{
	RNDefineMeta(PNGAssetLoader, AssetLoader)

	static PNGAssetLoader *__assetLoader;

	void __PNGEmptyLogFunction(png_structp png, const char *warning)
	{}

	void PNGAssetLoader::Register()
	{
		uint8 magic[] = {137, 80, 78, 71, 13, 10, 26, 10};

		Config config({ Texture::GetMetaClass(), Bitmap::GetMetaClass() });
		config.SetExtensions(Set::WithObjects({RNCSTR("png")}));
		config.SetMagicBytes(Data::WithBytes(magic, 8), 0);
		config.supportsBackgroundLoading = true;

		__assetLoader = new PNGAssetLoader(config);

		AssetManager *coordinator = AssetManager::GetSharedInstance();
		coordinator->RegisterAssetLoader(__assetLoader);
	}

	PNGAssetLoader::PNGAssetLoader(const Config &config) :
		AssetLoader(config)
	{}

	static void png_read_from_rnfile(png_structp png_ptr, png_bytep data, png_size_t length)
    {
		png_size_t check;

		if(png_ptr == NULL)
			return;

		File *file = static_cast<File*>(png_get_io_ptr(png_ptr));
		file->Read(data, length);
    }

	Asset *PNGAssetLoader::Load(File *file, const LoadOptions &options)
	{
		int transforms = PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_GRAY_TO_RGB;

		png_structp pngPointer = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop pngInfo = png_create_info_struct(pngPointer);

		png_set_read_fn(pngPointer, file, png_read_from_rnfile);

		png_set_sig_bytes(pngPointer, 0);
		png_set_error_fn(pngPointer, nullptr, nullptr, __PNGEmptyLogFunction);

		png_read_png(pngPointer, pngInfo, transforms, nullptr);

		png_uint_32 width, height;
		int depth, colorType, interlaceType;

		png_get_IHDR(pngPointer, pngInfo, &width, &height, &depth, &colorType, &interlaceType, nullptr, nullptr);

		png_bytepp rows = png_get_rows(pngPointer, pngInfo);


		uint8 *data = nullptr;
		size_t bytesPerRow;

		BitmapInfo::Format bitmapFormat;
		Texture::Format textureFormat;

		switch(colorType)
		{
			case PNG_COLOR_TYPE_RGB:
			{
				data = new uint8[width * height * 4];
				textureFormat = Texture::Format::RGB888SRGB;
				bitmapFormat = BitmapInfo::Format::RGBA8888;
				bytesPerRow = 4 * width;

				uint8 *temp = data;

				for(uint32 y = 0; y < height; y ++)
				{
					png_bytep row = rows[y];

					for(uint32 x = 0; x < width; x ++)
					{
						png_bytep ptr = &(row[x * 3]);

						*temp ++ = ptr[0];
						*temp ++ = ptr[1];
						*temp ++ = ptr[2];
						*temp ++ = 255;
					}
				}

				break;
			}

			case PNG_COLOR_TYPE_RGBA:
			{
				data = new uint8[width * height * 4];
				textureFormat = Texture::Format::RGBA8888SRGB;
				bitmapFormat = BitmapInfo::Format::RGBA8888;
				bytesPerRow = 4 * width;

				uint32 *temp = reinterpret_cast<uint32 *>(data);

				for(uint32 y = 0; y < height; y ++)
				{
					png_bytep row = rows[y];

					for(uint32 x = 0; x < width; x ++)
					{
						png_bytep ptr = &(row[x * 4]);
						*temp ++ = (ptr[3] << 24) | (ptr[2] << 16) | (ptr[1] << 8) | ptr[0];
					}
				}

				break;
			}
				
			default:
				throw InconsistencyException(RNSTR("File " << file << " is neither RGBA nor RGB"));
		}

		png_destroy_read_struct(&pngPointer, &pngInfo, nullptr);
		
		
		if(options.meta == Bitmap::GetMetaClass())
		{
			BitmapInfo info;
			info.bytesPerRow = bytesPerRow;
			info.width = width;
			info.height = height;
			info.format = bitmapFormat;
			
			Bitmap *bitmap = new Bitmap(data, info);
			delete[] data;
			
			return bitmap->Autorelease();
		}
		else
		{
			bool mipMapped = true;
			bool isLinear = false;
			Number *wrapper;

			if((wrapper = options.settings->GetObjectForKey<Number>(RNCSTR("mipMapped"))))
				mipMapped = wrapper->GetBoolValue();

			if ((wrapper = options.settings->GetObjectForKey<Number>(RNCSTR("isLinear"))))
				isLinear = wrapper->GetBoolValue();

			if(isLinear)
			{
				textureFormat = Texture::Format::RGBA8888;
				bitmapFormat = BitmapInfo::Format::RGBA8888;
			}

			Texture::Descriptor descriptor = Texture::Descriptor::With2DTextureAndFormat(textureFormat, width, height, mipMapped);
			Texture *texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);

			texture->SetData(0, data, bytesPerRow);

			if(mipMapped)
				texture->GenerateMipMaps();

			delete[] data;

			return texture->Autorelease();
		}
	}
}