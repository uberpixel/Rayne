//
//  RNModel.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNModel.h"
#include "RNFile.h"
#include "RNPathManager.h"
#include "RNSkeleton.h"
#include "RNResourcePool.h"

namespace RN
{
	RNDeclareMeta(Model)
	
	Model::Model()
	{
		LODGroup *group = new LODGroup(0.0f);
		_groups.push_back(group);
	}
	
	Model::Model(const std::string& tpath)
	{
		std::string path = PathManager::PathForName(tpath);
		std::string base = PathManager::Basepath(path);
		std::string name = PathManager::Basename(path);
		std::string extension = PathManager::Extension(path);
		
		LODGroup *group = new LODGroup(0.0f);
		ReadFileAtPath(path, group);
		
		_groups.push_back(group);
		
		int stage = 1;
		//float distance[] = { 0.125f, 0.25f, 0.50f, 0.75 };
		float distance[] = { 0.05f, 0.125f, 0.50f, 0.75 };
		
		while(stage < 5)
		{
			char buffer[32];
			sprintf(buffer, "_lod%i.", stage);
			
			std::string lodPath = PathManager::Join(base, (name + buffer + extension));
			
			if(PathManager::PathExists(lodPath))
			{
				group = new LODGroup(distance[stage - 1]);
				ReadFileAtPath(lodPath, group);
				
				_groups.push_back(group);
				stage ++;
			}
			else
			{
				break;
			}
		}
		
		CalculateBoundingBox();
	}
	
	Model::Model(Mesh *mesh, Material *material, const std::string& name)
	{
		LODGroup *group = new LODGroup(0.0f);
		_groups.push_back(group);
		
		AddMesh(mesh, material, 0, name);
	}
	
	Model::~Model()
	{
		for(LODGroup *group : _groups)
		{
			delete group;
		}
	}
	
	
	void Model::ReadFileAtPath(const std::string& path, LODGroup *group)
	{
		File *file = new File(path);
		uint32 magic = file->ReadUint32();
		
		if(magic == 0x15052290)
		{
			uint32 version = file->ReadUint8();
			
			switch(version)
			{
				case 1:
					ReadModelVersion1(file, group);
					break;
					
				default:
					break;
			}
		}
		
		file->Release();
	}
	
	
	Shader *Model::PickShaderForMaterialAndMesh(Material *material, Mesh *mesh)
	{
		static Shader *shader = 0;
		if(!shader)
		{
			shader = ResourcePool::SharedInstance()->ResourceWithName<Shader>(kRNResourceKeyTexture1Shader);
		}
		
		return shader->Retain();
	}
	
	Material *Model::PickMaterialForMesh(Mesh *mesh)
	{
		Material *material = new Material();
		material->SetShader(PickShaderForMaterialAndMesh(material, mesh));
		return material;
	}
	
	
	uint32 Model::AddLODStage(float distance)
	{
		LODGroup *group = new LODGroup(distance);
		_groups.push_back(group);
		
		std::sort(_groups.begin(), _groups.end(), [](LODGroup *groupA, LODGroup *groupB) {
			return groupA->lodDistance < groupB->lodDistance;
		});
		
		auto iterator = std::find(_groups.begin(), _groups.end(), group);
		return (uint32)std::distance(_groups.begin(), iterator);
	}
	
	void Model::RemoveLODStage(uint32 stage)
	{
		auto iterator = _groups.begin();
		std::advance(iterator, stage);
		
		delete *iterator;
		_groups.erase(iterator);
	}
	
	
	uint32 Model::LODStageForDistance(float distance) const
	{
		if(_groups.size() == 1 || distance <= kRNEpsilonFloat)
			return 0;
		
		uint32 result = 0;
		for(LODGroup *group : _groups)
		{
			if(distance <= group->lodDistance)
				break;
			
			result ++;
		}
		
		return result - 1;
	}
	
	
	void Model::AddMesh(Mesh *mesh, Material *material, uint32 lodStage, const std::string& name)
	{
		if(!material)
			material = PickMaterialForMesh(mesh);
		
		if(!material->Shader())
			material->SetShader(PickShaderForMaterialAndMesh(material, mesh));
			
		MeshGroup *group = new MeshGroup(mesh, material, name);
		_groups[lodStage]->groups.push_back(group);
		
		if(lodStage == 0)
		{
			_boundingBox += mesh->BoundingBox();
			_boundingSphere = Sphere(_boundingBox);
		}
	}
	
	void Model::RemoveMesh(Mesh *mesh, uint32 lodStage)
	{
	}
	
