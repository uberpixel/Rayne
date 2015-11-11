//
//  RNAssimpAssetLoader.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
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


			Config config(Mesh::GetMetaClass());
			config.SetExtensions(Set::WithArray(extensions));
			config.supportsBackgroundLoading = true;

			RNDebug("Registered Assimp loader with extensions: " << extensions);

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
		bool guessMaterial = true;
		bool recalculateNormals = false;
		float smoothNormalAngle = 20.0f;

		if(settings->GetObjectForKey(RNCSTR("guessMaterial")))
		{
			Number *number = settings->GetObjectForKey<Number>(RNCSTR("guessMaterial"));
			guessMaterial = number->GetBoolValue();
		}

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

		return LoadAssimpMesh(scene, 0);
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
			attributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords0, PrimitiveType::Vector2);

		if(aimesh->HasTextureCoords(1))
			attributes.emplace_back(Mesh::VertexAttribute::Feature::UVCoords1, PrimitiveType::Vector2);

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

			for(int face = 0; face < aimesh->mNumFaces; face ++)
			{
				if(aimesh->mFaces[face].mNumIndices != 3)
					continue;

				for(int ind = 0; ind < aimesh->mFaces[face].mNumIndices; ind ++)
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

			for(int ind = 0; ind < aimesh->mNumVertices; ind++)
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

				if(isnan(normal.x) || isnan(normal.y) || isnan(normal.z))
				{
					normal = RN::Vector3(0.0f, -1.0f, 0.0f);
					aimesh->mNormals[ind].x = 0.0f;
					aimesh->mNormals[ind].y = -1.0f;
					aimesh->mNormals[ind].z = 0.0f;
				}

				if(isnan(tangent.x) || isnan(tangent.y) || isnan(tangent.z))
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
}
