//
//  RNAssimpAssetLoader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAssimpAssetLoader.h"

namespace RN
{
	RNDefineMeta(AssimpAssetLoader, AssetLoader)

	static AssimpAssetLoader *__assetLoader;

	void AssimpAssetLoader::InitialWakeUp(MetaClass *meta)
	{
		if(meta == AssimpAssetLoader::GetMetaClass())
		{
			aiString extensionsString;
			Assimp::Importer{}.GetExtensionList(extensionsString);

			String *string = RNSTR(extensionsString.C_Str());
			string->ReplaceOccurrencesOfString(RNCSTR("*."), RNCSTR(""));
			Array *extensions = string->GetComponentsSeparatedByString(RNCSTR(";"));


			Config config({ Mesh::GetMetaClass(), Model::GetMetaClass() });
			config.SetExtensions(Set::WithArray(extensions));
			config.supportsBackgroundLoading = true;

			__assetLoader = new AssimpAssetLoader(config);

			AssetManager *manager = AssetManager::GetSharedInstance();
			manager->RegisterAssetLoader(__assetLoader);
		}
	}

	AssimpAssetLoader::AssimpAssetLoader(const Config &config) :
		AssetLoader(config)
	{}

	Asset *AssimpAssetLoader::Load(File *file, MetaClass *meta, Dictionary *settings)
	{
		bool loadLOD = true;
		bool recalculateNormals = false;
		float smoothNormalAngle = 20.0f;

		if(settings->GetObjectForKey(RNCSTR("recalculateNormals")))
		{
			Number *number = settings->GetObjectForKey<Number>(RNCSTR("recalculateNormals"));
			recalculateNormals = number->GetBoolValue();
		}

		if(settings->GetObjectForKey(RNCSTR("smoothNormalAngle")))
		{
			Number *number = settings->GetObjectForKey<Number>(RNCSTR("smoothNormalAngle"));
			smoothNormalAngle = number->GetFloatValue();
		}

		if(settings->GetObjectForKey(RNCSTR("loadLOD")))
		{
			Number *number = settings->GetObjectForKey<Number>(RNCSTR("loadLOD"));
			loadLOD = number->GetBoolValue();
		}



		Assimp::Importer importer;
		importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, smoothNormalAngle);

