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

		Data *fileData = Data::WithContentsOfFile(file->GetPath());
		bool res = gltfLoader.LoadBinaryFromMemory(&gltfModel, &err, &warn, fileData->GetBytes<unsigned char>(), fileData->GetLength());
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

		// Load a full model
		Model *model = new Model();
		size_t stageIndex = 0;

		std::vector<float> lodFactors = Model::GetDefaultLODFactors();
		LoadGLTFLODStage(gltfModel, model->AddLODStage(lodFactors[0]), options);
		
		if(gltfModel.skins.size() > 0)
		{
			Skeleton *skeleton = new Skeleton();
			tinygltf::Skin &gltfSkin = gltfModel.skins[0];
			
			tinygltf::Accessor &invBaseMatrixAccessor = gltfModel.accessors[gltfSkin.inverseBindMatrices];
			tinygltf::BufferView &matrixBufferView = gltfModel.bufferViews[invBaseMatrixAccessor.bufferView];
			tinygltf::Buffer &matrixBuffer = gltfModel.buffers[matrixBufferView.buffer];
			int matrixBufferByteStride = invBaseMatrixAccessor.ByteStride(matrixBufferView);
			
			std::map<int, int> jointIndexMapping;
			for(int i = 0; i < gltfSkin.joints.size(); i++)
			{
				jointIndexMapping[gltfSkin.joints[i]] = i;
			}
			
			for(int i = 0; i < gltfSkin.joints.size(); i++)
			{
				bool isRoot = gltfSkin.skeleton != -1? (gltfSkin.joints[i] == gltfSkin.skeleton) : (i == 0);
				tinygltf::Node &gltfNode = gltfModel.nodes[gltfSkin.joints[i]];
				Vector3 bonepos;
				/*if(gltfNode.translation.size() == 3)
				{
					bonepos = Vector3(gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]);
				}*/
				Bone bone(bonepos, RNSTR(gltfNode.name), isRoot);
				/*if(gltfNode.rotation.size() == 4)
				{
					bone.rotation = Quaternion(gltfNode.rotation[0], gltfNode.rotation[1], gltfNode.rotation[2], gltfNode.rotation[3]);
				}
				if(gltfNode.scale.size() == 3)
				{
					bone.scale = Vector3(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]);
				}*/
				
				for(int n = 0; n < gltfNode.children.size(); n++)
				{
					bone.tempChildren.push_back(jointIndexMapping[gltfNode.children[n]]);
				}
				
				//std::memcpy(bone.invBaseMatrix.m, &matrixBuffer.data[matrixBufferByteStride * i + matrixBufferView.byteOffset + invBaseMatrixAccessor.byteOffset], 64);
				
				//bone.absolute = true;
				
				skeleton->bones.push_back(bone);
			}
			
			skeleton->Init();
			model->SetSkeleton(skeleton);
		}

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
				
				//TODO: Support other component types
				if(accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT && accessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) continue;

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
						type = PrimitiveType::Matrix4x4;
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
				if(attrib.first.compare("JOINTS_0") == 0)
				{
					attributes.emplace_back(Mesh::VertexAttribute::Feature::BoneIndices, type);
					attributeAccessors.emplace_back(attrib.second);
				}
				if(attrib.first.compare("WEIGHTS_0") == 0)
				{
					attributes.emplace_back(Mesh::VertexAttribute::Feature::BoneWeights, type);
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
			size_t elementCount = 4;
			switch(attributes[i].GetType())
			{
				case PrimitiveType::Vector2:
					elementSize = 8;
					elementCount = 2;
					break;
				case PrimitiveType::Vector3:
					elementSize = 12;
					elementCount = 3;
					break;
				case PrimitiveType::Vector4:
					elementSize = 16;
					elementCount = 4;
					break;
				case PrimitiveType::Matrix4x4:
					elementSize = 64;
					elementCount = 16;
					break;
				case PrimitiveType::Float:
					elementSize = 4;
					elementCount = 1;
					break;
				case PrimitiveType::Uint16:
					elementSize = 2;
					elementCount = 1;
					break;
				case PrimitiveType::Uint32:
					elementSize = 4;
					elementCount = 1;
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
			else if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
			{
				for(int n = 0; n < vertexCount; n++)
				{
					std::memcpy(data->GetBytes<uint8>() + n * elementSize, &buffer.data[byteStride * n + bufferView.byteOffset + accessor.byteOffset], elementSize);
				}
			}
			else if(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
			{
				for(int n = 0; n < vertexCount; n++)
				{
					std::vector<float> floatDataArray;
					for(int m = 0; m < elementCount; m++)
					{
						uint8 byteData = buffer.data[byteStride * n + bufferView.byteOffset + accessor.byteOffset + m];
						float floatData = byteData;
						floatDataArray.push_back(floatData);
					}
					
					std::memcpy(data->GetBytes<uint8>() + n * elementSize, floatDataArray.data(), elementSize);
				}
			}
			
			mesh->SetElementData(attributes[i].GetFeature(), data->GetBytes());
		}

		mesh->EndChanges();

		return mesh->Autorelease();
	}
}
