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
#include "RNFileManager.h"
#include "RNSkeleton.h"
#include "RNResourcePool.h"

namespace RN
{
	RNDeclareMeta(Model)
	
	Model::Model()
	: _guessMaterial(false)
	{
		LODGroup *group = new LODGroup(0.0f);
		_groups.push_back(group);
	}
	
	Model::Model(const std::string& tpath, bool guessmaterial)
	: _guessMaterial(guessmaterial)
	{
		std::string path = FileManager::GetSharedInstance()->GetFilePathWithName(tpath);
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
			std::stringstream stream;
			stream << name << "_lod" << stage << "." << extension;
			
			std::string lodPath = PathManager::Join(base, stream.str());
			
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
	: _guessMaterial(false)
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
				case 3:
					ReadModelVersion3(file, group);
					break;
					
				default:
					throw Exception(Exception::Type::GenericException, "Unsupported sgm File Format Version \"" + std::to_string(version) + "\"");
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
			shader = ResourcePool::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyTexture1Shader);
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

	
	
	
	void Model::AddMesh(Mesh *mesh, Material *material, uint32 lodStage, const std::string& name)
	{
		if(!material)
			material = PickMaterialForMesh(mesh);
		
		if(!material->GetShader())
			material->SetShader(PickShaderForMaterialAndMesh(material, mesh));
			
		MeshGroup *group = new MeshGroup(mesh, material, name);
		_groups[lodStage]->groups.push_back(group);
		
		if(lodStage == 0)
		{
			_boundingBox += mesh->GetBoundingBox();
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
			_boundingBox += group->mesh->GetBoundingBox();
		}
		
		_boundingSphere = Sphere(_boundingBox);
	}
	
	
	
	uint32 Model::GetLODStageForDistance(float distance) const
	{
		if(_groups.size() == 1 || distance <= k::EpsilonFloat)
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
	
	uint32 Model::GetMeshCount(uint32 lodStage) const
	{
		return (uint32)_groups[lodStage]->groups.size();
	}
	
	Mesh *Model::GetMeshAtIndex(uint32 lodStage, uint32 index) const
	{
		return _groups[lodStage]->groups[index]->mesh;
	}
	
	Material *Model::GetMaterialAtIndex(uint32 lodStage, uint32 index) const
	{
		return _groups[lodStage]->groups[index]->material;
	}
	
	
	
	Model *Model::Empty()
	{
		Model *model = new Model();
		return model->Autorelease();
	}
	
	Model *Model::WithFile(const std::string& path, bool guessmaterial)
	{
		Model *model = new Model(path, guessmaterial);
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
		
		Texture::Parameter parameter;
		parameter.format = Texture::Format::RGB888;
		parameter.wrapMode = Texture::WrapMode::Clamp;
		
		
		Material *skyDownMaterial = new Material(matShader);
		skyDownMaterial->AddTexture(Texture::WithFile(down, parameter));
		skyDownMaterial->depthwrite = false;
		Mesh  *skyDownMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f));
		
