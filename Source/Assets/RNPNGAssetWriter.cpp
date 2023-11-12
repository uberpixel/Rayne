//
//  RNPNGAssetWriter.cpp
//  Rayne
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <png.h>

#include "../Rendering/RNRenderer.h"
#include "../Objects/RNData.h"

#include "RNPNGAssetWriter.h"

namespace RN
{
	bool PNGAssetWriter::Write(Texture *texture, const String *filename)
	{
		png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if(!png_ptr) return false;

		png_infop info_ptr = png_create_info_struct(png_ptr);
		if(!info_ptr)
		{
			png_destroy_write_struct(&png_ptr, NULL);
			return false;
		}
		
		FILE *fp = fopen(filename->GetUTF8String(), "wb");
		if(!fp)
		{
			png_destroy_write_struct(&png_ptr, NULL);
			return false;
		}
		png_init_io(png_ptr, fp);
		
		int colorType = 0;
		int bitDepth = 0;
		int channelCount = 0;
		switch(texture->GetDescriptor().format)
		{
			case Texture::Format::RGB_8_SRGB: //This is also layout with 4 channels
			case Texture::Format::RGBA_8_SRGB:
			{
				colorType = PNG_COLOR_TYPE_RGB_ALPHA;
				bitDepth = 8;
				channelCount = 4;
				break;
			}
				
			//PNG_COLOR_TYPE_GRAY
				
			default:
			{
				png_destroy_write_struct(&png_ptr, NULL);
				return false;
			}
		}
		
		png_set_IHDR(png_ptr, info_ptr, texture->GetDescriptor().width, texture->GetDescriptor().height, bitDepth, colorType, PNG_INTERLACE_ADAM7, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		
		//png_set_gAMA(png_ptr, info_ptr, mainprog_ptr->gamma);
		
		png_write_info(png_ptr, info_ptr);
		png_set_packing(png_ptr); //Not really needed apparently, as it only does stuff for greyscale and palettised images
		
		Data *textureData = Data::WithBytes(nullptr, texture->GetDescriptor().width * texture->GetDescriptor().height * channelCount);
		texture->GetData(textureData->GetBytes(), 0, texture->GetDescriptor().width * channelCount);
		
		RN::uint8 **rowPointers = new RN::uint8*[texture->GetDescriptor().height];
		for(size_t index = 0; index < texture->GetDescriptor().height; index++)
		{
			rowPointers[index] = &textureData->GetBytes<RN::uint8>()[index * texture->GetDescriptor().width * channelCount];
		}
		
		png_write_image(png_ptr, rowPointers);

		png_write_end(png_ptr, NULL);
		
		return true;
	}
}
