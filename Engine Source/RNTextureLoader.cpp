//
//  RNTextureLoader.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <png.h>
#include "RNTextureLoader.h"

namespace RN
{
	TextureLoader::TextureLoader(const std::string& name)
	{
		_width = _height = 0;
		_data = 0;
		
		File *file = new File(name);
		FILE *rfile = file->FilePointer();
		bool result = false;
		
		result = LoadPNGTexture(rfile);
		if(result)
			goto TextureLoaded;
		
		fseek(rfile, 0, SEEK_SET);
		
	TextureLoaded:
		file->Release();
	}
	
	TextureLoader::~TextureLoader()
	{
		if(_data)
			free(_data);
	}
	
	
	
	bool TextureLoader::LoadPNGTexture(FILE *file)
	{
		char header[8];
		fread(header, 1, 8, file);
		
		if(png_sig_cmp((png_const_bytep)header, 0, 8))
			return false;
		
		int transforms = PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_GRAY_TO_RGB;
	
		png_structp pngPointer = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop pngInfo = png_create_info_struct(pngPointer);
		
		png_init_io(pngPointer, file);
		png_set_sig_bytes(pngPointer, 8);
		
		png_read_png(pngPointer, pngInfo, transforms, 0);
		
		
		uint32 width, height;
		int depth, colorType, interlaceType;
		
		png_get_IHDR(pngPointer, pngInfo, &width, &height, &depth, &colorType, &interlaceType, 0, 0);
		
		RN_ASSERT0(width > 0 && height > 0);
		png_bytepp rows = png_get_rows(pngPointer, pngInfo);
		
		switch(colorType)
		{
			case PNG_COLOR_TYPE_RGB:
			{
				_format = TextureParameter::Format::RGB888;
				uint8 *data = (uint8 *)malloc(width * height * 3 * sizeof(uint32));
				uint8 *temp = data;
				
				for(uint32 y=0; y<height; y++)
				{
					png_bytep row = rows[y];
					
					for(uint32 x=0; x<width; x++)
					{
						png_bytep ptr = &(row[x * 3]);
						
						*temp ++ = ptr[0];
						*temp ++ = ptr[1];
						*temp ++ = ptr[2];
					}
				}
				
				_data = data;
				break;
			}
				
			case PNG_COLOR_TYPE_RGBA:
			{
				_format = TextureParameter::Format::RGBA8888;
				uint32 *data = (uint32 *)malloc(width * height * 4 * sizeof(uint32));
				uint32 *temp = data;
				
				for(uint32 y=0; y<height; y++)
				{
					png_bytep row = rows[y];
					
					for(uint32 x=0; x<width; x++)
					{
						png_bytep ptr = &(row[x * 4]);
						
						*temp ++ = (ptr[3] << 24) | (ptr[2] << 16) | (ptr[1] << 8) | ptr[0];
					}
				}
				
				_data = data;
				break;
			}
			
			default:
				printf("Wrong color type!");
				break;
		}
		
		png_destroy_read_struct((png_structpp)&pngPointer, (png_infopp)&pngInfo, 0);
		
		_width  = width;
		_height = height;
		
		return true;
	}
}
