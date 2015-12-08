//
//  RNSGMAssetLoader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Rendering/RNRenderer.h"
#include "../System/RNFileCoordinator.h"
#include "RNSGMAssetLoader.h"
#include "RNAssetManager.h"

namespace RN
{
	RNDefineMeta(SGMAssetLoader, AssetLoader)

	static SGMAssetLoader *__assetLoader;

	void SGMAssetLoader::Register()
	{
		uint8 magic[] = { 0x90, 0x22, 0x5, 0x15 };

		Config config(Model::GetMetaClass());
		config.SetExtensions(Set::WithObjects({RNCSTR("sgm")}));
		config.SetMagicBytes(Data::WithBytes(magic, 4), 0);
		config.supportsBackgroundLoading = true;

		__assetLoader = new SGMAssetLoader(config);

		AssetManager *coordinator = AssetManager::GetSharedInstance();
		coordinator->RegisterAssetLoader(__assetLoader);
	}

	SGMAssetLoader::SGMAssetLoader(const Config &config) :
		AssetLoader(config)
	{}

	Asset *SGMAssetLoader::Load(File *file, MetaClass *meta, Dictionary *settings)
	{
		bool autoLoadLOD = false;

		if(settings->GetObjectForKey(RNCSTR("autoLoadLOD")))
		{
			Number *number = settings->GetObjectForKey<Number>(RNCSTR("autoLoadLOD"));
			autoLoadLOD = number->GetBoolValue();
		}

		// Load a full model
		Model *model = new Model();
		size_t stageIndex = 0;

		std::vector<float> lodFactors = Model::GetDefaultLODFactors();

		const String *path = file->GetPath();
		String *basePath = path->StringByDeletingLastPathComponent();

		LoadLODStage(file, model->AddLODStage(lodFactors[0]));

		if(autoLoadLOD)
		{
			String *extension = path->GetPathExtension();
			String *name = path->GetLastPathComponent()->StringByDeletingLastPathComponent();

			stageIndex ++;

			while(stageIndex < lodFactors.size())
			{
				String *lodPath = basePath->StringByAppendingPathComponent(RNSTR(name << "_lod" << stageIndex << "." << extension));

				try
				{
					File *lodFile = File::WithName(lodPath);

					Model::LODStage *stage = model->AddLODStage(lodFactors[stageIndex]);
					LoadLODStage(lodFile, stage);
				}
				catch(Exception &e)
				{
					break;
				}

				stageIndex ++;
			}
		}

		model->CalculateBoundingVolumes();
		return model->Autorelease();
	}

	void SGMAssetLoader::LoadLODStage(File *file, Model::LODStage *stage)
	{
		const String *path = file->GetPath()->StringByDeletingLastPathComponent();

		file->Seek(5); // Skip over magic bytes and version number

		uint8 materialCount = file->ReadUint8();

		Renderer *renderer = Renderer::GetActiveRenderer();
		std::vector<std::pair<ShaderLookupRequest *, MaterialDescriptor>> materials;

		// Get Materials
		for(uint8 i = 0; i < materialCount; i ++)
		{
			ShaderLookupRequest *lookup = new ShaderLookupRequest();
			MaterialDescriptor descriptor;

			file->ReadUint8();

			uint8 uvCount = file->ReadUint8();

			for(uint8 u = 0; u < uvCount; u ++)
			{
				uint8 textureCount = file->ReadUint8();

				for(uint8 n = 0; n < textureCount; n ++)
				{
					__unused uint8 usageHint = file->ReadUint8();

					size_t length = file->ReadUint16();
					char *buffer = new char[length];

					file->Read(buffer, length);

					String *filename = RNSTR(buffer);
					String *fullPath = path->StringByAppendingPathComponent(filename);

					String *normalized = FileCoordinator::GetSharedInstance()->GetNormalizedPathFromFullPath(fullPath);
					Texture *texture = Texture::WithName(normalized);

					descriptor.AddTexture(texture);

					if(texture->HasColorChannel(Texture::ColorChannel::Alpha))
						lookup->discard = true;

					delete[] buffer;
				}
			}

			uint8 colorCount = file->ReadUint8();

			for(uint8 u = 0; u < colorCount; u ++)
			{
				uint8 usagehint = file->ReadUint8();
				Color color;
				color.r = file->ReadFloat();
				color.g = file->ReadFloat();
				color.b = file->ReadFloat();
				color.a = file->ReadFloat();

				//if(usagehint == 0 && u == 0)
				//	descriptor->S(color);
			}

			materials.emplace_back(std::make_pair(lookup, descriptor));
			lookup->Autorelease();
		}

		// Get Meshes
		size_t meshCount = file->ReadUint8();

		for(size_t i = 0; i < meshCount; i ++)
		{
			file->ReadUint8();

			auto &materialPair = materials[file->ReadUint8()];

			uint32 verticesCount = file->ReadUint32();
			uint8 uvCount   = file->ReadUint8();
			uint8 dataCount = file->ReadUint8();
			bool hasTangent = file->ReadUint8();
			bool hasBones   = file->ReadUint8();

			std::vector<Mesh::VertexAttribute> attributes;

			attributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector3);
			attributes.emplace_back(Mesh::VertexAttribute::Feature::Normals, PrimitiveType::Vector3);

			size_t size = sizeof(Vector3) * 2;
			size_t offset = size;

			size_t uv0Offset = 0;
			size_t uv1Offset = 0;
			size_t tangentOffset = 0;
			size_t colorOffset = 0;

			if(uvCount > 0)
			{
				attributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2);
				size += sizeof(Vector2);
				uv0Offset = offset;
				offset += sizeof(Vector2);
			}

