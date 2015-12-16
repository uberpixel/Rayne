//
//  RNModel.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Debug/RNLogger.h"
#include "../Assets/RNAssetManager.h"
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
			LODStage *stage = _lodStages->GetObjectAtIndex<LODStage>(i);
			stage->_index = i;
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
