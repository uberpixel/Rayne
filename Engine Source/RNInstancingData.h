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
	class InstancingLODStageData
	{
	public:
		friend class InstancingLODStage;
		
		InstancingLODStageData(Model *model, size_t stage, size_t index);
		
	private:
		Mesh *_mesh;
		Material *_material;
	};

	class InstancingLODStage
	{
	public:
		InstancingLODStage(Model *model, size_t stage);
		~InstancingLODStage();
		
		void RemoveIndex(size_t index);
		void AddIndex(size_t index);
		
		void UpdateData();
		void Render(RenderingObject &object, Renderer *renderer);
		
	private:
		Model *_model;
		bool _dirty;
		
		GLuint _texture;
		GLuint _buffer;
		
		std::vector<uint16> _indices;
		std::vector<InstancingLODStageData> _data;
	};
	
	class InstancingData
	{
	public:
		InstancingData(Model *model);
		~InstancingData();
		
		void Reserve(size_t count);
		void PivotMoved();
		
		void UpdateData();
		void Render(SceneNode *node, Renderer *renderer);
		
		void InsertEntity(Entity *entity);
		void RemoveEntity(Entity *entity);
		void UpdateEntity(Entity *entity);
		
		Model *GetModel() const { return _model; }
		
	private:
		void UpdateEntityLODStange(Entity *entity);
		void SortEntities();
		
		Model *_model;
		Entity *_pivot;
		
		GLuint _texture;
		GLuint _buffer;
		
		size_t _count;
		size_t _used;
		
		bool _dirty;
		
		std::vector<bool> _usage;
		std::vector<Matrix> _matrices;
		std::vector<InstancingLODStage *> _stages;
		
		std::unordered_set<Entity *> _entities;
		std::vector<Entity *> _sortedEntities;
	};
}

#endif /* __RAYNE_INSTANCINGDATA_H__ */
