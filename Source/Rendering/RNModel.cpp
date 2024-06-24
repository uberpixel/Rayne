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
#include "RNShadowVolume.h"

namespace RN
{
	RNDefineMeta(Model, Asset)
	RNDefineScopedMeta(Model, LODStage, Object)

	static std::vector<float> __defaultLODFactors({ 0.05f, 0.125f, 0.50f, 0.75f, 0.90f });

	Model::Model() :
		_skeleton(nullptr), _shadowVolume(nullptr),
#if RN_MODEL_LOD_DISABLED
		_lodStage(nullptr)
#else
		_lodStages(new Array())
#endif
	{}

	Model::Model(Mesh *mesh) : Model()
	{
		Material *material = Material::WithShaders(nullptr, nullptr);
		if(!RN::Renderer::IsHeadless())
		{
			Renderer *renderer = Renderer::GetActiveRenderer();
			Shader::Options *shaderOptions = Shader::Options::WithMesh(mesh);
			material->SetVertexShader(renderer->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Default), Shader::UsageHint::Default);
			material->SetFragmentShader(renderer->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Default), Shader::UsageHint::Default);
			material->SetVertexShader(renderer->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Depth), Shader::UsageHint::Depth);
			material->SetFragmentShader(renderer->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Depth), Shader::UsageHint::Depth);
			material->SetVertexShader(renderer->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Instancing), Shader::UsageHint::Instancing);
			material->SetFragmentShader(renderer->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Instancing), Shader::UsageHint::Instancing);
			material->SetVertexShader(renderer->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Multiview), Shader::UsageHint::Multiview);
			material->SetFragmentShader(renderer->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Multiview), Shader::UsageHint::Multiview);
			material->SetVertexShader(renderer->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::DepthMultiview), Shader::UsageHint::DepthMultiview);
			material->SetFragmentShader(renderer->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::DepthMultiview), Shader::UsageHint::DepthMultiview);
		}
		
		auto &distances = GetDefaultLODFactors();
		LODStage *stage = AddLODStage(distances[0]);

		stage->AddMesh(mesh, material);
		CalculateBoundingVolumes();
	}

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
#if RN_MODEL_LOD_DISABLED
		if(other->_lodStage) _lodStage = other->_lodStage->Copy();
		else _lodStage = nullptr;
#else
		_lodStages = new Array();
		other->_lodStages->Enumerate<LODStage>([&](LODStage *stage, size_t index, bool &stop)
		{
			_lodStages->AddObject(stage->Copy()->Autorelease());
		});
