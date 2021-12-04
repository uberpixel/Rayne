//
//  RNglTFAssetLoader.cpp
//  RayneGlTF
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#include "RNglTFAssetLoader.h"

namespace RN
{
	RNDefineMeta(GlTFAssetLoader, AssetLoader)

	static GlTFAssetLoader *__assetLoader;

	void GlTFAssetLoader::InitialWakeUp(MetaClass *meta)
	{
		if(meta == GlTFAssetLoader::GetMetaClass())
		{
			Config config({ Mesh::GetMetaClass(), Model::GetMetaClass() });
			config.SetExtensions(Set::WithObjects({ RNCSTR("glb") })); //TODO: Also support non binary gltf files
			config.supportsBackgroundLoading = true;

			__assetLoader = new GlTFAssetLoader(config);

			AssetManager *manager = AssetManager::GetSharedInstance();
			manager->RegisterAssetLoader(__assetLoader);
		}
	}

	GlTFAssetLoader::GlTFAssetLoader(const Config &config) :
		AssetLoader(config)
	{}

	Asset *GlTFAssetLoader::Load(File *file, const LoadOptions &options)
	{
		Dictionary *settings = options.settings;
		MetaClass *meta = options.meta;

/*		if(settings->GetObjectForKey(RNCSTR("recalculateNormals")))
		{
			Number *number = settings->GetObjectForKey<Number>(RNCSTR("recalculateNormals"));
			recalculateNormals = number->GetBoolValue();
		}*/
		
		tinygltf::TinyGLTF gltfLoader;
		tinygltf::Model gltfModel;
		std::string err;
		std::string warn;

		bool res = gltfLoader.LoadBinaryFromFile(&gltfModel, &err, &warn, file->GetPath()->GetUTF8String());
		if(!warn.empty())
		{
			RNDebug("load glTF warning: " << warn);
		}

		if(!err.empty())
		{
			RNDebug("load glTF error: " << err);
		}

		if(!res)
		{
			throw InconsistencyException(RNSTR("Failed to load gltf file: " << file->GetPath()));
		}
		else
		{
			RNDebug("Loaded glTF: " << file->GetPath());
		}

//		if(meta->InheritsFromClass(Mesh::GetMetaClass()))
//			return LoadGLTFMesh(scene, 0);

		// Load a full model
		Model *model = new Model();
		size_t stageIndex = 0;

		std::vector<float> lodFactors = Model::GetDefaultLODFactors();
		LoadGLTFLODStage(gltfModel, model->AddLODStage(lodFactors[0]), options);

		model->CalculateBoundingVolumes();
		return model->Autorelease();
	}

	void GlTFAssetLoader::LoadGLTFLODStage(tinygltf::Model &gltfModel, Model::LODStage *stage, const LoadOptions &options)
	{
		for(size_t i = 0; i < gltfModel.meshes.size(); ++i)
		{
			auto pair = LoadGLTFMeshGroup(gltfModel, gltfModel.meshes[i], options);
			stage->AddMesh(pair.first, pair.second->Autorelease());
		}
		
		
/*		if(options.queue)
		{
			for(size_t i = 0; i < scene->mNumMeshes; i++)
			{
				aiMesh *aimesh = scene->mMeshes[i];
				aiMaterial *aimaterial = scene->mMaterials[aimesh->mMaterialIndex];

				LoadAsyncTexture(aimaterial, filepath, aiTextureType_DIFFUSE, 0);
			}
		}*/
	}

