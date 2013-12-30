//
//  RNInstancingData.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INSTANCINGDATA_H__
#define __RAYNE_INSTANCINGDATA_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNEntity.h"
#include "RNModel.h"
#include "RNRenderer.h"

namespace RN
{
	class InstancingLODStage
	{
	public:
		RNAPI InstancingLODStage(Model *model, size_t stage);
		RNAPI ~InstancingLODStage();
		
		RNAPI void RemoveIndex(size_t index);
		RNAPI void AddIndex(size_t index);
		
		RNAPI void UpdateData(bool dynamic);
		RNAPI void Render(RenderingObject &object, Renderer *renderer);
		
		bool IsEmpty() const { return _indices.empty(); }
		
	private:
		Model *_model;
		size_t _stage;
		
		bool _dirty;
		
		GLuint _texture;
		GLuint _buffer;
		
		std::vector<uint32> _indices;
	};
	
	class InstancingData
	{
	public:
		RNAPI InstancingData(Model *model);
		RNAPI ~InstancingData();
		
		RNAPI void Reserve(size_t capacity);
		RNAPI void PivotMoved();
		RNAPI void SetPivot(Camera *pivot);
		RNAPI void SetLimit(size_t limit);
		
		RNAPI void UpdateData();
		RNAPI void Render(SceneNode *node, Renderer *renderer);
		
		RNAPI void InsertEntity(Entity *entity);
		RNAPI void RemoveEntity(Entity *entity);
		RNAPI void UpdateEntity(Entity *entity);
		
		Model *GetModel() const { return _model; }
		
	private:
		void InsertEntityIntoLODStage(Entity *entity, size_t index);
		void UpdateEntityLODStage(Entity *entity, const Vector3 &position);
		
		void InsertEntityAtIndex(Entity *entity, size_t index);
		void SortEntities();
		
		Model *_model;
		Camera *_pivot;
		
		GLuint _texture;
		GLuint _buffer;
		
		size_t _capacity;
		size_t _count;
		size_t _limit;
		
		SpinLock _lock;		
		bool _dirty;
		bool _needsSort;
		bool _pivotMoved;
		
		std::vector<size_t> _freeList;
		std::vector<Matrix> _matrices;
		std::vector<InstancingLODStage *> _stages;
		
		std::unordered_set<Entity *> _entities;
		std::vector<Entity *> _sortedEntities;
	};
}

#endif /* __RAYNE_INSTANCINGDATA_H__ */
