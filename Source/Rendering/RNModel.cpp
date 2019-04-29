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
#include "RNSkeleton.h"

namespace RN
{
	RNDefineMeta(Model, Asset)
	RNDefineScopedMeta(Model, LODStage, Object)

	static std::vector<float> __defaultLODFactors({ 0.05f, 0.125f, 0.50f, 0.75f, 0.90f });

	Model::Model() :
		_lodStages(new Array()), _skeleton(nullptr)
	{}

	Model::Model(Mesh *mesh, Material *material) :
		Model()
	{
		auto &distances = GetDefaultLODFactors();
		LODStage *stage = AddLODStage(distances[0]);

		stage->AddMesh(mesh, material);
		CalculateBoundingVolumes();
	}

	Model::Model(const Model *other) :
		_boundingBox(other->_boundingBox),
		_boundingSphere(other->_boundingSphere)
	{
		_lodStages = new Array();
		other->_lodStages->Enumerate<LODStage>([&](LODStage *stage, size_t index, bool &stop)
		{
			_lodStages->AddObject(stage->Copy());
		});
		
		_skeleton = SafeRetain(other->_skeleton);
	}

	Model::~Model()
	{
		_lodStages->Release();
		SafeRelease(_skeleton);
	}

	Model *Model::WithName(const String *name, const Dictionary *settings)
	{
		AssetManager *coordinator = AssetManager::GetSharedInstance();
		return coordinator->GetAssetWithName<Model>(name, settings);
	}

	Model *Model::WithSkycube(const String *left, const String *front, const String *right, const String *back, const String *up, const String *down)
	{
		Model *sky = new Model();
		LODStage *stage = sky->AddLODStage(1.0f);

		Mesh *leftMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(90.0f, 90.0f, 0.0f)), RN::Vector3(-0.5f, 0.0f, 0.0f));
		Material *baseMaterial = Material::WithShaders(nullptr, nullptr);
		baseMaterial->SetDepthMode(DepthMode::LessOrEqual);

		Shader::Options *shaderOptions = Shader::Options::WithMesh(leftMesh);
		shaderOptions->AddDefine(RNCSTR("RN_SKY"), RNCSTR("1"));
		baseMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Default));
		baseMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Default));
		baseMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Depth), Shader::UsageHint::Depth);
		baseMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Depth), Shader::UsageHint::Depth);

		//TODO: Add sky depth shader

		Material *leftMaterial = Material::WithMaterial(baseMaterial);
		if(left)
			leftMaterial->AddTexture(Texture::WithName(left));
		stage->AddMesh(leftMesh, leftMaterial);

		Mesh *frontMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(0.0f, 90.0f, 0.0f)), RN::Vector3(0.0f, 0.0f, -0.5f));
		Material *frontMaterial = Material::WithMaterial(baseMaterial);
		if(front)
			frontMaterial->AddTexture(Texture::WithName(front));
		stage->AddMesh(frontMesh, frontMaterial);

		Mesh *rightMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(-90.0f, 90.0f, 0.0f)), RN::Vector3(0.5f, 0.0f, 0.0f));
		Material *rightMaterial = Material::WithMaterial(baseMaterial);
		if(right)
			rightMaterial->AddTexture(Texture::WithName(right));
		stage->AddMesh(rightMesh, rightMaterial);

		Mesh *backMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(180.0f, 90.0f, 0.0f)), RN::Vector3(0.0f, 0.0f, 0.5f));
		Material *backMaterial = Material::WithMaterial(baseMaterial);
		if(back)
			backMaterial->AddTexture(Texture::WithName(back));
		stage->AddMesh(backMesh, backMaterial);

		Mesh *upMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(0.0f, 180.0f, 0.0f)), RN::Vector3(0.0f, 0.5f, 0.0f));
		Material *upMaterial = Material::WithMaterial(baseMaterial);
		if(up)
			upMaterial->AddTexture(Texture::WithName(up));
		stage->AddMesh(upMesh, upMaterial);

		Mesh *downMesh = Mesh::WithTexturedPlane(Quaternion::WithEulerAngle(Vector3(0.0f, 0.0f, 0.0f)), RN::Vector3(0.0f, -0.5f, 0.0f));
		Material *downMaterial = Material::WithMaterial(baseMaterial);
		if(down)
			downMaterial->AddTexture(Texture::WithName(down));
		stage->AddMesh(downMesh, downMaterial);

		sky->CalculateBoundingVolumes();

		return sky->Autorelease();
	}

	Model *Model::WithSkydome(const String *texture)
	{
		Model *sky = new Model();
		LODStage *stage = sky->AddLODStage(1.0f);

		Mesh *domeMesh = Mesh::WithTexturedDome(0.5f, 80, 81);
		Material *domeMaterial = Material::WithShaders(nullptr, nullptr);
		domeMaterial->SetDepthMode(DepthMode::LessOrEqual);

		Shader::Options *shaderOptions = Shader::Options::WithMesh(domeMesh);
		shaderOptions->AddDefine(RNCSTR("RN_SKY"), RNCSTR("1"));

		domeMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Default));
		domeMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Default));
		domeMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Depth), Shader::UsageHint::Depth);
		domeMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Depth), Shader::UsageHint::Depth);

		if(texture)
			domeMaterial->AddTexture(Texture::WithName(texture));
		stage->AddMesh(domeMesh, domeMaterial);

		sky->CalculateBoundingVolumes();

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
	
	void Model::SetSkeleton(Skeleton *skeleton)
	{
		SafeRelease(_skeleton);
		_skeleton = SafeRetain(skeleton);
	}
	
	Skeleton *Model::GetSkeleton()
	{
		return _skeleton;
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
