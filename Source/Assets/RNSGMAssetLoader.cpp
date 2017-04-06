//
//  RNSGMAssetLoader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Rendering/RNRenderer.h"
#include "../System/RNFileManager.h"
#include "../Threads/RNWorkQueue.h"
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

	Asset *SGMAssetLoader::Load(File *file, const LoadOptions &options)
	{
		bool autoLoadLOD = false;

		if(options.settings->GetObjectForKey(RNCSTR("autoLoadLOD")))
		{
			Number *number = options.settings->GetObjectForKey<Number>(RNCSTR("autoLoadLOD"));
			autoLoadLOD = number->GetBoolValue();
		}

		// Load a full model
		Model *model = new Model();
		size_t stageIndex = 0;

		std::vector<float> lodFactors = Model::GetDefaultLODFactors();

		const String *path = file->GetPath();
		String *basePath = path->StringByDeletingLastPathComponent();

		LoadLODStage(file, model->AddLODStage(lodFactors[0]), options);

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
					LoadLODStage(lodFile, stage, options);
				}
				catch(Exception )
				{
					break;
				}

				stageIndex ++;
			}
		}

		model->CalculateBoundingVolumes();
		return model->Autorelease();
	}

	void SGMAssetLoader::LoadLODStage(File *file, Model::LODStage *stage, const LoadOptions &options)
	{
		const String *path = file->GetPath()->StringByDeletingLastPathComponent();

		file->Seek(5); // Skip over magic bytes and version number

		uint8 materialCount = file->ReadUint8();

		Renderer *renderer = Renderer::GetActiveRenderer();
		std::vector<std::pair<bool, MaterialDescriptor>> materials;

		// Get Materials
		Array *materialPlaceholder = new Array();

		for(uint8 i = 0; i < materialCount; i ++)
		{
			Array *textures = new Array();

			file->ReadUint8();

			uint8 uvCount = file->ReadUint8();

			for(uint8 u = 0; u < uvCount; u ++)
			{
				uint8 textureCount = file->ReadUint8();

				for(uint8 n = 0; n < textureCount; n ++)
				{
					/*RN_UNUSED uint8 usageHint =*/ file->ReadUint8();

					size_t length = file->ReadUint16();
					char *buffer = new char[length];

					file->Read(buffer, length);

					String *filename = RNSTR(buffer);
					String *fullPath = path->StringByAppendingPathComponent(filename);

					String *normalized = FileManager::GetSharedInstance()->GetNormalizedPathFromFullPath(fullPath);

					if(options.queue)
						AssetManager::GetSharedInstance()->GetFutureAssetWithName<Texture>(normalized, nullptr);
					else
						AssetManager::GetSharedInstance()->GetAssetWithName<Texture>(normalized, nullptr);

					textures->AddObject(normalized);

					delete[] buffer;
				}
			}

			Color diffuseColor;
			uint8 colorCount = file->ReadUint8();
			for(uint8 u = 0; u < colorCount; u ++)
			{
				RN_UNUSED uint8 usagehint = file->ReadUint8();
				Color color;
				color.r = file->ReadFloat();
				color.g = file->ReadFloat();
				color.b = file->ReadFloat();
				color.a = file->ReadFloat();

				if(usagehint == 0 && u == 0)
					diffuseColor = color;
			}

			Dictionary *info = new Dictionary();
			info->SetObjectForKey(textures, RNCSTR("textures"));
			info->SetObjectForKey(Value::WithColor(diffuseColor), RNCSTR("diffusecolor"));

			materialPlaceholder->AddObject(info);

			textures->Release();
			info->Release();
		}

		// Collect the loaded textures
		materialPlaceholder->Enumerate<Dictionary>([&](Dictionary *info, size_t index, bool &stop) {

			Array *textures = info->GetObjectForKey<Array>(RNCSTR("textures"));

			MaterialDescriptor descriptor;
			bool wantsDiscard = false;

			textures->Enumerate<String>([&](String *file, size_t index, bool &stop) {

				Texture *texture;

				if(options.queue)
				{
					std::shared_future<StrongRef<Asset>> future = AssetManager::GetSharedInstance()->GetFutureAssetWithName<Texture>(file, nullptr);
					WorkQueue *queue = WorkQueue::GetCurrentWorkQueue();

					if(queue)
						queue->YieldWithFuture(future);
					else
						future.wait();

					texture = future.get()->Downcast<Texture>();
				}
				else
				{
					texture = AssetManager::GetSharedInstance()->GetAssetWithName<Texture>(file, nullptr);
				}

				descriptor.AddTexture(texture);

				//Activate discarding of transparent pixels if first texture has alpha
				if(index == 0 && texture->HasColorChannel(Texture::ColorChannel::Alpha))
					wantsDiscard = true;

			});

			Value *diffuseColor = info->GetObjectForKey<Value>(RNCSTR("diffusecolor"));
			if(diffuseColor)
			{
				descriptor.diffuseColor = diffuseColor->GetValue<Color>();
			}

			materials.emplace_back(std::make_pair(wantsDiscard, descriptor));

		});

		materialPlaceholder->Release();


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
			
			(void)(offset);

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

			uint8 *indicesBuffer = new uint8[indicesCount * indicesSize];
			file->Read(indicesBuffer, indicesCount * indicesSize);
			mesh->SetElementData(Mesh::VertexAttribute::Feature::Indices, indicesBuffer);
			delete[] indicesBuffer;

			mesh->EndChanges();

			// Load the material
			bool wantsDiscard = materialPair.first;
			MaterialDescriptor &descriptor = materialPair.second;
			Shader::Options *shaderOptions = Shader::Options::WithMesh(mesh);
			if(wantsDiscard)
				shaderOptions->EnableDiscard();

			descriptor.vertexShader = renderer->GetDefaultShader(Shader::Type::Vertex, shaderOptions);
			descriptor.fragmentShader = renderer->GetDefaultShader(Shader::Type::Fragment, shaderOptions);

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
