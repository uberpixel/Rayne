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
		config.supportsVirtualFiles = true;

		__assetLoader = new PNGAssetLoader(config);

		AssetManager *coordinator = AssetManager::GetSharedInstance();
		coordinator->RegisterAssetLoader(__assetLoader);
	}

	PNGAssetLoader::PNGAssetLoader(const Config &config) :
		AssetLoader(config)
	{}

	static void png_read_from_rnfile(png_structp png_ptr, png_bytep data, png_size_t length)
    {
		if(png_ptr == NULL)
			return;

		File *file = static_cast<File*>(png_get_io_ptr(png_ptr));
		file->Read(data, length);
    }

	Asset *PNGAssetLoader::Load(File *file, const LoadOptions &options)
	{
		int transforms = PNG_TRANSFORM_SCALE_16 | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_GRAY_TO_RGB;

		png_structp pngPointer = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_set_read_fn(pngPointer, file, png_read_from_rnfile);

		png_set_sig_bytes(pngPointer, 0);
		png_set_error_fn(pngPointer, nullptr, nullptr, __PNGEmptyLogFunction);

		png_infop pngInfo = png_create_info_struct(pngPointer);
		png_read_png(pngPointer, pngInfo, transforms, nullptr);

		png_uint_32 width, height = 0;
		int depth, colorType, interlaceType = 0;

		png_get_IHDR(pngPointer, pngInfo, &width, &height, &depth, &colorType, &interlaceType, nullptr, nullptr);

		const png_bytepp rows = png_get_rows(pngPointer, pngInfo);
		
		uint8 *data = nullptr;
		size_t bytesPerRow;
		
		bool mipMapped = true;
		bool isLinear = false;
		Number *wrapper;
		
		if((wrapper = options.settings->GetObjectForKey<Number>(RNCSTR("mipMapped"))))
			mipMapped = wrapper->GetBoolValue();
		
		if ((wrapper = options.settings->GetObjectForKey<Number>(RNCSTR("isLinear"))))
			isLinear = wrapper->GetBoolValue();

		BitmapInfo::Format bitmapFormat = BitmapInfo::Format::Invalid;
		Texture::Format textureFormat = Texture::Format::Invalid;

		switch(colorType)
		{
			case PNG_COLOR_TYPE_RGB:
			{
				data = new uint8[width * height * 4];
				textureFormat = isLinear?Texture::Format::RGB_8:Texture::Format::RGB_8_SRGB;
				bitmapFormat = BitmapInfo::Format::RGBA_8;
				bytesPerRow = 4 * width;

				uint8 *temp = data;

				for(uint32 y = 0; y < height; y ++)
				{
					const png_bytep row = rows[y];

					for(uint32 x = 0; x < width; x ++)
					{
						const png_bytep ptr = &(row[x * 3]);

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
				textureFormat = isLinear?Texture::Format::RGBA_8:Texture::Format::RGBA_8_SRGB;
				bitmapFormat = BitmapInfo::Format::RGBA_8;
				bytesPerRow = 4 * width;

				uint32 *temp = reinterpret_cast<uint32 *>(data);

				for(uint32 y = 0; y < height; y ++)
				{
					const png_bytep row = rows[y];

					for(uint32 x = 0; x < width; x ++)
					{
						const png_bytep ptr = &(row[x * 4]);
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
			info.isTransposed = false;
			
			Bitmap *bitmap = new Bitmap(data, info);
			delete[] data;
			
			return bitmap->Autorelease();
		}
		else
		{
			Texture::Descriptor descriptor = Texture::Descriptor::With2DTextureAndFormat(textureFormat, width, height, mipMapped);
			
			if(WorkQueue::GetCurrentWorkQueue()->IsEqual(WorkQueue::GetMainQueue()))
			{
				Texture *texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);
				texture->SetData(0, data, bytesPerRow, height);

				if(mipMapped)
					texture->GenerateMipMaps();
				
				delete[] data;
				
				return texture->Autorelease();
			}
			
			//Force this to run on the main thread as it will otherwise cause issues with the Vulkan renderer
			auto textureFuture = WorkQueue::GetMainQueue()->PerformWithFuture([data, bytesPerRow, height, descriptor, mipMapped]() -> Texture * {
			
				Texture *texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);
				texture->SetData(0, data, bytesPerRow, height);

				if(mipMapped)
					texture->GenerateMipMaps();
				
				return texture;
			});
			
			textureFuture.wait();
			
			delete[] data;

			return textureFuture.get()->Autorelease();
		}
	}
	
	Asset *PNGAssetLoader::Load(const String *name, const LoadOptions &options)
	{
		Number *isCubemapNumber = options.settings->GetObjectForKey<Number>(RNCSTR("cubemap"));
		if(!isCubemapNumber || !isCubemapNumber->GetBoolValue())
		{
			throw InconsistencyException(RNSTR("Loading virtual png files is only supported for cubemaps!"));
		}
		
		Array *cubemapFaceTextureFileNamePostfixes = new Array();
        cubemapFaceTextureFileNamePostfixes->Autorelease();
		cubemapFaceTextureFileNamePostfixes->AddObject(RNSTR("posx"));
		cubemapFaceTextureFileNamePostfixes->AddObject(RNSTR("negx"));
		cubemapFaceTextureFileNamePostfixes->AddObject(RNSTR("posy"));
		cubemapFaceTextureFileNamePostfixes->AddObject(RNSTR("negy"));
		cubemapFaceTextureFileNamePostfixes->AddObject(RNSTR("posz"));
		cubemapFaceTextureFileNamePostfixes->AddObject(RNSTR("negz"));
		
		String *filetypeExtension = name->GetPathExtension();
		String *filenameWithoutExtension = name->StringByDeletingPathExtension();
		String *cubemapFaceTextureFileName = filenameWithoutExtension->StringByAppendingString(cubemapFaceTextureFileNamePostfixes->GetObjectAtIndex<String>(0));
		cubemapFaceTextureFileName->AppendPathExtension(filetypeExtension);
		
		File *file = File::WithName(cubemapFaceTextureFileName);
		
		int transforms = PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_GRAY_TO_RGB;
		
		png_structp pngPointer = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop pngInfo = png_create_info_struct(pngPointer);
		
		png_set_read_fn(pngPointer, file, png_read_from_rnfile);
		png_set_error_fn(pngPointer, nullptr, nullptr, __PNGEmptyLogFunction);
		
		png_set_sig_bytes(pngPointer, 0);
		png_read_info(pngPointer, pngInfo);
		
		png_uint_32 cubeWidth, cubeHeight;
		int cubeDepth, cubeColorType, cubeInterlaceType;
		
		png_get_IHDR(pngPointer, pngInfo, &cubeWidth, &cubeHeight, &cubeDepth, &cubeColorType, &cubeInterlaceType, nullptr, nullptr);
		
		png_destroy_read_struct(&pngPointer, &pngInfo, nullptr);
		
		//bool mipMapped = true;
		bool isLinear = false;
		Number *wrapper;
		
		//if((wrapper = options.settings->GetObjectForKey<Number>(RNCSTR("mipMapped"))))
			//mipMapped = wrapper->GetBoolValue();
		
		if ((wrapper = options.settings->GetObjectForKey<Number>(RNCSTR("isLinear"))))
			isLinear = wrapper->GetBoolValue();
		
		Texture::Format textureFormat = Texture::Format::Invalid;
		switch(cubeColorType)
		{
			case PNG_COLOR_TYPE_RGB:
			{
				textureFormat = isLinear?Texture::Format::RGB_8:Texture::Format::RGB_8_SRGB;
				break;
			}
				
			case PNG_COLOR_TYPE_RGBA:
			{
				textureFormat = isLinear?Texture::Format::RGBA_8:Texture::Format::RGBA_8_SRGB;
				break;
			}
				
			default:
				throw InconsistencyException(RNSTR("File " << file << " is neither RGBA nor RGB"));
		}
		
		Texture::Descriptor descriptor;
		descriptor.width = cubeWidth;
		descriptor.height = cubeHeight;
		descriptor.depth = 6;
		descriptor.format = textureFormat;
		descriptor.type = Texture::Type::TypeCube;
		
//		if(mipMapped)
//			descriptor.CalculateMipMapCount();
		
		Texture *texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);
		
		cubemapFaceTextureFileNamePostfixes->Enumerate<String>([&](String *postfix, size_t index, bool &stop){
			
			cubemapFaceTextureFileName = filenameWithoutExtension->StringByAppendingString(postfix);
			cubemapFaceTextureFileName->AppendPathExtension(filetypeExtension);
			File *file = File::WithName(cubemapFaceTextureFileName);
			
			png_structp pngPointer = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
			png_infop pngInfo = png_create_info_struct(pngPointer);
			
			png_set_read_fn(pngPointer, file, png_read_from_rnfile);
			png_set_error_fn(pngPointer, nullptr, nullptr, __PNGEmptyLogFunction);
			
			png_set_sig_bytes(pngPointer, 0);
			png_read_png(pngPointer, pngInfo, transforms, nullptr);
			
			png_uint_32 width, height;
			int depth, colorType, interlaceType;
			
			png_get_IHDR(pngPointer, pngInfo, &width, &height, &depth, &colorType, &interlaceType, nullptr, nullptr);
			
			if(width != cubeWidth || height != cubeHeight || colorType != cubeColorType)
			{
				throw InconsistencyException(RNSTR("All cubemap faces need the same height, width and format properties."));
			}
			
			png_bytepp rows = png_get_rows(pngPointer, pngInfo);
			
			
			uint8 *data = nullptr;
			size_t bytesPerRow;
			
			switch(colorType)
			{
				case PNG_COLOR_TYPE_RGB:
				{
					data = new uint8[width * height * 4];
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
			
			texture->SetData(Texture::Region::With2D(0, 0, descriptor.width, descriptor.height), 0, index, data, bytesPerRow, height);
			
			png_destroy_read_struct(&pngPointer, &pngInfo, nullptr);
			delete[] data;
			
			//if(mipMapped)
				//texture->GenerateMipMaps();
		});
		
		return texture->Autorelease();
	}
}