#endif
		
		_skeleton = SafeRetain(other->_skeleton);
	}

	Model::~Model()
	{
#if RN_MODEL_LOD_DISABLED
		SafeRelease(_lodStage);
#else
		_lodStages->Release();
#endif
		SafeRelease(_skeleton);
	}

	void Model::Warmup(Camera *camera)
	{
		Renderer *renderer = Renderer::GetActiveRenderer();
		RN_DEBUG_ASSERT(renderer, "No active renderer!");
		if(!renderer) return;
		
		for(size_t lodStage = 0; lodStage < GetLODStageCount(); lodStage += 1)
		{
			LODStage *stage = GetLODStage(lodStage);
			size_t count = stage->GetCount();
			for(size_t i = 0; i < count; i ++)
			{
				renderer->WarmupDrawable(stage->GetMeshAtIndex(i), stage->GetMaterialAtIndex(i), camera);
			}
		}
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
		baseMaterial->SetDepthMode(DepthMode::GreaterOrEqual);

		Shader::Options *shaderOptions = Shader::Options::WithMesh(leftMesh);
		shaderOptions->AddDefine("RN_SKY", "1");
		baseMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Default));
		baseMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Default));
		baseMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Depth), Shader::UsageHint::Depth);
		baseMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Depth), Shader::UsageHint::Depth);
		baseMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Instancing), Shader::UsageHint::Instancing);
		baseMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Instancing), Shader::UsageHint::Instancing);
		baseMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Multiview), Shader::UsageHint::Multiview);
		baseMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Multiview), Shader::UsageHint::Multiview);
		baseMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::DepthMultiview), Shader::UsageHint::DepthMultiview);
		baseMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::DepthMultiview), Shader::UsageHint::DepthMultiview);

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
		domeMaterial->SetDepthMode(DepthMode::GreaterOrEqual);

		Shader::Options *shaderOptions = Shader::Options::WithMesh(domeMesh);
		shaderOptions->AddDefine("RN_SKY", "1");

		domeMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Default));
		domeMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Default));
		domeMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Depth), Shader::UsageHint::Depth);
		domeMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Depth), Shader::UsageHint::Depth);
		domeMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Instancing), Shader::UsageHint::Instancing);
		domeMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Instancing), Shader::UsageHint::Instancing);
		domeMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::Multiview), Shader::UsageHint::Multiview);
		domeMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::Multiview), Shader::UsageHint::Multiview);
		domeMaterial->SetVertexShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Vertex, shaderOptions, Shader::UsageHint::DepthMultiview), Shader::UsageHint::DepthMultiview);
		domeMaterial->SetFragmentShader(Renderer::GetActiveRenderer()->GetDefaultShader(Shader::Type::Fragment, shaderOptions, Shader::UsageHint::DepthMultiview), Shader::UsageHint::DepthMultiview);

		if(texture)
			domeMaterial->AddTexture(Texture::WithName(texture));
		stage->AddMesh(domeMesh, domeMaterial);

		sky->CalculateBoundingVolumes();

		return sky->Autorelease();
	}


	Model::LODStage *Model::AddLODStage(float distance)
	{
		LODStage *stage = new LODStage(distance);

#if RN_MODEL_LOD_DISABLED
		RN_ASSERT(!_lodStage, "Only 1 LOD stage is supported, disable RN_MODEL_LOD_DISABLED to support multiple LOD stages!");
		_lodStage = stage;
		_lodStage->_index = 0;
#else
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
		stage->Autorelease();
#endif

		return stage;
	}

	void Model::RemoveLODStage(size_t index)
	{
#if RN_MODEL_LOD_DISABLED
		RN_ASSERT(index == 0, "Only 1 LOD stage is supported, disable RN_MODEL_LOD_DISABLED to support multiple LOD stages!");
		SafeRelease(_lodStage);
#else
		_lodStages->RemoveObjectAtIndex(index);
#endif
	}


	Model::LODStage *Model::GetLODStage(size_t index) const
	{
#if RN_MODEL_LOD_DISABLED
		return _lodStage;
#else
		return static_cast<Model::LODStage *>(_lodStages->GetObjectAtIndex(index));
#endif
	}
	Model::LODStage *Model::GetLODStageForDistance(float distance) const
	{
#if RN_MODEL_LOD_DISABLED
		return _lodStage;
#else
		size_t count = _lodStages->GetCount();
		if(count <= 1 || distance <= k::EpsilonFloat)
			return static_cast<Model::LODStage *>(_lodStages->GetObjectAtIndex(0));

		size_t result = 0;
		for(size_t i = 0; i < count; i ++)
		{
			LODStage *stage = static_cast<Model::LODStage *>(_lodStages->GetObjectAtIndex(i));
			if(distance <= stage->GetDistance())
				return stage;

			result ++;
		}

		return _lodStages->GetLastObject<LODStage>();
#endif
	}
	
	void Model::SetSkeleton(Skeleton *skeleton)
	{
		SafeRelease(_skeleton);
		_skeleton = SafeRetain(skeleton);
	}
	
	Skeleton *Model::GetSkeleton() const
	{
		return _skeleton;
	}
	
	void Model::SetShadowVolume(ShadowVolume *shadowVolume)
	{
		SafeRelease(_shadowVolume);
		_shadowVolume = SafeRetain(shadowVolume);
	}
	
	ShadowVolume *Model::GetShadowVolume() const
	{
		return _shadowVolume;
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