			if(hasTangent)
			{
				attributes.emplace_back(Mesh::VertexAttribute::Feature::Tangents, PrimitiveType::Vector4);
				size += sizeof(Vector4);
				tangentOffset = offset;
				offset += sizeof(Vector4);
			}

			if(uvCount > 1)
			{
				attributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords1, PrimitiveType::Vector2);
				size += sizeof(Vector2);
				uv1Offset = offset;
				offset += sizeof(Vector2);
			}

			if(dataCount == 4)
			{
				attributes.emplace_back(Mesh::VertexAttribute::Feature::Color0, PrimitiveType::Color);
				size += sizeof(Color);
				colorOffset = offset;
				offset += sizeof(Color);
			}

			if(hasBones)
			{
				attributes.emplace_back(Mesh::VertexAttribute::Feature::BoneWeights, PrimitiveType::Vector4);
				attributes.emplace_back(Mesh::VertexAttribute::Feature::BoneIndices, PrimitiveType::Vector4);

				size += sizeof(Vector4) * 2;
			}

			size_t verticesSize = size * verticesCount;

			size_t vertexOffset = file->GetOffset();
			file->Seek(verticesSize, false);

			uint32 indicesCount = file->ReadUint32();
			uint8 indicesSize = file->ReadUint8();

			size_t indicesOffset = file->GetOffset();

			attributes.emplace_back(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint16);


			// Load the mesh data
			Mesh *mesh = new Mesh(attributes, verticesCount, indicesCount);

			mesh->BeginChanges();

			// Tear the mesh apart
			uint8 *buffer = new uint8[verticesSize];
			uint8 *buildBuffer = new uint8[verticesCount * sizeof(Vector4)];

			file->Seek(vertexOffset);
			file->Read(buffer, verticesSize);

#define CopyVertexData(elementSize, offset, feature) \
			do { \
				uint8 *tempBuffer = buffer; \
				uint8 *tempBuildBuffer = buildBuffer; \
	            for(size_t i = 0; i < verticesCount; i ++) \
    	        { \
					std::copy(tempBuffer + offset, tempBuffer + offset + elementSize, tempBuildBuffer); \
	                tempBuildBuffer += elementSize; \
					tempBuffer += size; \
            	} \
				mesh->SetElementData(feature, buildBuffer); \
            } while(0)

			CopyVertexData(sizeof(Vector3), 0, Mesh::VertexAttribute::Feature::Vertices);
			CopyVertexData(sizeof(Vector3), sizeof(Vector3), Mesh::VertexAttribute::Feature::Normals);

			if(uvCount > 0)
				CopyVertexData(sizeof(Vector2), uv0Offset, Mesh::VertexAttribute::Feature::UVCoords0);
			if(uvCount > 1)
				CopyVertexData(sizeof(Vector2), uv1Offset, Mesh::VertexAttribute::Feature::UVCoords1);
			if(hasTangent)
				CopyVertexData(sizeof(Vector4), tangentOffset, Mesh::VertexAttribute::Feature::Tangents);
			if(dataCount == 4)
				CopyVertexData(sizeof(Color), colorOffset, Mesh::VertexAttribute::Feature::Color0);

			delete[] buildBuffer;
			delete[] buffer;

			// Read the indices
			file->Seek(indicesOffset);

			GPUBuffer *indexBuffer = mesh->GetIndicesBuffer();
			file->Read(indexBuffer->GetBuffer(), indicesCount * indicesSize);
			indexBuffer->Invalidate();

			mesh->EndChanges();

			// Load the material
			ShaderLookupRequest *lookup = materialPair.first;
			MaterialDescriptor &descriptor = materialPair.second;

			descriptor.SetShaderProgram(renderer->GetDefaultShader(mesh, lookup));

			stage->AddMesh(mesh, Material::WithDescriptor(descriptor));
			mesh->Autorelease();
		}
	}

	bool SGMAssetLoader::SupportsLoadingFile(File *file) const
	{
		file->Seek(4); // Skip the magic bytes, they've already been checked
		uint32 version = file->ReadUint8();

		return (version == 3);
	}
}
