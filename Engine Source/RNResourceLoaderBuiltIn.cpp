//
//  RNResourceLoaderBuiltIn.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <png.h>
#include "RNResourceLoaderBuiltIn.h"
#include "RNResourceCoordinator.h"
#include "RNWrappingObject.h"
#include "RNFileManager.h"
#include "RNPathManager.h"
#include "RNString.h"
#include "RNNumber.h"
#include "RNTexture.h"
#include "RNShader.h"
#include "RNUIFont.h"

namespace RN
{
	RNDeclareMeta(PNGResourceLoader)
	RNDeclareMeta(GLSLResourceLoader)
	
	// ---------------------
	// MARK: -
	// MARK: PNGResourceLoader
	// ---------------------
	
	PNGResourceLoader::PNGResourceLoader() :
		ResourceLoader(Texture2D::MetaClass())
	{
		SetFileExtensions({ "png" });
	}
	
	void PNGResourceLoader::InitialWakeUp(MetaClassBase *meta)
	{
		if(meta == MetaClass())
		{
			PNGResourceLoader *loader = new PNGResourceLoader();
			ResourceCoordinator::GetSharedInstance()->RegisterResourceLoader(loader);
			loader->Release();
		}
	}
	
	Object *PNGResourceLoader::Load(File *rfile, Dictionary *settings)
	{
		FILE *file = rfile->GetFilePointer();
		
		int transforms = PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_GRAY_TO_RGB;
		
		png_structp pngPointer = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop pngInfo      = png_create_info_struct(pngPointer);
		
		png_init_io(pngPointer, file);
		png_set_sig_bytes(pngPointer, 0);
		
		png_read_png(pngPointer, pngInfo, transforms, nullptr);
		
		uint32 width, height;
		int depth, colorType, interlaceType;
		
		png_get_IHDR(pngPointer, pngInfo, &width, &height, &depth, &colorType, &interlaceType, nullptr, nullptr);
		
		png_bytepp rows = png_get_rows(pngPointer, pngInfo);
	
		uint8 *data = nullptr;
		Texture::Format format;
		
		switch(colorType)
		{
			case PNG_COLOR_TYPE_RGB:
			{
				data   = new uint8[width * height * 3];
				format = Texture::Format::RGB888;
				
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
					}
				}
				
				break;
			}
				
			case PNG_COLOR_TYPE_RGBA:
			{
				data   = new uint8[width * height * 4];
				format = Texture::Format::RGBA8888;
				
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
		}
		
		png_destroy_read_struct(&pngPointer, &pngInfo, nullptr);
		
		// Create the texutre
		Texture::PixelData pixelData;
		Texture::Parameter parameter;
		bool isLinear = false;
		
		pixelData.data = data;
		pixelData.alignment = 1;
		pixelData.width  = width;
		pixelData.height = height;
		pixelData.format = format;
		
		parameter.format = format;
		
		if(settings->GetObjectForKey(RNCSTR("parameter")))
		{
			WrappingObject<Texture::Parameter> *wrapper = static_cast<WrappingObject<Texture::Parameter> *>(settings->GetObjectForKey(RNCSTR("parameter")));
			parameter = wrapper->GetData();
		}
		
		if(settings->GetObjectForKey(RNCSTR("linear")))
		{
			Number *wrapper = settings->GetObjectForKey<Number>(RNCSTR("linear"));
			isLinear = wrapper->GetBoolValue();
		}
		
		
		Texture2D *texture = new Texture2D(parameter, isLinear);
		texture->SetData(pixelData);
		
		delete [] data;
		
		return texture;
	}
	
	bool PNGResourceLoader::SupportsBackgroundLoading()
	{
		return true;
	}
	
	bool PNGResourceLoader::SupportsLoadingFile(File *file)
	{
		char header[8];
		file->ReadIntoBuffer(header, 8);
		
		return (png_sig_cmp(reinterpret_cast<png_const_bytep>(header), 0, 8) == 0);
	}
	
	uint32 PNGResourceLoader::GetPriority() const
	{
		return kRNResourceCoordinatorBuiltInPriority;
	}
	
	// ---------------------
	// MARK: -
	// MARK: GLSLResourceLoader
	// ---------------------
	
	GLSLResourceLoader::GLSLResourceLoader() :
		ResourceLoader(Shader::MetaClass())
	{
		SetSupportsImaginaryFiles(true);
	}
	
	void GLSLResourceLoader::InitialWakeUp(MetaClassBase *meta)
	{
		if(meta == MetaClass())
		{
			GLSLResourceLoader *loader = new GLSLResourceLoader();
			ResourceCoordinator::GetSharedInstance()->RegisterResourceLoader(loader);
			loader->Release();
		}
	}
	
	Object *GLSLResourceLoader::Load(String *name, Dictionary *settings)
	{
		Shader *shader = new Shader(name->GetUTF8String());
		return shader;
	}
	
	bool GLSLResourceLoader::SupportsLoadingName(String *name)
	{
		try
		{
			std::string path = name->GetUTF8String();
			
			FileManager::GetSharedInstance()->GetFilePathWithName(path + ".vsh");
			FileManager::GetSharedInstance()->GetFilePathWithName(path + ".fsh");
			
			return true;
		}
		catch(Exception)
		{
			return false;
		}
	}
	
	bool GLSLResourceLoader::SupportsLoadingFile(File *file)
	{
		return false;
	}
	
	bool GLSLResourceLoader::SupportsBackgroundLoading()
	{
		return true;
	}
	
	uint32 GLSLResourceLoader::GetPriority() const
	{
		return kRNResourceCoordinatorBuiltInPriority;
	}
}
