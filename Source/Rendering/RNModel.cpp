//
//  RNModel.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Debug/RNLogger.h"
#include "../Assets/RNAssetManager.h"
#include "../Rendering/RNRenderer.h"
#include "RNModel.h"

namespace RN
{
	RNDefineMeta(Model, Asset)
	RNDefineScopedMeta(Model, LODStage, Object)

	static std::vector<float> __defaultLODFactors({ 0.05f, 0.125f, 0.50f, 0.75f, 0.90f });

	Model::Model() :
		_lodStages(new Array())
	{}

	Model::Model(Mesh *mesh, Material *material) :
		Model()
	{
		auto &distances = GetDefaultLODFactors();
		LODStage *stage = AddLODStage(distances[0]);

		stage->AddMesh(mesh, material);
	}

	Model::Model(const Model *other) :
		_lodStages(other->_lodStages->Copy()),
		_boundingBox(other->_boundingBox),
		_boundingSphere(other->_boundingSphere)
	{}

	Model::~Model()
	{
		_lodStages->Release();
	}

	Model *Model::WithName(const String *name, const Dictionary *settings)
	{
		AssetManager *coordinator = AssetManager::GetSharedInstance();
		return coordinator->GetAssetWithName<Model>(name, settings);
	}

	Model *Model::WithSkycube(const MaterialDescriptor &materialDescriptor, const String *left, const String *front, const String *right, const String *back, const String *up, const String *down)
	{
		Model *sky = new Model();
		LODStage *stage = sky->AddLODStage(1.0f);

		Mesh *leftMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(90.0f, 90.0f, 0.0f)), RN::Vector3(-0.5f, 0.0f, 0.0f));
		MaterialDescriptor tempDescriptor = materialDescriptor;
		if(!tempDescriptor.vertexShader)
			tempDescriptor.vertexShader = Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, Shader::Options::WithMesh(leftMesh), Shader::Default::Sky);
		if(!tempDescriptor.fragmentShader)
			tempDescriptor.fragmentShader = Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, Shader::Options::WithMesh(leftMesh), Shader::Default::Sky);

		MaterialDescriptor leftMaterialDescriptor = tempDescriptor;
		if(left)
			leftMaterialDescriptor.AddTexture(Texture::WithName(left));
		Material *leftMaterial = Material::WithDescriptor(leftMaterialDescriptor);
		stage->AddMesh(leftMesh, leftMaterial);

		Mesh *frontMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(0.0f, 90.0f, 0.0f)), RN::Vector3(0.0f, 0.0f, -0.5f));
		MaterialDescriptor frontMaterialDescriptor = tempDescriptor;
		if(front)
			frontMaterialDescriptor.AddTexture(Texture::WithName(front));
		Material *frontMaterial = Material::WithDescriptor(frontMaterialDescriptor);
		stage->AddMesh(frontMesh, frontMaterial);

		Mesh *rightMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(-90.0f, 90.0f, 0.0f)), RN::Vector3(0.5f, 0.0f, 0.0f));
		MaterialDescriptor rightMaterialDescriptor = tempDescriptor;
		if(right)
			rightMaterialDescriptor.AddTexture(Texture::WithName(right));
		Material *rightMaterial = Material::WithDescriptor(rightMaterialDescriptor);
		stage->AddMesh(rightMesh, rightMaterial);

		Mesh *backMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(180.0f, 90.0f, 0.0f)), RN::Vector3(0.0f, 0.0f, 0.5f));
		MaterialDescriptor backMaterialDescriptor = tempDescriptor;
		if(back)
			backMaterialDescriptor.AddTexture(Texture::WithName(back));
		Material *backMaterial = Material::WithDescriptor(backMaterialDescriptor);
		stage->AddMesh(backMesh, backMaterial);

		Mesh *upMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(0.0f, 180.0f, 0.0f)), RN::Vector3(0.0f, 0.5f, 0.0f));
		MaterialDescriptor upMaterialDescriptor = tempDescriptor;
		if(up)
			upMaterialDescriptor.AddTexture(Texture::WithName(up));
		Material *upMaterial = Material::WithDescriptor(upMaterialDescriptor);
		stage->AddMesh(upMesh, upMaterial);

		Mesh *downMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(0.0f, 0.0f, 0.0f)), RN::Vector3(0.0f, -0.5f, 0.0f));
		MaterialDescriptor downMaterialDescriptor = tempDescriptor;
		if(down)
			downMaterialDescriptor.AddTexture(Texture::WithName(down));
		Material *downMaterial = Material::WithDescriptor(downMaterialDescriptor);
		stage->AddMesh(downMesh, downMaterial);


		return sky->Autorelease();
	}

	Model *Model::WithSkydome(const MaterialDescriptor &materialDescriptor, const String *texture)
	{
		Model *sky = new Model();
		LODStage *stage = sky->AddLODStage(1.0f);

		Mesh *domeMesh = Mesh::WithTexturedDome(0.5f, 80, 81);
		MaterialDescriptor domeMaterialDescriptor = materialDescriptor;
		if(!domeMaterialDescriptor.vertexShader)
			domeMaterialDescriptor.vertexShader = Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, Shader::Options::WithMesh(domeMesh), Shader::Default::Sky);
		if(!domeMaterialDescriptor.fragmentShader)
			domeMaterialDescriptor.fragmentShader = Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, Shader::Options::WithMesh(domeMesh), Shader::Default::Sky);

		if(texture)
			domeMaterialDescriptor.AddTexture(Texture::WithName(texture));
		Material *domeMaterial = Material::WithDescriptor(domeMaterialDescriptor);
		stage->AddMesh(domeMesh, domeMaterial);

		return sky->Autorelease();
	}


	Model::LODStage *Model::AddLODStage(float distance)
	{
		LODStage *stage = new LODStage(distance);

		_lodStages->AddObject(stage);
		_lodStages->Sort<LODStage>([](const LODStage *lhs, const LODStage *rhs) -> bool {
			return lhs->GetDistance() < rhs->GetDistance();
		});

		size_t count = _lodStages->GetCount();

		for(size_t i = 0; i < count; i ++)
		{
			LODStage *temp = _lodStages->GetObjectAtIndex<LODStage>(i);
			temp->_index = i;
		}

		return stage->Autorelease();
	}

	void Model::RemoveLODStage(size_t index)
	{
		_lodStages->RemoveObjectAtIndex(index);
	}


	Model::LODStage *Model::GetLODStage(size_t index) const
	{
		return _lodStages->GetObjectAtIndex<LODStage>(index);
	}
	Model::LODStage *Model::GetLODStageForDistance(float distance) const
	{
		if(_lodStages->GetCount() <= 1 || distance <= k::EpsilonFloat)
			return _lodStages->GetFirstObject<LODStage>();

		size_t result = 0;
		size_t count = _lodStages->GetCount();

		for(size_t i = 0; i < count; i ++)
		{
			LODStage *stage = _lodStages->GetObjectAtIndex<LODStage>(i);
			if(distance <= stage->GetDistance())
				return stage;

			result ++;
		}

		return _lodStages->GetLastObject<LODStage>();
	}

	void Model::CalculateBoundingVolumes()
	{
		_boundingBox = AABB(Vector3(0.0), Vector3(0.0));

		LODStage *stage = GetLODStage(0);

		for(LODStage::Group &group : stage->_groups)
		{
			Mesh *mesh = group._mesh;

			mesh->CalculateBoundingVolumes();
			_boundingBox += mesh->GetBoundingBox();
		}

		_boundingSphere = Sphere(_boundingBox);
	}

	const std::vector<float> &Model::GetDefaultLODFactors()
	{
		return __defaultLODFactors;
	}
	void Model::SetDefaultLODFactors(const std::vector<float> &factors)
	{
		if(factors.size() < 3)
		{
			static std::once_flag flag;
			std::call_once(flag, [] {
				RNDebug("There should be at least 3 LOD factors in the default LOD factors (this message will be logged once)");
			});
		}

		__defaultLODFactors = factors;
		std::sort(__defaultLODFactors.begin(), __defaultLODFactors.end());
	}
}
