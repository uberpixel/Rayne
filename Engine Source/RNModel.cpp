//
//  RNModel.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNModel.h"
#include "RNFile.h"
#include "RNSkeleton.h"

namespace RN
{
	Model::Model()
	{
	}
	
	Model::Model(const std::string& path)
	{
		File *file = new File(path);
		
		uint32 magic = file->ReadUint32();
		
		if(magic == 352658064)
		{
			uint32 version = file->ReadUint8();
			
			switch(version)
			{
				case 1:
					ReadModelVersion1(file);
					break;
					
				default:
					break;
			}
		}
		
		file->Release();
	}
	
	Model::Model(Mesh *mesh, Material *material, const std::string& name)
	{
		MeshGroup group;
		group.mesh = mesh->Retain<Mesh>();
		group.material = material;
		group.name = name;
		
		_groups.push_back(group);
		_materials.AddObject(material);
		
		if(!material->Shader())
		{
			// Pick a shader automatically
			Shader *shader = 0;
			
			uint32 textures = material->TextureCount();
			bool hasTexcoord0 = mesh->LODStage(0)->SupportsFeature(kMeshFeatureUVSet0);
			bool hasColor0 = mesh->LODStage(0)->SupportsFeature(kMeshFeatureColor0);
			bool hasColor1 = mesh->LODStage(0)->SupportsFeature(kMeshFeatureColor1);
			
			if(hasTexcoord0 && textures > 0)
			{
				if(textures == 1)
				{
					shader = new Shader("shader/rn_Texture1");
				}
				if(textures == 2)
				{
					shader = new Shader("shader/rn_Texture2");
				}
			}
			else if(hasColor0)
			{
				if(hasColor1)
				{
					shader = new Shader("shader/rn_Color2");
				}
				else
				{
					shader = new Shader("shader/rn_Color1");
				}
			}
			
			material->SetShader(shader);
			shader->Release();
		}
	}
	
	Model::~Model()
	{
	}
	
	Model *Model::WithFile(const std::string& path)
	{
		Model *model = new Model(path);
		return model->Autorelease<Model>();
	}
	
	Model *Model::WithSkyCube(std::string up, std::string down, std::string left, std::string right, std::string front, std::string back, std::string shader)
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
		skyModel->AddMesh(skyDownMesh->Autorelease<Mesh>(), skyDownMaterial->Autorelease<Material>());
		skyModel->AddMesh(skyUpMesh->Autorelease<Mesh>(), skyUpMaterial->Autorelease<Material>());
		skyModel->AddMesh(skyLeftMesh->Autorelease<Mesh>(), skyLeftMaterial->Autorelease<Material>());
		skyModel->AddMesh(skyRightMesh->Autorelease<Mesh>(), skyRightMaterial->Autorelease<Material>());
		skyModel->AddMesh(skyFrontMesh->Autorelease<Mesh>(), skyFrontMaterial->Autorelease<Material>());
		skyModel->AddMesh(skyBackMesh->Autorelease<Mesh>(), skyBackMaterial->Autorelease<Material>());
		
		return skyModel;
	}
	
	Model *Model::WithMesh(Mesh *mesh, Material *material, const std::string& name)
	{
		Model *model = new Model(mesh, material, name);
		return model->Autorelease<Model>();
	}
	
	Model *Model::Empty()
	{
		Model *model = new Model();
		return model->Autorelease<Model>();
	}
	
	void Model::AddMesh(Mesh *mesh, Material *material, const std::string& name)
	{
		MeshGroup group;
		group.mesh = mesh->Retain<Mesh>();
		group.material = material;
		group.name = name;
		
		_groups.push_back(group);
		_materials.AddObject(material);
	}
	
	uint32 Model::Meshes() const
	{
		return (uint32)_groups.size();
	}
	
	uint32 Model::Materials() const
	{
		return (uint32)_materials.Count();
	}
	
	Mesh *Model::MeshAtIndex(uint32 index) const
	{
		return _groups[index].mesh;
	}
	
	Material *Model::MaterialForMesh(const Mesh *mesh) const
	{
		for(auto i=_groups.begin(); i!=_groups.end(); i++)
		{			
			if(i->mesh == mesh)
				return i->material;
		}
		
		return 0;
	}
	
	void Model::ReadModelVersion1(File *file)
	{
		//Get materials
		unsigned char countmats = file->ReadUint8();
		Shader *shader = new Shader("shader/rn_Texture1");
		Material *material;
		for(unsigned int i = 0; i < countmats; i++)
		{
			material = new Material(shader);
			file->ReadUint8();
			unsigned char texcount = file->ReadUint8();
			for(unsigned int n = 0; n < texcount; n++)
			{
				unsigned short lentexfilename = file->ReadUint16();
				char *texfilename = new char[lentexfilename];
				file->ReadIntoBuffer(texfilename, lentexfilename);
				
				std::string path = file->Path();
				Texture *texture = new Texture(path+"/"+std::string(texfilename), Texture::FormatRGBA8888);
				material->AddTexture(texture);
				texture->Release();
				
				delete[] texfilename;
			}
			
			_materials.AddObject(material);
		}
		
		//Get meshes
		unsigned char countmeshs = file->ReadUint8();
		for(int i = 0; i < countmeshs; i++)
		{
			MeshGroup group;
			
			file->ReadUint8();
			material = _materials.ObjectAtIndex(file->ReadUint8());
			
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
			
			float *vertexdata = new float[meshDescriptor.offset/sizeof(float)*numverts];
			file->ReadIntoBuffer(vertexdata, meshDescriptor.offset*numverts);
			
			uint32 numindices = file->ReadUint32();
			uint8 sizeindices = file->ReadUint8();
			meshDescriptor.feature = kMeshFeatureIndices;
			meshDescriptor.elementCount = numindices;
			meshDescriptor.elementSize = sizeindices;
			meshDescriptor.elementMember = 1;
			meshDescriptor.offset = 0;
			descriptors.AddObject(meshDescriptor);
			
			Mesh *mesh = new Mesh();
			MeshLODStage *stage = mesh->AddLODStage(descriptors, vertexdata);
			delete[] vertexdata;
			void *indices = stage->Data<void>(kMeshFeatureIndices);
			
			file->ReadIntoBuffer(indices, numindices*sizeindices);
			mesh->UpdateMesh();
			
			group.mesh = mesh;
			group.material = material;
			_groups.push_back(group);
		}
		
		//Animations
		unsigned char hasanimations = file->ReadInt8();
		if(hasanimations)
		{
			unsigned short lenanimfilename = file->ReadInt16();
			char *animfilename = new char[lenanimfilename];
			file->ReadIntoBuffer(animfilename, lenanimfilename*sizeof(char));
			delete[] animfilename;
		}
	}
}
