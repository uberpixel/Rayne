//
//  RNModel.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNModel.h"
#include "RNFile.h"

namespace RN
{
	Model::Model(const std::string& path)
	{
		File *file = new File(path);
		
//		uint32 magic = file->ReadUint32();
		uint32 version = file->ReadUint8();
		
//		if(magic == 0x8A7FEED1)
//		{
			switch(version)
			{
				case 1:
					ReadModelVersion1(file);
					break;
					
				default:
					break;
			}
//		}
		
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
	
	Model *Model::WithMesh(Mesh *mesh, Material *material, const std::string& name)
	{
		Model *model = new Model(mesh, material, name);
		return model->Autorelease<Model>();
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
			unsigned char bonecount = file->ReadUint8();
			
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
			if(bonecount > 0)
			{
/*				meshDescriptor.feature = kMeshFeatureBoneWeights;
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
				meshDescriptor.offset += sizeof(Vector4);*/
				
				unsigned short *bonemapping = new unsigned short[bonecount];
				file->ReadIntoBuffer(bonemapping, 2*bonecount);
				delete[] bonemapping;
			}
			
			float *vertexdata = new float[meshDescriptor.offset/sizeof(float)*numverts];
			file->ReadIntoBuffer(vertexdata, meshDescriptor.offset*numverts);
			
			uint32 numindices = file->ReadUint32();
			meshDescriptor.feature = kMeshFeatureIndices;
			meshDescriptor.elementCount = numindices;
			meshDescriptor.elementSize = sizeof(uint16);
			meshDescriptor.elementMember = 1;
			meshDescriptor.offset = 0;
			descriptors.AddObject(meshDescriptor);
			
			Mesh *mesh = new Mesh();
			MeshLODStage *stage = mesh->AddLODStage(descriptors, vertexdata);
			delete[] vertexdata;
			uint16 *indices = stage->Data<uint16>(kMeshFeatureIndices);
			
			file->ReadIntoBuffer(indices, numindices*sizeof(uint16));
			mesh->UpdateMesh();
			
			group.mesh = mesh;
			group.material = material;
			_groups.push_back(group);
		}
		
		//Animations
/*		unsigned char hasanimations = file->ReadInt8();
		if(hasanimations)
		{
			unsigned short lenanimfilename = file->ReadInt16();
			char *animfilename = new char[lenanimfilename];
			file->ReadIntoBuffer(animfilename, lenanimfilename*sizeof(char));
			printf("Animation filename: %s\n", animfilename);
			//meshesstd::string(animfilename);
		}*/
		
		
		
		
//		uint32 materialCount = file->ReadUint32();
//		uint32 groups = file->ReadUint32();
		
/*		ReadMaterials(file, materialCount);
		ReadGroups(file, groups);*/
	}
	
/*	void Model::ReadMaterials(File *file, uint32 count)
	{
		while(count)
		{
			Material *material = new Material(0);
			
			// Culling
			uint8 culling = file->ReadUint8();
			
			material->culling = (culling & 0x1);
			material->cullmode = ((culling >> 1) & 0x1) ? GL_CCW : GL_CW;
			
			// Alpha test
			uint8 alphaTest = file->ReadUint8();
			
			material->alphatest = (alphaTest);
			
			// Diffuse color
			uint32 diffuse = file->ReadUint32();
			uint8 diffuseR = (diffuse >> 24) & 0xff;
			uint8 diffuseG = (diffuse >> 16) & 0xff;
			uint8 diffuseB = (diffuse >> 8)  & 0xff;
			uint8 diffuseA = (diffuse >> 0)  & 0xff;
			
			material->diffuse = Color((diffuseR / 255.0f), (diffuseG / 255.0f), (diffuseB / 255.0f), (diffuseA / 255.0f));
			
			// Grab the textures
			uint32 textures = file->ReadUint32();
			while(textures)
			{
				uint32 length = file->ReadUint32();
				char *name = new char[length + 1];
				
				file->ReadIntoBuffer(name, length);
				name[length] = '\0';
				
				Texture *texture = new Texture(std::string(name), Texture::FormatRGBA8888);
				material->AddTexture(texture);
				texture->Release();
				
				delete [] name;
				
				textures --;
			}
			
			_materials->AddObject(material);
			material->Release();
			
			count --;
		}
	}
	
	void Model::ReadGroups(File *file, uint32 count)
	{
		while(count)
		{
			MeshGroup group;
			
			Material *material = (Material *)_materials->ObjectAtIndex(file->ReadUint32());
			Mesh *mesh = new Mesh();
			
			uint32 features = file->ReadUint32();
			
			// Read the name
			{
				uint32 length = file->ReadUint32();
				char *name = new char[length + 1];
				
				file->ReadIntoBuffer(name, length);
				name[length] = '\0';
				
				group.name = std::string(name);
				delete [] name;
			}
			
			// Read all features of the mesh
			std::vector<FeatureDescriptor> descriptors;
			Array<MeshDescriptor> meshDescriptors;
			
			while(features)
			{
				FeatureDescriptor descriptor = ReadFeature(file);
				
				descriptors.push_back(descriptor);
				meshDescriptors.AddObject(descriptor.meshDescriptor);
				
				features --;
			}
			
			// Create the mesh
			MeshLODStage *stage = mesh->AddLODStage(meshDescriptors);
			
			for(auto i=descriptors.begin(); i!=descriptors.end(); i++)
			{
				FeatureDescriptor descriptor = *i;
				uint8 *target = stage->Data<uint8>(descriptor.meshDescriptor.feature);
				uint8 *source = descriptor.data;
				
				std::copy(source, source + descriptor.size, target);
			}
			
			mesh->UpdateMesh();
			
			group.material = material;
			group.mesh = mesh;
			
			_groups.push_back(group);
			count --;
		}
	}
	
	Model::FeatureDescriptor Model::ReadFeature(File *file)
	{
		MeshDescriptor meshDescriptor;
		meshDescriptor.feature = (MeshFeature)file->ReadUint32();
		meshDescriptor.elementCount = file->ReadUint32();
		meshDescriptor.elementSize = file->ReadUint32();
		meshDescriptor.elementMember = file->ReadUint32();
		
		uint32 size = file->ReadUint32();
		uint8 *data = new uint8[size];
		
		file->ReadIntoBuffer(data, size);
		
		FeatureDescriptor descriptor;
		descriptor.meshDescriptor = meshDescriptor;
		descriptor.data = data;
		descriptor.size = size;
		
		return descriptor;
	}*/
}
