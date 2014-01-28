//
//  RNModel.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNModel.h"
#include "RNFile.h"
#include "RNPathManager.h"
#include "RNFileManager.h"
#include "RNSkeleton.h"
#include "RNResourceCoordinator.h"

namespace RN
{
	RNDeclareMeta(Model)
	
	Model::Model() :
		_skeleton(nullptr)
	{
		LODGroup *group = new LODGroup(0.0f);
		_groups.push_back(group);
	}
	
	Model::Model(Mesh *mesh, Material *material) :
		_skeleton(nullptr)
	{
		LODGroup *group = new LODGroup(0.0f);
		_groups.push_back(group);
		
		AddMesh(mesh, material, 0);
	}
	
	Model::~Model()
	{
		for(LODGroup *group : _groups)
		{
			delete group;
		}
		
		SafeRelease(_skeleton);
	}
	
	
	Shader *Model::PickShaderForMaterialAndMesh(Material *material, Mesh *mesh)
	{
		return ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyTexture1Shader, nullptr);
	}
	
	Material *Model::PickMaterialForMesh(Mesh *mesh)
	{
		Material *material = new Material();
		material->SetShader(PickShaderForMaterialAndMesh(material, mesh));
		return material;
	}
	
	
	size_t Model::AddLODStage(float distance)
	{
		LODGroup *group = new LODGroup(distance);
		_groups.push_back(group);
		
		std::sort(_groups.begin(), _groups.end(), [](const LODGroup *groupA, const LODGroup *groupB) {
			return groupA->lodDistance < groupB->lodDistance;
		});
		
		auto iterator = std::find(_groups.begin(), _groups.end(), group);
		return std::distance(_groups.begin(), iterator);
	}
	
	void Model::RemoveLODStage(size_t stage)
	{
		auto iterator = _groups.begin();
		std::advance(iterator, stage);
		
		delete *iterator;
		_groups.erase(iterator);
	}

	
	
	
	void Model::AddMesh(Mesh *mesh, Material *material, size_t lodStage)
	{
		if(!material)
			material = PickMaterialForMesh(mesh);
		
		if(!material->GetShader())
			material->SetShader(PickShaderForMaterialAndMesh(material, mesh));
			
		MeshGroup *group = new MeshGroup(mesh, material);
		_groups[lodStage]->groups.push_back(group);
		
		if(lodStage == 0)
		{
			_boundingBox += mesh->GetBoundingBox();
			_boundingSphere = Sphere(_boundingBox);
		}
	}
	
	void Model::CalculateBoundingVolumes()
	{
		_boundingBox = AABB();
		
		for(MeshGroup *group : _groups[0]->groups)
			_boundingBox += group->mesh->GetBoundingBox();
		
		_boundingSphere = Sphere(_boundingBox);
	}
	
	
	size_t Model::GetLODStageForDistance(float distance) const
	{
		if(_groups.size() == 1 || distance <= k::EpsilonFloat)
			return 0;
		
		size_t result = 0;
		for(LODGroup *group : _groups)
		{
			if(distance <= group->lodDistance)
				break;
			
			result ++;
		}
		
		return result - 1;
	}
	
	size_t Model::GetMeshCount(size_t lodStage) const
	{
		return _groups[lodStage]->groups.size();
	}
	
	Mesh *Model::GetMeshAtIndex(size_t lodStage, size_t index) const
	{
		return _groups[lodStage]->groups[index]->mesh;
	}
	
	Material *Model::GetMaterialAtIndex(size_t lodStage, size_t index) const
	{
		return _groups[lodStage]->groups[index]->material;
	}
	
	
	void Model::SetSkeleton(Skeleton *skeleton)
	{
		SafeRelease(_skeleton);
		_skeleton = SafeRetain(skeleton);
	}
	
	Skeleton *Model::GetSkeleton()
	{
		return _skeleton;
	}
	
	
	Model *Model::Empty()
	{
		Model *model = new Model();
		return model->Autorelease();
	}
	
	Model *Model::WithFile(const std::string& path, const Dictionary *settings)
	{
		Dictionary *finalsettings = new Dictionary();
		finalsettings->Autorelease();
		finalsettings->SetObjectForKey(Number::WithBool(true), RNCSTR("guessMaterial"));
		finalsettings->SetObjectForKey(Number::WithBool(true), RNCSTR("autoloadLOD"));
		
		if(settings)
			finalsettings->AddEntriesFromDictionary(settings);
		
		Model *model = ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Model>(RNSTR(path.c_str()), finalsettings);
		return model;
	}
	
	Model *Model::WithMesh(Mesh *mesh, Material *material)
	{
		Model *model = new Model(mesh, material);
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
		skyDownMaterial->SetDepthWrite(false);
		Mesh  *skyDownMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f));
		
		Material *skyUpMaterial = new Material(matShader);
		skyUpMaterial->AddTexture(Texture::WithFile(up, parameter));
		skyUpMaterial->SetDepthWrite(false);
		Mesh  *skyUpMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(180.0f, 180.0f, 0.0f));
		
		Material *skyLeftMaterial = new Material(matShader);
		skyLeftMaterial->AddTexture(Texture::WithFile(left, parameter));
		skyLeftMaterial->SetDepthWrite(false);
		Mesh  *skyLeftMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(-90.0f, 90.0f, 0.0f));
		
		Material *skyRightMaterial = new Material(matShader);
		skyRightMaterial->AddTexture(Texture::WithFile(right, parameter));
		skyRightMaterial->SetDepthWrite(false);
		Mesh  *skyRightMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(90.0f, 90.0f, 0.0f));
		
		Material *skyFrontMaterial = new Material(matShader);
		skyFrontMaterial->AddTexture(Texture::WithFile(front, parameter));
		skyFrontMaterial->SetDepthWrite(false);
		Mesh  *skyFrontMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(180.0f, 90.0f, 0.0f));
		
		Material *skyBackMaterial = new Material(matShader);
		skyBackMaterial->AddTexture(Texture::WithFile(back, parameter));
		skyBackMaterial->SetDepthWrite(false);
		Mesh  *skyBackMesh = Mesh::PlaneMesh(Vector3(1.0f, -1.0f, 1.0f), Vector3(0.0f, 90.0f, 0.0f));
		
		Model *skyModel = Model::Empty();
		skyModel->AddMesh(skyDownMesh, skyDownMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyUpMesh, skyUpMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyLeftMesh, skyLeftMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyRightMesh, skyRightMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyFrontMesh, skyFrontMaterial->Autorelease(), 0);
		skyModel->AddMesh(skyBackMesh, skyBackMaterial->Autorelease(), 0);
		
		return skyModel;
	}
}
