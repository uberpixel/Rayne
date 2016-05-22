//
//  RNInstancingData.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INSTANCINGDATA_H_
#define __RAYNE_INSTANCINGDATA_H_

#include "../Base/RNBase.h"
#include "../Math/RNRandom.h"
#include "../Scene/RNEntity.h"
#include "../Data/RNSpatialMap.h"
#include "RNDynamicGPUBuffer.h"

namespace RN
{
	class InstancingLODStage
	{
	public:
		RNAPI InstancingLODStage(Model::LODStage *stage);
		RNAPI ~InstancingLODStage();

		RNAPI void RemoveIndex(size_t index);
		RNAPI void AddIndex(size_t index);

		RNAPI void UpdateData(bool dynamic);
		RNAPI void Render(const SceneNode *node, Renderer *renderer, GPUBuffer *buffer) const;
		RNAPI void Clear();

		bool IsEmpty() const { return _indices.empty(); }

	private:
		std::vector<Drawable *> _drawables;
		Model::LODStage *_stage;
		bool _dirty;

		std::unordered_set<uint32> _indices;
		DynamicGPUBuffer *_indicesBuffer;
		size_t _indicesSize;
	};

	struct __InstancingBucket;
	typedef std::shared_ptr<__InstancingBucket> InstancingBucket;

	class InstancingData
	{
	public:
		RNAPI InstancingData(Model *model);
		RNAPI ~InstancingData();

		RNAPI void Reserve(size_t capacity);
		RNAPI void PivotMoved();
		RNAPI void SetPivot(Camera *pivot);
		RNAPI void SetClipping(bool clipping, float distance);
		RNAPI void SetCellSize(float cellSize);
		RNAPI void SetThinningRange(bool thinning, float thinRange);

		RNAPI void UpdateData();
		RNAPI void Render(const SceneNode *node, Renderer *renderer);

		RNAPI void InsertEntity(Entity *entity);
		RNAPI void RemoveEntity(Entity *entity);
		RNAPI void UpdateEntity(Entity *entity);

		Model *GetModel() const { return _model; }

	private:
		void InsertEntityIntoLODStage(Entity *entity);
		void UpdateEntityLODStage(Entity *entity, const Vector3 &position);

		void ResignBucket(InstancingBucket &bucket);
		void ClipEntities();

		size_t GetIndex(Entity *entity);
		void ResignIndex(size_t index);

		Random::MersenneTwister _random;

		Model *_model;
		Camera *_pivot;
		bool _hasLODStages;

		DynamicGPUBuffer *_buffer;

		size_t _capacity;
		size_t _count;

		bool  _clipping;
		float _clipRange;

		Lockable _lock;

		bool _dirty;
		bool _dirtyIndices;
		bool _needsClipping;
		bool _pivotMoved;
		bool _needsRecreation;

		float _thinRange;
		bool _thinning;

		std::vector<size_t> _freeList;
		std::vector<Matrix> _matrices;
		std::vector<InstancingLODStage *> _stages;

		std::vector<Entity *> _entities;
		std::unordered_set<Entity *> _activeEntites;
		std::list<InstancingBucket> _activeBuckets;
		SpatialMap<InstancingBucket> _buckets;
	};
}


#endif /* __RAYNE_INSTANCINGDATA_H_ */