		Material *skyUpMaterial = new Material(matShader);
		skyUpMaterial->AddTexture(Texture::WithFile(up, parameter));
		skyUpMaterial->depthwrite = false;
		Mesh  *skyUpMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(180.0f, 180.0f, 0.0f));
		
		Material *skyLeftMaterial = new Material(matShader);
		skyLeftMaterial->AddTexture(Texture::WithFile(left, parameter));
		skyLeftMaterial->depthwrite = false;
		Mesh  *skyLeftMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(-90.0f, 90.0f, 0.0f));
		
		Material *skyRightMaterial = new Material(matShader);
		skyRightMaterial->AddTexture(Texture::WithFile(right, parameter));
		skyRightMaterial->depthwrite = false;
		Mesh  *skyRightMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(90.0f, 90.0f, 0.0f));
		
		Material *skyFrontMaterial = new Material(matShader);
		skyFrontMaterial->AddTexture(Texture::WithFile(front, parameter));
		skyFrontMaterial->depthwrite = false;
		Mesh  *skyFrontMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(180.0f, 90.0f, 0.0f));
		
		Material *skyBackMaterial = new Material(matShader);
		skyBackMaterial->AddTexture(Texture::WithFile(back, parameter));
		skyBackMaterial->depthwrite = false;
		Mesh  *skyBackMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(0.0f, 90.0f, 0.0f));
		
		Model *skyModel = Model::Empty();
		skyModel->AddMesh(skyDownMesh->Autorelease(), skyDownMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyUpMesh->Autorelease(), skyUpMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyLeftMesh->Autorelease(), skyLeftMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyRightMesh->Autorelease(), skyRightMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyFrontMesh->Autorelease(), skyFrontMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyBackMesh->Autorelease(), skyBackMaterial->Autorelease(), 0);
		
		return skyModel;
	}
	
	void Model::ReadModelVersion3(File *file, LODGroup *group)
	{
		//Get materials
		uint8 countmats = file->ReadUint8();
		Shader *shader = ResourcePool::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyTexture1Shader);
		
		std::vector<Material *> materials;
		
		for(uint8 i=0; i<countmats; i++)
		{
			Material *material = new Material(shader);
			__unused uint8 materialid = file->ReadUint8();
			
			uint8 uvcount = file->ReadUint8();
			for(uint8 u = 0; u < uvcount; u++)
			{
				uint8 texcount = file->ReadUint8();
				for(uint8 n=0; n<texcount; n++)
				{
					uint8 usagehint = file->ReadUint8();
					
					std::string textureFile;
					file->ReadIntoString(textureFile, file->ReadUint16());
				
					std::string path = file->GetPath();
					Texture *texture = Texture::WithFile(PathManager::Join(path, textureFile), (usagehint == 1));
					material->AddTexture(texture);
					
					if(_guessMaterial)
					{
						if(usagehint == 0)
						{
							//Diffuse texture
						}
						else if(usagehint == 1)
						{
							material->Define("RN_NORMALMAP");
						}
						else if(usagehint == 2)
						{
							material->Define("RN_SPECULARITY");
							material->Define("RN_SPECMAP");
						}
					}
				}
			}
			
			uint8 numcolors = file->ReadUint8();
			for(uint8 u = 0; u < numcolors; u++)
			{
				uint8 usagehint = file->ReadUint8();
				Color color(file->ReadFloat(), file->ReadFloat(), file->ReadFloat(), file->ReadFloat());
				if(usagehint == 0 && u == 0)
				{
					material->diffuse = color;
				}
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
			
			unsigned int numverts = file->ReadUint32();
			unsigned char uvcount = file->ReadUint8();
			unsigned char datacount = file->ReadUint8();
			unsigned char hastangent = file->ReadUint8();
			unsigned char hasbones = file->ReadUint8();
			
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
			
			if(uvcount > 0)
			{
				meshDescriptor = MeshDescriptor(kMeshFeatureUVSet0);
				meshDescriptor.elementSize = sizeof(Vector2);
				meshDescriptor.elementMember = 2;
			
				descriptors.push_back(meshDescriptor);
				size += meshDescriptor.elementSize;
			}
			
			if(hastangent == 1)
			{
				meshDescriptor = MeshDescriptor(kMeshFeatureTangents);
				meshDescriptor.elementSize = sizeof(Vector4);
				meshDescriptor.elementMember = 4;
				
				descriptors.push_back(meshDescriptor);
				size += meshDescriptor.elementSize;
			}
			if(uvcount > 1)
			{
				meshDescriptor = MeshDescriptor(kMeshFeatureUVSet1);
				meshDescriptor.elementSize = sizeof(Vector2);
				meshDescriptor.elementMember = 2;
				
				descriptors.push_back(meshDescriptor);
				size += meshDescriptor.elementSize;
			}
			if(datacount == 4)
			{
				meshDescriptor = MeshDescriptor(kMeshFeatureColor0);
				meshDescriptor.elementSize = sizeof(Vector4);
				meshDescriptor.elementMember = 4;
				
				descriptors.push_back(meshDescriptor);
				size += meshDescriptor.elementSize;
			}
			if(hasbones > 0)
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
			
			
			size *= numverts;
			
			uint8 *vertexData = new uint8[size];
			file->ReadIntoBuffer(vertexData, size);
			
			uint32 numindices = file->ReadUint32();
			uint8 sizeindices = file->ReadUint8();
			
			uint8 *indicesData = new uint8[numindices * sizeindices];
			file->ReadIntoBuffer(indicesData, numindices * sizeindices);
			
			meshDescriptor = MeshDescriptor(kMeshFeatureIndices);
			meshDescriptor.elementSize = sizeindices;
			meshDescriptor.elementMember = 1;
			descriptors.push_back(meshDescriptor);
			
			Mesh *mesh = new Mesh(descriptors, numverts, numindices, std::make_pair(vertexData, indicesData));
			mesh->CalculateBoundingVolumes();
			
			delete [] vertexData;
			delete [] indicesData;
			
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
