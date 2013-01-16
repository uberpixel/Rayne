//
//  RNTextureLoader.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
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
	
		png_structp pngPointer = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop pngInfo = png_create_info_struct(pngPointer);
		
		png_init_io(pngPointer, file);
		png_set_sig_bytes(pngPointer, 8);
		
		png_read_info(pngPointer, pngInfo);
		
		uint32 width  = (uint32)png_get_image_width(pngPointer, pngInfo);
		uint32 height = (uint32)png_get_image_height(pngPointer, pngInfo);
		
		RN_ASSERT0(width > 0 && height > 0);
		
		png_byte colorType = png_get_color_type(pngPointer, pngInfo);
		
		png_read_update_info(pngPointer, pngInfo);
		
		
		png_size_t rowBytes = png_get_rowbytes(pngPointer, pngInfo);
		png_bytep *rows = (png_bytep *)malloc(height * sizeof(png_bytep));
		
		for(uint32 i=0; i<height; i++)
			rows[i] = (png_bytep)malloc(rowBytes);
		
		png_read_image(pngPointer, rows);
		
		size_t offset = 0;
		switch(colorType)
		{
			case PNG_COLOR_TYPE_RGB:
				_format = Texture::FormatRGBA8888;
				offset = 3;
				break;
				
			case PNG_COLOR_TYPE_RGBA:
				_format = Texture::FormatRGBA8888;
				offset = 4;
				break;
				
			default:
				printf("Wrong color type!");
				break;
		}
		
		uint32 i = 0;
		uint32 *data = (uint32 *)malloc(width * height * sizeof(uint32));
		
		for(uint32 y=0; y<height; y++)
		{
			png_bytep row = rows[y];
			
			for(uint32 x=0; x<width; x++)
			{
				png_bytep ptr = &(row[x * offset]);
				
				uint32 r = ptr[0];
				uint32 g = ptr[1];
				uint32 b = ptr[2];
				uint32 a = (offset == 4) ? ptr[3] : 255;
				
				data[i ++] = (a << 24) | (b << 16) | (g << 8) | r;
			}
			
			free(rows[y]);
		}
		
		free(rows);
		
		png_destroy_info_struct(pngPointer, (png_infopp)&pngInfo);
		png_destroy_read_struct((png_structpp)&pngPointer, 0, 0);
		
		_width  = width;
		_height = height;
		_data = data;
		
		return true;
	}
}