	std::pair<Mesh *, Material *> GlTFAssetLoader::LoadGLTFMeshGroup(tinygltf::Model &gltfModel, tinygltf::Mesh &gltfMesh, const LoadOptions &options)
	{
		Mesh *mesh = LoadGLTFMesh(gltfModel, gltfMesh);

		Renderer *renderer = Renderer::GetActiveRenderer();
		Material *material = RN::Material::WithShaders(nullptr, nullptr);
		
		if(gltfModel.materials.size() > 0)
		{
			// fixme: Use material's baseColor
			tinygltf::Material &mat = gltfModel.materials[0];
			tinygltf::Texture &tex = gltfModel.textures[mat.pbrMetallicRoughness.baseColorTexture.index];

			if(tex.source > -1)
			{
				tinygltf::Image &image = gltfModel.images[tex.source];

				Texture::Format textureFormat = Texture::Format::RGBA_8;
				if(image.bits == 8)
				{
					if(image.component == 1)
					{
						textureFormat = Texture::Format::R_8;
					}
					else if(image.component == 2)
					{
						textureFormat = Texture::Format::RG_8;
					}
					else if(image.component == 3)
					{
						textureFormat = Texture::Format::RGB_8;
					}
					else if(image.component == 4)
					{
						textureFormat = Texture::Format::RGBA_8;
					}
					else
					{
						//Not supported
						//TODO: Show error
					}
				}
/*				else if(image.bits == 16)
				{
					if(image.component == 1)
					{
						textureFormat = Texture::Format::R_16F;
					}
					else if(image.component == 2)
					{
						textureFormat = Texture::Format::RG_16F;
					}
					else if(image.component == 3)
					{
						textureFormat = Texture::Format::RGB_16F;
					}
					else if(image.component == 4)
					{
						textureFormat = Texture::Format::RGBA_16F;
					}
					else
					{
						//Not supported
						//TODO: Show error
					}
				}*/
				else
				{
					//Not supported
					//TODO: Show error
				}
				
				Texture::Descriptor descriptor = Texture::Descriptor::With2DTextureAndFormat(textureFormat, image.width, image.height, true);
				Texture *texture = Renderer::GetActiveRenderer()->CreateTextureWithDescriptor(descriptor);
				texture->SetData(0, &image.image.at(0), image.width * image.component * image.bits / 8, image.height);
				texture->GenerateMipMaps();
				material->AddTexture(texture->Autorelease());
			}
		}

		Shader::Options *shaderOptions = Shader::Options::WithMesh(mesh);
		/*if(wantsDiscard)
		{
			shaderOptions->EnableAlpha();
			material->SetAlphaToCoverage(true);
		}*/
		
		if(options.settings->GetObjectForKey(RNCSTR("enablePointLights")))
		{
			Number *number = options.settings->GetObjectForKey<Number>(RNCSTR("enablePointLights"));
			if(number->GetBoolValue())
			{
				shaderOptions->EnablePointLights();
			}
		}
		
		if(options.settings->GetObjectForKey(RNCSTR("enableDirectionalLights")))
		{
			Number *number = options.settings->GetObjectForKey<Number>(RNCSTR("enableDirectionalLights"));
			if(number->GetBoolValue())
			{
				shaderOptions->EnableDirectionalLights();
				
				if(options.settings->GetObjectForKey(RNCSTR("enableDirectionalShadows")))
				{
					Number *number = options.settings->GetObjectForKey<Number>(RNCSTR("enableDirectionalShadows"));
					if(number->GetBoolValue())
					{
						shaderOptions->EnableDirectionalShadows();
					}
				}
			}
		}
		
		material->SetVertexShader(renderer->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Default), Shader::UsageHint::Default);
		material->SetFragmentShader(renderer->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Default), Shader::UsageHint::Default);
		material->SetVertexShader(renderer->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Depth), Shader::UsageHint::Depth);
		material->SetFragmentShader(renderer->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Depth), Shader::UsageHint::Depth);
		material->SetVertexShader(renderer->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Instancing), Shader::UsageHint::Instancing);
		material->SetFragmentShader(renderer->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Instancing), Shader::UsageHint::Instancing);

		return std::make_pair(mesh, material->Retain());
	}


	Mesh *GlTFAssetLoader::LoadGLTFMesh(tinygltf::Model &gltfModel, tinygltf::Mesh &gltfMesh)
	{
		std::vector<Mesh::VertexAttribute> attributes;
		std::vector<int> attributeAccessors;
		size_t indexCount = 0;
		size_t vertexCount = 0;
		
		for(size_t i = 0; i < gltfMesh.primitives.size(); ++i)
		{
			if(i > 0) continue; //TODO: Not sure what it means to have multiple primitives, so just sticking to the first for now
			
			tinygltf::Primitive primitive = gltfMesh.primitives[i];

			for(auto &attrib : primitive.attributes)
			{
				tinygltf::Accessor accessor = gltfModel.accessors[attrib.second];
				vertexCount = accessor.count;

				PrimitiveType type = PrimitiveType::Float;
				switch(accessor.type)
				{
					case TINYGLTF_TYPE_VEC2:
						type = PrimitiveType::Vector2;
						break;
					case TINYGLTF_TYPE_VEC3:
						type = PrimitiveType::Vector3;
						break;
					case TINYGLTF_TYPE_VEC4:
					case TINYGLTF_TYPE_VECTOR:
						type = PrimitiveType::Vector4;
						break;
					case TINYGLTF_TYPE_MAT2:
					case TINYGLTF_TYPE_MAT3:
					case TINYGLTF_TYPE_MAT4:
					case TINYGLTF_TYPE_MATRIX:
						type = PrimitiveType::Matrix;
						break;
					case TINYGLTF_TYPE_SCALAR:
						type = PrimitiveType::Float;
						break;
				}

				if(attrib.first.compare("POSITION") == 0)
				{
					attributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, type);
					attributeAccessors.emplace_back(attrib.second);
				}
				if(attrib.first.compare("NORMAL") == 0)
				{
					attributes.emplace_back(Mesh::VertexAttribute::Feature::Normals, type);
					attributeAccessors.emplace_back(attrib.second);
				}
				if(attrib.first.compare("TEXCOORD_0") == 0)
				{
					attributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, type);
					attributeAccessors.emplace_back(attrib.second);
				}
				if(attrib.first.compare("TEXCOORD_1") == 0)
				{
					attributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords1, type);
					attributeAccessors.emplace_back(attrib.second);
				}
				
				//TODO: Support other attributes
			}
			
			tinygltf::Accessor indexAccessor = gltfModel.accessors[primitive.indices];
			
			indexCount = indexAccessor.count;
			PrimitiveType indicesType = PrimitiveType::Uint16;
			if(indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
			{
				indicesType = PrimitiveType::Uint32;
			}
			attributes.emplace_back(Mesh::VertexAttribute::Feature::Indices, indicesType);
			attributeAccessors.emplace_back(primitive.indices);
		}


		Mesh *mesh = new Mesh(attributes, vertexCount, indexCount);

		mesh->BeginChanges();
		Data *data = Data::WithBytes(nullptr, 4 * 4 * std::max(vertexCount, indexCount / 4 + 1));
		for(int i = 0; i < attributes.size(); i++)
		{
			const tinygltf::Accessor &accessor = gltfModel.accessors[attributeAccessors[i]];
			const tinygltf::BufferView &bufferView = gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer &buffer = gltfModel.buffers[bufferView.buffer];
			
			int byteStride = accessor.ByteStride(bufferView);
			
			size_t elementSize = 4;
			switch(attributes[i].GetType())
			{
				case PrimitiveType::Vector2:
					elementSize = 8;
					break;
				case PrimitiveType::Vector3:
					elementSize = 12;
					break;
				case PrimitiveType::Vector4:
					elementSize = 16;
					break;
				case PrimitiveType::Matrix:
					elementSize = 64;
					break;
				case PrimitiveType::Float:
					elementSize = 4;
					break;
				case PrimitiveType::Uint16:
					elementSize = 2;
					break;
				case PrimitiveType::Uint32:
					elementSize = 4;
					break;
					
				default:
					break;
			}
			
			if(attributes[i].GetFeature() == Mesh::VertexAttribute::Feature::Indices)
			{
				for(int n = 0; n < indexCount; n++)
				{
					std::memcpy(data->GetBytes<uint8>() + n * elementSize, &buffer.data[byteStride * n + bufferView.byteOffset + accessor.byteOffset], elementSize);
				}
			}
			else
			{
				for(int n = 0; n < vertexCount; n++)
				{
					std::memcpy(data->GetBytes<uint8>() + n * elementSize, &buffer.data[byteStride * n + bufferView.byteOffset + accessor.byteOffset], elementSize);
				}
			}
			
			mesh->SetElementData(attributes[i].GetFeature(), data->GetBytes());
		}

		mesh->EndChanges();

		return mesh->Autorelease();
	}
}