	void Model::CalculateBoundingBox()
	{
		_boundingBox = AABB();
		
		for(MeshGroup *group : _groups[0]->groups)
		{
			_boundingBox += group->mesh->BoundingBox();
		}
		
		_boundingSphere = Sphere(_boundingBox);
	}
	
	
	uint32 Model::Meshes(uint32 lodStage) const
	{
		return (uint32)_groups[lodStage]->groups.size();
	}
	
	Mesh *Model::MeshAtIndex(uint32 lodStage, uint32 index) const
	{
		return _groups[lodStage]->groups[index]->mesh;
	}
	
	Material *Model::MaterialAtIndex(uint32 lodStage, uint32 index) const
	{
		return _groups[lodStage]->groups[index]->material;
	}
	
	
	
	Model *Model::Empty()
	{
		Model *model = new Model();
		return model->Autorelease();
	}
	
	Model *Model::WithFile(const std::string& path)
	{
		Model *model = new Model(path);
		return model->Autorelease();
	}
	
	Model *Model::WithMesh(Mesh *mesh, Material *material, const std::string& name)
	{
		Model *model = new Model(mesh, material, name);
		return model->Autorelease();
	}
	
	Model *Model::WithSkyCube(const std::string& up, const std::string& down, const std::string& left, const std::string& right, const std::string& front, const std::string& back, const std::string& shader)
	{
		Shader *matShader = Shader::WithFile(shader);
		
		Material *skyDownMaterial = new Material(matShader);
		skyDownMaterial->AddTexture(Texture::WithFile(down, Texture::FormatRGB888, Texture::WrapModeClamp));
		skyDownMaterial->depthwrite = false;
		Mesh  *skyDownMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f));
		
		Material *skyUpMaterial = new Material(matShader);
		skyUpMaterial->AddTexture(Texture::WithFile(up, Texture::FormatRGB888, Texture::WrapModeClamp));
		skyUpMaterial->depthwrite = false;
		Mesh  *skyUpMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(0.0f, 180.0f, 0.0f));
		
		Material *skyLeftMaterial = new Material(matShader);
		skyLeftMaterial->AddTexture(Texture::WithFile(left, Texture::FormatRGB888, Texture::WrapModeClamp));
		skyLeftMaterial->depthwrite = false;
		Mesh  *skyLeftMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(-90.0f, 0.0f, 90.0f));
		
		Material *skyRightMaterial = new Material(matShader);
		skyRightMaterial->AddTexture(Texture::WithFile(right, Texture::FormatRGB888, Texture::WrapModeClamp));
		skyRightMaterial->depthwrite = false;
		Mesh  *skyRightMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(90.0f, 0.0f, 90.0f));
		
		Material *skyFrontMaterial = new Material(matShader);
		skyFrontMaterial->AddTexture(Texture::WithFile(front, Texture::FormatRGB888, Texture::WrapModeClamp));
		skyFrontMaterial->depthwrite = false;
		Mesh  *skyFrontMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(180.0f, 0.0f, 90.0f));
		
		Material *skyBackMaterial = new Material(matShader);
		skyBackMaterial->AddTexture(Texture::WithFile(back, Texture::FormatRGB888, Texture::WrapModeClamp));
		skyBackMaterial->depthwrite = false;
		Mesh  *skyBackMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(0.0f, 0.0f, 90.0f));
		
		Model *skyModel = Model::Empty();
		skyModel->AddMesh(skyDownMesh->Autorelease(), skyDownMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyUpMesh->Autorelease(), skyUpMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyLeftMesh->Autorelease(), skyLeftMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyRightMesh->Autorelease(), skyRightMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyFrontMesh->Autorelease(), skyFrontMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyBackMesh->Autorelease(), skyBackMaterial->Autorelease(), 0);
		
		return skyModel;
	}
	
	
	
	void Model::ReadModelVersion1(File *file, LODGroup *group)
	{
		//Get materials
		uint8 countmats = file->ReadUint8();
		Shader *shader = ResourcePool::SharedInstance()->ResourceWithName<Shader>(kRNResourceKeyTexture1Shader);
		
		std::vector<Material *> materials;
		
		for(uint8 i=0; i<countmats; i++)
		{
			Material *material = new Material(shader);
			file->ReadUint8();
			
			uint8 texcount = file->ReadUint8();
			for(uint8 n=0; n<texcount; n++)
			{
				std::string textureFile;
				file->ReadIntoString(textureFile, file->ReadUint16());
				
				std::string path = file->Path();
				Texture *texture = new Texture(PathManager::Join(path, textureFile), Texture::FormatRGBA8888);
				material->AddTexture(texture);
				texture->Release();
			}
			
			materials.push_back(material);
		}
		
		//Get meshes
		uint8 countmeshs = file->ReadUint8();
		for(uint8 i=0; i<countmeshs; i++)
		{
			file->ReadUint8();
			
			//MeshGroup group;
			Material *material = materials[file->ReadUint8()];
			
			unsigned short numverts = file->ReadUint16();
			unsigned char uvcount = file->ReadUint8();
			unsigned char datacount = file->ReadUint8();
			unsigned char hastangent = file->ReadUint8();
			unsigned char hasbones = file->ReadUint8();
			
			Array<MeshDescriptor> descriptors;
			
			MeshDescriptor meshDescriptor;
			meshDescriptor.feature = kMeshFeatureVertices;
			meshDescriptor.elementCount = numverts;
			meshDescriptor.elementSize = sizeof(Vector3);
			meshDescriptor.elementMember = 3;
			meshDescriptor.offset = 0;
			descriptors.AddObject(meshDescriptor);
			meshDescriptor.offset += sizeof(Vector3);
			
			meshDescriptor.feature = kMeshFeatureNormals;
			meshDescriptor.elementCount = numverts;
			meshDescriptor.elementSize = sizeof(Vector3);
			meshDescriptor.elementMember = 3;
			descriptors.AddObject(meshDescriptor);
			meshDescriptor.offset += sizeof(Vector3);
			
			meshDescriptor.feature = kMeshFeatureUVSet0;
			meshDescriptor.elementCount = numverts;
			meshDescriptor.elementSize = sizeof(Vector2);
			meshDescriptor.elementMember = 2;
			descriptors.AddObject(meshDescriptor);
			meshDescriptor.offset += sizeof(Vector2);
			
			if(hastangent == 1)
			{
				meshDescriptor.feature = kMeshFeatureTangents;
				meshDescriptor.elementCount = numverts;
				meshDescriptor.elementSize = sizeof(Vector4);
				meshDescriptor.elementMember = 4;
				descriptors.AddObject(meshDescriptor);
				meshDescriptor.offset += sizeof(Vector4);
			}
			if(uvcount > 1)
			{
				meshDescriptor.feature = kMeshFeatureUVSet1;
				meshDescriptor.elementCount = numverts;
				meshDescriptor.elementSize = sizeof(Vector2);
				meshDescriptor.elementMember = 2;
				descriptors.AddObject(meshDescriptor);
				meshDescriptor.offset += sizeof(Vector2);
			}
			if(datacount == 4)
			{
				meshDescriptor.feature = kMeshFeatureColor0;
				meshDescriptor.elementCount = numverts;
				meshDescriptor.elementSize = sizeof(Vector4);
				meshDescriptor.elementMember = 4;
				descriptors.AddObject(meshDescriptor);
				meshDescriptor.offset += sizeof(Vector4);
			}
			if(hasbones > 0)
			{
				meshDescriptor.feature = kMeshFeatureBoneWeights;
				meshDescriptor.elementCount = numverts;
				meshDescriptor.elementSize = sizeof(Vector4);
				meshDescriptor.elementMember = 4;
				descriptors.AddObject(meshDescriptor);
				meshDescriptor.offset += sizeof(Vector4);
				
				meshDescriptor.feature = kMeshFeatureBoneIndices;
				meshDescriptor.elementCount = numverts;
				meshDescriptor.elementSize = sizeof(Vector4);
				meshDescriptor.elementMember = 4;
				descriptors.AddObject(meshDescriptor);
				meshDescriptor.offset += sizeof(Vector4);
			}
			
			size_t size = meshDescriptor.offset * numverts;
			
			uint8 *vertexdata = new uint8[size];
			file->ReadIntoBuffer(vertexdata, meshDescriptor.offset * numverts);
			
			uint32 numindices = file->ReadUint32();
			uint8 sizeindices = file->ReadUint8();
			
			meshDescriptor.feature = kMeshFeatureIndices;
			meshDescriptor.elementCount = numindices;
			meshDescriptor.elementSize = sizeindices;
			meshDescriptor.elementMember = 1;
			meshDescriptor.offset = 0;
			descriptors.AddObject(meshDescriptor);
			
			Mesh *mesh = new Mesh(descriptors, vertexdata);
			void *data = mesh->MutableData<void>(kMeshFeatureIndices);
			
			file->ReadIntoBuffer(data, numindices*sizeindices);
			mesh->ReleaseData(kMeshFeatureIndices);
			
			delete[] vertexdata;
			
			MeshGroup *meshGroup = new MeshGroup(mesh->Autorelease(), material, "Unnamed");
			group->groups.push_back(meshGroup);
		}
		
		// Animations
		bool hasAnimations = file->ReadInt8();
		if(hasAnimations)
		{
			std::string animationFile;
			file->ReadIntoString(animationFile, file->ReadInt16());
		}
	}
}
