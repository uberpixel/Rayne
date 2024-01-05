//
//  RNTexture.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Assets/RNAssetManager.h"
#include "RNTexture.h"
#include "RNRenderer.h"

#include <png.h>

namespace RN
{
	RNDefineMeta(Texture, Asset)

	RNExceptionImp(InvalidTextureFormat)

	Texture::Texture(const Descriptor &descriptor) :
		_descriptor(descriptor)
	{}

	Texture::~Texture()
	{}

	Texture *Texture::WithName(const String *name, const Dictionary *settings)
	{
		AssetManager *coordinator = AssetManager::GetSharedInstance();

		String *textureName = RNSTR(name);
		if(name->HasSuffix(RNCSTR(".*")))
		{
			textureName = textureName->GetSubstring(Range(0, textureName->GetLength()-1));
			textureName->Append(AssetManager::GetSharedInstance()->GetPreferredTextureFileExtension());
		}

		return coordinator->GetAssetWithName<Texture>(textureName, settings);
	}

	struct PngReadFromData
	{
		const Data *data;
		size_t offset;
	};

	void __TexturePNGEmptyLogFunction(png_structp png, const char *warning)
	{}

	static void __TexturePNGReadFromData(png_structp png_ptr, png_bytep data, png_size_t length)
	{
		if(png_ptr == NULL)
			return;

		PngReadFromData *readData = static_cast<PngReadFromData*>(png_get_io_ptr(png_ptr));
		
		try
		{
			readData->data->GetBytesInRange(data, Range(readData->offset, length));
		}
		catch(RangeException exception)
		{
			png_error(png_ptr, exception.GetReason().c_str());
		}
		png_error(png_ptr, "yey");
		readData->offset += length;
	}

	Texture *Texture::WithPNGData(const Data *data, const Dictionary *settings)
	{
		//TODO: Ideally this should use the AssetManager, but extending it to support Data and not just File is a lot of work.
		
		int transforms = PNG_TRANSFORM_SCALE_16 | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_GRAY_TO_RGB;

		png_structp pngPointer = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		
		//TODO: Read 8 magic bytes first and check if correct
		//uint8 magic[] = {137, 80, 78, 71, 13, 10, 26, 10};
		
		PngReadFromData readData;
		readData.data = data;
		readData.offset = 0;
		png_set_read_fn(pngPointer, &readData, __TexturePNGReadFromData);

		png_set_sig_bytes(pngPointer, 0);
		png_set_error_fn(pngPointer, nullptr, nullptr, __TexturePNGEmptyLogFunction);

		png_infop pngInfo = png_create_info_struct(pngPointer);
		
		if(setjmp(png_jmpbuf(pngPointer)))
		{
			png_destroy_read_struct(&pngPointer, &pngInfo, nullptr);
			throw InconsistencyException(RNSTR("PNG data could not be decoded."));
		}
		
		png_read_png(pngPointer, pngInfo, transforms, nullptr);

		png_uint_32 width, height = 0;
		int depth, colorType, interlaceType = 0;

		png_get_IHDR(pngPointer, pngInfo, &width, &height, &depth, &colorType, &interlaceType, nullptr, nullptr);

		const png_bytepp rows = png_get_rows(pngPointer, pngInfo);
		
		uint8 *imageData = nullptr;
		size_t bytesPerRow;
		
		bool mipMapped = true;
		bool isLinear = false;
		Number *wrapper;
		
		if(settings && (wrapper = settings->GetObjectForKey<Number>(RNCSTR("mipMapped"))))
			mipMapped = wrapper->GetBoolValue();
		
		if(settings && (wrapper = settings->GetObjectForKey<Number>(RNCSTR("isLinear"))))
			isLinear = wrapper->GetBoolValue();

		Texture::Format textureFormat = Texture::Format::Invalid;

		switch(colorType)
		{
			case PNG_COLOR_TYPE_RGB:
			{
				imageData = new uint8[width * height * 4];
				textureFormat = isLinear?Texture::Format::RGB_8:Texture::Format::RGB_8_SRGB;
				bytesPerRow = 4 * width;

				uint8 *temp = imageData;

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
				imageData = new uint8[width * height * 4];
				textureFormat = isLinear?Texture::Format::RGBA_8:Texture::Format::RGBA_8_SRGB;
				bytesPerRow = 4 * width;
				
				uint32 *temp = reinterpret_cast<uint32 *>(imageData);

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
				throw InconsistencyException(RNSTR("PNG Data is neither RGBA nor RGB"));
		}

		png_destroy_read_struct(&pngPointer, &pngInfo, nullptr);
		
		Texture::Descriptor descriptor = Texture::Descriptor::With2DTextureAndFormat(textureFormat, width, height, mipMapped);
		Texture *texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);

		texture->SetData(0, imageData, bytesPerRow, height);

		if(mipMapped)
			texture->GenerateMipMaps();

		delete[] imageData;

		return texture->Autorelease();
	}

	Texture *Texture::WithDescriptor(const Descriptor &descriptor)
	{
		Texture *texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);
		return texture->Autorelease();
	}
	
	bool Texture::HasColorChannel(ColorChannel channel) const
	{
		#define ColorChannel(format, r, g, b, a) \
		case format: \
		{ \
			switch(channel) \
			{ \
				case ColorChannel::Red: \
					return r; \
				case ColorChannel::Green: \
					return g; \
				case ColorChannel::Blue: \
					return b; \
				case ColorChannel::Alpha: \
					return a; \
			} \
		return false; \
		}
		
		switch(_descriptor.format)
		{
			ColorChannel(Format::RGBA_8_SRGB, true, true, true, true)
			ColorChannel(Format::BGRA_8_SRGB, true, true, true, true)
				
			ColorChannel(Format::RGB_8_SRGB, true, true, true, false)
			ColorChannel(Format::BGR_8_SRGB, true, true, true, false)
				
			ColorChannel(Format::RGBA_8, true, true, true, true)
			ColorChannel(Format::BGRA_8, true, true, true, true)
			ColorChannel(Format::RGB_10_A_2, true, true, true, true)
			ColorChannel(Format::BGR_10_A_2, true, true, true, true)
				
			ColorChannel(Format::R_8, true, false, false, false)
			ColorChannel(Format::RG_8, true, true, false, false)
			ColorChannel(Format::RGB_8, true, true, true, false)
				
			ColorChannel(Format::R_16F, true, false, false, false)
			ColorChannel(Format::RG_16F, true, true, false, false)
			ColorChannel(Format::RGB_16F, true, true, true, false)
			ColorChannel(Format::RGBA_16F, true, true, true, true)
				
			ColorChannel(Format::R_32F, true, false, false, false)
			ColorChannel(Format::RG_32F, true, true, false, false)
			ColorChannel(Format::RGB_32F, true, true, true, false)
			ColorChannel(Format::RGBA_32F, true, true, true, true)
				
			default:
				return false;
		}
	}
}
