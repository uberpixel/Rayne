//
//  RNInstancingData.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		InstancingLODStage(Model *model, size_t stage);
		~InstancingLODStage();
		
		void RemoveIndex(size_t index);
		void AddIndex(size_t index);
		
		void UpdateData();
		void Render(RenderingObject &object, Renderer *renderer);
		
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
		InstancingData(Model *model);
		~InstancingData();
		
		void Reserve(size_t capacity);
		void PivotMoved();
		void SetPivot(Camera *pivot);
		void SetLimit(size_t limit);
		
		void UpdateData();
		void Render(SceneNode *node, Renderer *renderer);
		
		void InsertEntity(Entity *entity);
		void RemoveEntity(Entity *entity);
		void UpdateEntity(Entity *entity);
		
		Model *GetModel() const { return _model; }
		
	private:
		void UpdateEntityLODStange(Entity *entity, const Vector3 &position);
		void SortEntities();
		
		Model *_model;
		Camera *_pivot;
		
		GLuint _texture;
		GLuint _buffer;
		
		size_t _capacity;
		size_t _count;
		
		SpinLock _lock;
		bool _dirty;
		
		std::vector<size_t> _freeList;
		std::vector<Matrix> _matrices;
		std::vector<InstancingLODStage *> _stages;
		
		std::unordered_set<Entity *> _entities;
		std::vector<Entity *> _sortedEntities;
	};
}

#endif /* __RAYNE_INSTANCINGDATA_H__ */
