//
//  RNResourceLoaderBuiltIn.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
#include "RNModel.h"
#include "RNSkeleton.h"

namespace RN
{
	RNDeclareMeta(PNGResourceLoader)
	RNDeclareMeta(GLSLResourceLoader)
	RNDeclareMeta(SGMResourceLoader)
	RNDeclareMeta(SGAResourceLoader)
	
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
	// MARK: SGMResourceLoader
	// ---------------------
	
	SGMResourceLoader::SGMResourceLoader() :
		ResourceLoader(Model::MetaClass())
	{
		uint8 magicBytes[] = { 0x90, 0x22, 0x5, 0x15 };
		
		Data *data = new Data(magicBytes, 4);
		
		SetFileExtensions({ "sgm" });
		SetMagicBytes(data, 0);
		
		data->Release();
	}
	
	void SGMResourceLoader::InitialWakeUp(MetaClassBase *meta)
	{
		if(meta == MetaClass())
		{
			SGMResourceLoader *loader = new SGMResourceLoader();
			ResourceCoordinator::GetSharedInstance()->RegisterResourceLoader(loader);
			loader->Release();
		}
	}
	
	Object *SGMResourceLoader::Load(File *file, Dictionary *settings)
	{
		Model *model = new Model();
		
		bool guessMaterial = true;
		float distance[] = { 0.05f, 0.125f, 0.50f, 0.75 };
		size_t stage = 0;
		
		if(settings->GetObjectForKey(RNCSTR("guessMaterial")))
		{
			Number *number = settings->GetObjectForKey<Number>(RNCSTR("guessMaterial"));
			guessMaterial = number->GetBoolValue();
		}
		
		LoadLODStage(file, model, stage, guessMaterial);
		
		uint8 hasAnimations = file->ReadInt8();
		if(hasAnimations == 1)
		{
			std::string animationFile;
			std::string path;
			
			file->ReadIntoString(animationFile, file->ReadUint16());
			path = FileManager::GetSharedInstance()->GetNormalizedPathFromFullpath(PathManager::Join(file->GetPath(), animationFile));
			Skeleton *skeleton = Skeleton::WithFile(path);
			model->SetSkeleton(skeleton);
		}
		
		std::string base = PathManager::Basepath(file->GetFullPath());
		std::string name = PathManager::Basename(file->GetFullPath());
		std::string extension = PathManager::Extension(file->GetFullPath());
		
		while(stage < 5)
		{
			std::stringstream stream;
			stream << name << "_lod" << (stage + 1) << "." << extension;
			
			std::string lodPath = PathManager::Join(base, stream.str());
			
			try
			{
				File *lodFile = new File(lodPath);
				
				stage = model->AddLODStage(distance[stage]);
				LoadLODStage(lodFile, model, stage, guessMaterial);
				
				lodFile->Release();
			}
			catch(Exception e)
			{
				break;
			}
		}
		
		return model;
	}
	
	void SGMResourceLoader::LoadLODStage(File *file, Model *model, size_t stage, bool guessMaterial)
	{
		file->Seek(5); // Skip over magic bytes and version number
		
		Shader *shader = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyTexture1Shader, nullptr);
		
		std::vector<Material *> materials;
		uint8 materialCount = file->ReadUint8();
		
		// Get Materials
		for(uint8 i = 0; i < materialCount; i ++)
		{
			Material *material = new Material(shader);
			file->ReadUint8();
			
			uint8 uvCount = file->ReadUint8();
			
			for(uint8 u = 0; u < uvCount; u ++)
			{
				uint8 textureCount = file->ReadUint8();
				
				for(uint8 n = 0; n < textureCount; n ++)
				{
					uint8 usagehint = file->ReadUint8();
					
					std::string textureFile;
					std::string path;
					
					file->ReadIntoString(textureFile, file->ReadUint16());
					path = FileManager::GetSharedInstance()->GetNormalizedPathFromFullpath(PathManager::Join(file->GetPath(), textureFile));
					
					Texture *texture = Texture::WithFile(path, (usagehint == 1));
					material->AddTexture(texture);
					
					if(guessMaterial)
					{
						switch(usagehint)
						{
							case 1:
								material->Define("RN_NORMALMAP");
								break;
								
							case 2:
								material->Define("RN_SPECULARITY");
								material->Define("RN_SPECMAP");
								break;
								
							default:
								break;
						}
					}
				}
			}
			
			uint8 colorCount = file->ReadUint8();
			for(uint8 u = 0; u < colorCount; u ++)
			{
				uint8 usagehint = file->ReadUint8();
				Color color(file->ReadFloat(), file->ReadFloat(), file->ReadFloat(), file->ReadFloat());
				
				if(usagehint == 0 && u == 0)
					material->diffuse = color;
			}
			
			materials.push_back(material);
		}
		
