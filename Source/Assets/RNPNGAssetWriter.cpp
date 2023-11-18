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
#include "../Assets/RNBitmap.h"
#include "../Debug/RNLogger.h"
#include "../Threads/RNWorkQueue.h"
#include "../Threads/RNWorkGroup.h"
#include "../Objects/RNAutoreleasePool.h"

#include "RNPNGAssetWriter.h"

namespace RN
{
	static void png_write_to_rndata(png_structp png_ptr, png_bytep data, png_size_t length)
	{
		if(png_ptr == NULL)
			return;

		RN::Data *rndata = static_cast<RN::Data*>(png_get_io_ptr(png_ptr));
		rndata->Append(data, length);
	}

	void PNGAssetWriter::Encode(Texture *texture, std::function<void(RN::Data *pngData)> callback)
	{
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
				RNError("Unsupported texture format");
				callback(nullptr);
				return;
			}
		}

		Data *textureData = Data::WithBytes(nullptr, texture->GetDescriptor().width * texture->GetDescriptor().height * channelCount);
		textureData->Retain();
		texture->Retain();
		texture->GetData(textureData->GetBytes(), 0, texture->GetDescriptor().width * channelCount, [textureData, colorType, bitDepth, channelCount, texture, callback](){

			WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Background);
			WorkGroup *group = new WorkGroup();
			group->Perform(queue, [textureData, colorType, bitDepth, channelCount, texture, callback] {

				AutoreleasePool pool;
				
				png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
				if(!png_ptr)
				{
					textureData->Release();
					texture->Release();
					callback(nullptr);
					return;
				}

				png_infop info_ptr = png_create_info_struct(png_ptr);
				if(!info_ptr)
				{
					png_destroy_write_struct(&png_ptr, NULL);
					textureData->Release();
					texture->Release();
					callback(nullptr);
					return;
				}
				
				if(setjmp(png_jmpbuf(png_ptr)))
				{
					png_destroy_write_struct(&png_ptr, &info_ptr);
					textureData->Release();
					texture->Release();
					callback(nullptr);
					return;
				}
				
				RN::Data *resultData = new RN::Data();
				png_set_write_fn(png_ptr, resultData, png_write_to_rndata, nullptr);

				png_set_IHDR(png_ptr, info_ptr, texture->GetDescriptor().width, texture->GetDescriptor().height, bitDepth, colorType, PNG_INTERLACE_ADAM7, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

				//png_set_gAMA(png_ptr, info_ptr, mainprog_ptr->gamma);

				png_write_info(png_ptr, info_ptr);
				png_set_packing(png_ptr); //Not really needed apparently, as it only does stuff for greyscale and palettised images

				RN::uint8 **rowPointers = new RN::uint8*[texture->GetDescriptor().height];
				for(size_t index = 0; index < texture->GetDescriptor().height; index++)
				{
					rowPointers[index] = &textureData->GetBytes<RN::uint8>()[index * texture->GetDescriptor().width * channelCount];
				}
				
				png_write_image(png_ptr, rowPointers);

				png_write_end(png_ptr, NULL);
				
				callback(resultData);
				
				delete[] rowPointers;
				textureData->Release();
				texture->Release();
				resultData->Release();
			});
		});
		
		return;
	}

	RN::Data *PNGAssetWriter::Encode(Bitmap *bitmap)
	{
		int colorType = 0;
		int bitDepth = 0;
		int channelCount = 0;
		switch(bitmap->GetInfo().format)
		{
			case BitmapInfo::Format::RGB_8: //This is also layout with 4 channels
			case BitmapInfo::Format::RGBA_8:
			{
				colorType = PNG_COLOR_TYPE_RGB_ALPHA;
				bitDepth = 8;
				channelCount = 4;
				break;
			}

				//PNG_COLOR_TYPE_GRAY

			default:
			{
				RNError("Unsupported texture format");
				return nullptr;
			}
		}
		
		png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if(!png_ptr)
		{
			return nullptr;
		}

		png_infop info_ptr = png_create_info_struct(png_ptr);
		if(!info_ptr)
		{
			png_destroy_write_struct(&png_ptr, NULL);
			return nullptr;
		}
		
		if(setjmp(png_jmpbuf(png_ptr)))
		{
			png_destroy_write_struct(&png_ptr, &info_ptr);
			return nullptr;
		}
		
		RN::Data *resultData = new RN::Data();
		png_set_write_fn(png_ptr, resultData, png_write_to_rndata, nullptr);

		png_set_IHDR(png_ptr, info_ptr, bitmap->GetInfo().width, bitmap->GetInfo().height, bitDepth, colorType, PNG_INTERLACE_ADAM7, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

		//png_set_gAMA(png_ptr, info_ptr, mainprog_ptr->gamma);

		png_write_info(png_ptr, info_ptr);
		png_set_packing(png_ptr); //Not really needed apparently, as it only does stuff for greyscale and palettised images

		RN::uint8 **rowPointers = new RN::uint8*[bitmap->GetInfo().height];
		for(size_t index = 0; index < bitmap->GetInfo().height; index++)
		{
			rowPointers[index] = &bitmap->GetData()->GetBytes<RN::uint8>()[index * bitmap->GetInfo().width * channelCount];
		}
		
		png_write_image(png_ptr, rowPointers);

		png_write_end(png_ptr, NULL);
		
		delete[] rowPointers;
		
		return resultData->Autorelease();
	}

	bool PNGAssetWriter::Write(Texture *texture, const String *filename)
	{
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
				return false;
			}
		}

		Data *textureData = Data::WithBytes(nullptr, texture->GetDescriptor().width * texture->GetDescriptor().height * channelCount);
		textureData->Retain();
		texture->Retain();
		filename->Retain();
		texture->GetData(textureData->GetBytes(), 0, texture->GetDescriptor().width * channelCount, [textureData, colorType, bitDepth, channelCount, texture, filename](){

			WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Background);
			WorkGroup *group = new WorkGroup();
			group->Perform(queue, [textureData, colorType, bitDepth, channelCount, texture, filename] {

				AutoreleasePool pool;
				
				png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
				if(!png_ptr)
				{
					textureData->Release();
					filename->Release();
					texture->Release();
					return;
				}

				png_infop info_ptr = png_create_info_struct(png_ptr);
				if(!info_ptr)
				{
					png_destroy_write_struct(&png_ptr, NULL);
					textureData->Release();
					filename->Release();
					texture->Release();
					return;
				}

				FILE *fp = fopen(filename->GetUTF8String(), "wb");
				if(!fp)
				{
					png_destroy_write_struct(&png_ptr, &info_ptr);
					textureData->Release();
					filename->Release();
					texture->Release();
					return;
				}
				
				if(setjmp(png_jmpbuf(png_ptr)))
				{
					//If we get here, we had a problem writing the file
					fclose(fp);
					png_destroy_write_struct(&png_ptr, &info_ptr);
					textureData->Release();
					filename->Release();
					texture->Release();
					return;
				}
				
				png_init_io(png_ptr, fp);

				png_set_IHDR(png_ptr, info_ptr, texture->GetDescriptor().width, texture->GetDescriptor().height, bitDepth, colorType, PNG_INTERLACE_ADAM7, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

				//png_set_gAMA(png_ptr, info_ptr, mainprog_ptr->gamma);

				png_write_info(png_ptr, info_ptr);
				png_set_packing(png_ptr); //Not really needed apparently, as it only does stuff for greyscale and palettised images

				RN::uint8 **rowPointers = new RN::uint8*[texture->GetDescriptor().height];
				for(size_t index = 0; index < texture->GetDescriptor().height; index++)
				{
					rowPointers[index] = &textureData->GetBytes<RN::uint8>()[index * texture->GetDescriptor().width * channelCount];
				}
				
				png_write_image(png_ptr, rowPointers);

				png_write_end(png_ptr, NULL);
				
				fclose(fp);
				
				delete[] rowPointers;
				textureData->Release();
				filename->Release();
				texture->Release();
			});
		});
		
		return true;
	}
}