		const aiScene *scene = importer.ReadFile(file->GetPath()->GetUTF8String(), 0);
		if(recalculateNormals)
		{
			importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS | aiComponent_TANGENTS_AND_BITANGENTS);
			scene = importer.ApplyPostProcessing(aiProcess_RemoveComponent);
		}

		scene = importer.ApplyPostProcessing(aiProcessPreset_TargetRealtime_Quality | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes | aiProcess_FlipUVs);

		if(!scene)
			throw InconsistencyException(RNSTR(importer.GetErrorString()));

		if(meta->InheritsFromClass(Mesh::GetMetaClass()))
			return LoadAssimpMesh(scene, 0);

		// Load a full model
		Model *model = new Model();
		size_t stageIndex = 0;

		std::vector<float> lodFactors = Model::GetDefaultLODFactors();

		const String *path = file->GetPath();
		String *basePath = path->StringByDeletingLastPathComponent();

		LoadAssimpLODStage(basePath, scene, model->AddLODStage(lodFactors[0]));

		if(loadLOD)
		{
			String *extension = path->GetPathExtension();
			String *name = path->GetLastPathComponent()->StringByDeletingLastPathComponent();

			stageIndex ++;

			while(stageIndex < lodFactors.size())
			{
				String *lodPath = basePath->StringByAppendingPathComponent(RNSTR(name << "_lod" << stageIndex << "." << extension));

				try
				{
					const aiScene *scene = importer.ReadFile(lodPath->GetUTF8String(), 0);
					if(recalculateNormals)
					{
						importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS);
						scene = importer.ApplyPostProcessing(aiProcess_RemoveComponent);
					}
					scene = importer.ApplyPostProcessing(aiProcessPreset_TargetRealtime_Quality | aiProcess_OptimizeGraph | aiProcess_OptimizeMeshes | aiProcess_FlipUVs);

					if(!scene)
						throw InconsistencyException(RNSTR(importer.GetErrorString()));

					Model::LODStage *stage = model->AddLODStage(lodFactors[stageIndex]);
					LoadAssimpLODStage(basePath, scene, stage);
				}
				catch(Exception &)
				{
					break;
				}

				stageIndex ++;
			}
		}

		model->CalculateBoundingVolumes();
		return model->Autorelease();
	}

	void AssimpAssetLoader::LoadAssimpLODStage(const String *filepath, const aiScene *scene, Model::LODStage *stage)
	{
		for(size_t i = 0; i < scene->mNumMeshes; i++)
		{
			auto pair = LoadAssimpMeshGroup(filepath, scene, i);
			stage->AddMesh(pair.first, pair.second);
		}
	}

	std::pair<Mesh *, Material *> AssimpAssetLoader::LoadAssimpMeshGroup(const String *filepath, const aiScene *scene, size_t index)
	{
		Mesh *mesh = LoadAssimpMesh(scene, index);

		aiMesh *aimesh = scene->mMeshes[index];
		aiMaterial *aimaterial = scene->mMaterials[aimesh->mMaterialIndex];

		Renderer *renderer = Renderer::GetActiveRenderer();
		ShaderLookupRequest *lookup = new ShaderLookupRequest();
		MaterialDescriptor descriptor;

		if(aimaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			Texture *texture = LoadAssimpTexture(aimaterial, filepath, aiTextureType_DIFFUSE, 0);

			descriptor.AddTexture(texture);
			lookup->discard = texture->HasColorChannel(Texture::ColorChannel::Alpha);
		}

		descriptor.SetShaderProgram(renderer->GetDefaultShader(mesh, lookup));
		lookup->Release();

		return std::make_pair(mesh, Material::WithDescriptor(descriptor));
	}


	Mesh *AssimpAssetLoader::LoadAssimpMesh(const aiScene *scene, size_t index)
	{
		aiMesh *aimesh = scene->mMeshes[index];

		std::vector<Mesh::VertexAttribute> attributes;

		if(aimesh->HasPositions())
			attributes.emplace_back(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector3);

		if(aimesh->HasNormals())
			attributes.emplace_back(Mesh::VertexAttribute::Feature::Normals, PrimitiveType::Vector3);

		if(aimesh->HasTextureCoords(0))
			attributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector3);

		if(aimesh->HasTextureCoords(1))
			attributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords1, PrimitiveType::Vector3);

		if(aimesh->HasTangentsAndBitangents())
			attributes.emplace_back(Mesh::VertexAttribute::Feature::Tangents, PrimitiveType::Vector4);

		uint32 indexCount = 0;
		uint8 *indices = nullptr;

		if(aimesh->HasFaces())
		{
			PrimitiveType indicesType = PrimitiveType::Uint16;

			if(aimesh->mNumFaces * 3 > 65535)
				indicesType = PrimitiveType::Uint32;

			size_t size = (indicesType == PrimitiveType::Uint16) ? 2 : 4;
			size *= aimesh->mNumFaces * 3;

			indices = new uint8[size];

			for(uint32 face = 0; face < aimesh->mNumFaces; face ++)
			{
				if(aimesh->mFaces[face].mNumIndices != 3)
					continue;

				for(uint32 ind = 0; ind < aimesh->mFaces[face].mNumIndices; ind ++)
				{
					switch(indicesType)
					{
						case PrimitiveType::Uint16:
							((uint16 *)indices)[indexCount] = static_cast<uint16>(aimesh->mFaces[face].mIndices[ind]);
							break;
						case PrimitiveType::Uint32:
							((uint32 *)indices)[indexCount] = static_cast<uint32>(aimesh->mFaces[face].mIndices[ind]);
							break;
						default:
							break;
					}

					indexCount ++;
				}
			}

			attributes.emplace_back(Mesh::VertexAttribute::Feature::Indices, indicesType);
		}

		Mesh *mesh = new Mesh(attributes, aimesh->mNumVertices, indexCount);

		mesh->BeginChanges();

		if(aimesh->HasPositions())
			mesh->SetElementData(Mesh::VertexAttribute::Feature::Vertices, aimesh->mVertices);

		if(aimesh->HasTextureCoords(0))
			mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords0, aimesh->mTextureCoords[0]);

		if(aimesh->HasTextureCoords(1))
			mesh->SetElementData(Mesh::VertexAttribute::Feature::UVCoords1, aimesh->mTextureCoords[0]);

		if(aimesh->HasTangentsAndBitangents())
		{
			Mesh::Chunk chunk = mesh->GetChunk();
			Mesh::ElementIterator<Vector4> it = chunk.GetIterator<Vector4>(Mesh::VertexAttribute::Feature::Tangents);

			for(uint32 ind = 0; ind < aimesh->mNumVertices; ind++)
			{
				Vector4 tangent(aimesh->mTangents[ind].x, aimesh->mTangents[ind].y, aimesh->mTangents[ind].z, 0.0f);
				Vector3 normal(aimesh->mNormals[ind].x, aimesh->mNormals[ind].y, aimesh->mNormals[ind].z);
				Vector3 binormal(aimesh->mBitangents[ind].x, aimesh->mBitangents[ind].y, aimesh->mBitangents[ind].z);
				Vector3 inversebinormal = normal.GetCrossProduct(Vector3(tangent));

				if(binormal.GetDotProduct(inversebinormal) > 0.0f)
				{
					tangent.w = 1.0f;
				}
				else
				{
					tangent.w = -1.0f;
				}

				if(std::isnan(normal.x) || std::isnan(normal.y) || std::isnan(normal.z))
				{
					normal = RN::Vector3(0.0f, -1.0f, 0.0f);
					aimesh->mNormals[ind].x = 0.0f;
					aimesh->mNormals[ind].y = -1.0f;
					aimesh->mNormals[ind].z = 0.0f;
				}

				if(std::isnan(tangent.x) || std::isnan(tangent.y) || std::isnan(tangent.z))
				{
					Vector3 newnormal(normal);
					newnormal.x += 1.0f;
					tangent = Vector4(newnormal.GetCrossProduct(normal).Normalize(), 1.0f);
				}

				it->x = tangent.x;
				it->y = tangent.y;
				it->z = tangent.z;
				it->w = tangent.w;
				it ++;
			}
		}

		if(aimesh->HasNormals())
			mesh->SetElementData(Mesh::VertexAttribute::Feature::Normals, aimesh->mNormals);

		if(indices)
		{
			mesh->SetElementData(Mesh::VertexAttribute::Feature::Indices, indices);
			delete[] indices;
		}

		mesh->EndChanges();

		return mesh->Autorelease();
	}

	Texture *AssimpAssetLoader::LoadAssimpTexture(aiMaterial *material, const String *path, aiTextureType aitexturetype, uint8 index)
	{
		aiString aipath;
		material->GetTexture(aitexturetype, index, &aipath);

		String *filename = RNSTR(aipath.C_Str())->GetLastPathComponent();
		String *fullPath = path->StringByAppendingPathComponent(filename);

		String *normalized = FileCoordinator::GetSharedInstance()->GetNormalizedPathFromFullPath(fullPath);

		//bool linear = (aitexturetype == aiTextureType_NORMALS || aitexturetype == aiTextureType_HEIGHT || aitexturetype == aiTextureType_DISPLACEMENT);

		return Texture::WithName(normalized);
	}
}