		// Get Meshes
		uint8 meshCount = file->ReadUint8();
		
		for(uint8 i = 0; i < meshCount; i ++)
		{
			file->ReadUint8();
			
			Material *material = materials[file->ReadUint8()];
			
			uint32 verticesCount = file->ReadUint32();
			uint8 uvCount   = file->ReadUint8();
			uint8 dataCount = file->ReadUint8();
			bool hasTangent = file->ReadUint8();
			bool hasBones   = file->ReadUint8();
			
			std::vector<MeshDescriptor> descriptors;
			size_t size = 0;
			
			MeshDescriptor meshDescriptor(kMeshFeatureVertices);
			meshDescriptor.elementSize = sizeof(Vector3);
			meshDescriptor.elementMember = 3;
			
			descriptors.push_back(meshDescriptor);
			size += meshDescriptor.elementSize;
			
			meshDescriptor = MeshDescriptor(kMeshFeatureNormals);
			meshDescriptor.elementSize = sizeof(Vector3);
			meshDescriptor.elementMember = 3;
			
			descriptors.push_back(meshDescriptor);
			size += meshDescriptor.elementSize;
			
			if(uvCount > 0)
			{
				meshDescriptor = MeshDescriptor(kMeshFeatureUVSet0);
				meshDescriptor.elementSize = sizeof(Vector2);
				meshDescriptor.elementMember = 2;
				
				descriptors.push_back(meshDescriptor);
				size += meshDescriptor.elementSize;
			}
			
			if(hasTangent == 1)
			{
				meshDescriptor = MeshDescriptor(kMeshFeatureTangents);
				meshDescriptor.elementSize = sizeof(Vector4);
				meshDescriptor.elementMember = 4;
				
				descriptors.push_back(meshDescriptor);
				size += meshDescriptor.elementSize;
			}
			if(uvCount > 1)
			{
				meshDescriptor = MeshDescriptor(kMeshFeatureUVSet1);
				meshDescriptor.elementSize = sizeof(Vector2);
				meshDescriptor.elementMember = 2;
				
				descriptors.push_back(meshDescriptor);
				size += meshDescriptor.elementSize;
			}
			if(dataCount == 4)
			{
				meshDescriptor = MeshDescriptor(kMeshFeatureColor0);
				meshDescriptor.elementSize = sizeof(Vector4);
				meshDescriptor.elementMember = 4;
				
				descriptors.push_back(meshDescriptor);
				size += meshDescriptor.elementSize;
			}
			if(hasBones)
			{
				meshDescriptor = MeshDescriptor(kMeshFeatureBoneWeights);
				meshDescriptor.elementSize = sizeof(Vector4);
				meshDescriptor.elementMember = 4;
				
				descriptors.push_back(meshDescriptor);
				size += meshDescriptor.elementSize;
				
				meshDescriptor = MeshDescriptor(kMeshFeatureBoneIndices);
				meshDescriptor.elementSize = sizeof(Vector4);
				meshDescriptor.elementMember = 4;
				
				descriptors.push_back(meshDescriptor);
				size += meshDescriptor.elementSize;
			}
			
			size *= verticesCount;
			
			uint8 *vertexData = new uint8[size];
			file->ReadIntoBuffer(vertexData, size);
			
			uint32 indicesCount = file->ReadUint32();
			uint8 indicesSize   = file->ReadUint8();
			
			uint8 *indicesData = new uint8[indicesCount * indicesSize];
			file->ReadIntoBuffer(indicesData, indicesCount * indicesSize);
			
			meshDescriptor = MeshDescriptor(kMeshFeatureIndices);
			meshDescriptor.elementSize = indicesSize;
			meshDescriptor.elementMember = 1;
			descriptors.push_back(meshDescriptor);
			
			Mesh *mesh = new Mesh(descriptors, verticesCount, indicesCount, std::make_pair(vertexData, indicesData));
			
			if(stage == 0)
				mesh->CalculateBoundingVolumes();
			
			delete [] vertexData;
			delete [] indicesData;
			
			model->AddMesh(mesh, material, stage);
		}
	}
	
	bool SGMResourceLoader::SupportsLoadingFile(File *file)
	{
		file->Seek(4); // Skip the magic bytes, they've already been checked
		uint32 version = file->ReadUint8();
		
		switch(version)
		{
			case 3:
				return true;
				
			default:
				return false;
		}
	}
	
	bool SGMResourceLoader::SupportsBackgroundLoading()
	{
		return true;
	}
	
	uint32 SGMResourceLoader::GetPriority() const
	{
		return kRNResourceCoordinatorBuiltInPriority;
	}
	
	// ---------------------
	// MARK: -
	// MARK: SGAResourceLoader
	// ---------------------
	
	SGAResourceLoader::SGAResourceLoader() :
	ResourceLoader(Skeleton::MetaClass())
	{
		uint8 magicBytes[] = { 0x5A, 0x4E, 0xDA, 0x16 };
		
		Data *data = new Data(magicBytes, 4);
		
		SetFileExtensions({ "sga" });
		SetMagicBytes(data, 0);
		
		data->Release();
	}
	
	void SGAResourceLoader::InitialWakeUp(MetaClassBase *meta)
	{
		if(meta == MetaClass())
		{
			SGAResourceLoader *loader = new SGAResourceLoader();
			ResourceCoordinator::GetSharedInstance()->RegisterResourceLoader(loader);
			loader->Release();
		}
	}
	
	Object *SGAResourceLoader::Load(File *file, Dictionary *settings)
	{
		Skeleton *skeleton = new Skeleton();
		
		file->Seek(5); // Skip over magic bytes and version number
		
		uint16 lenskeletonname = file->ReadUint16();
		char *skeletonname = new char[lenskeletonname];
		file->ReadIntoBuffer(skeletonname, lenskeletonname);
		delete[] skeletonname;
		
		uint16 bonecount = file->ReadUint16();
		for(int i = 0; i < bonecount; i++)
		{
			uint16 lenbonename = file->ReadUint16();
			char *bonename = new char[lenbonename];
			file->ReadIntoBuffer(bonename, lenbonename);
			
			Vector3 bonepos;
			file->ReadIntoBuffer(&bonepos.x, sizeof(Vector3));
			
			uint8 isroot = file->ReadUint8();
			
			Bone bone(bonepos, bonename, isroot);
			delete[] bonename;
			
			uint16 numchildren = file->ReadUint16();
			for(int n = 0; n < numchildren; n++)
			{
				uint16 child = file->ReadUint16();
				bone.tempChildren.push_back(child);
			}
			
			skeleton->bones.push_back(bone);
		}
		
		uint16 numanims = file->ReadUint16();
		for(int i = 0; i < numanims; i++)
		{
			uint16 lenanimname = file->ReadUint16();
			char *animname = new char[lenanimname];
			file->ReadIntoBuffer(animname, lenanimname);
			
			Animation *anim = new Animation(animname);
			anim->Autorelease();
			anim->Retain();
			skeleton->animations.insert(std::pair<std::string, Animation*>(animname, anim));
			delete[] animname;
			
			uint16 numanimbones = file->ReadUint16();
			for(int n = 0; n < numanimbones; n++)
			{
				AnimationBone *animbone = 0;
				uint16 boneid = file->ReadUint16();
				uint32 numframes = file->ReadUint32();
				for(int f = 0; f < numframes; f++)
				{
					float time = file->ReadFloat();
					Vector3 animbonepos;
					file->ReadIntoBuffer(&animbonepos.x, sizeof(Vector3));
					Vector3 animbonescale;
					file->ReadIntoBuffer(&animbonescale.x, sizeof(Vector3));
					Quaternion animbonerot;
					file->ReadIntoBuffer(&animbonerot.x, sizeof(Quaternion));
					
					animbone = new AnimationBone(animbone, 0, time, animbonepos, animbonescale, animbonerot);
				}
				
				AnimationBone *lastbone = animbone;
				while(animbone->prevFrame != 0)
				{
					animbone->prevFrame->nextFrame = animbone;
					animbone = animbone->prevFrame;
				}
				animbone->prevFrame = lastbone;
				lastbone->nextFrame = animbone;
				anim->bones.insert(std::pair<int, AnimationBone*>(boneid, animbone));
			}
		}
		
		skeleton->Init();
		
		return skeleton;
	}
	
	bool SGAResourceLoader::SupportsLoadingFile(File *file)
	{
		file->Seek(4); // Skip the magic bytes, they've already been checked
		uint32 version = file->ReadUint8();
		
		switch(version)
		{
			case 1:
				return true;
				
			default:
				return false;
		}
	}
	
	bool SGAResourceLoader::SupportsBackgroundLoading()
	{
		return true;
	}
	
	uint32 SGAResourceLoader::GetPriority() const
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
