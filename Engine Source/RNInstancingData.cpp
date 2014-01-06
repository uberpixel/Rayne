//
//  RNInstancingData.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInstancingData.h"
#include "RNLogging.h"

#define kRNInstancingNodeAssociatedIndexKey    "kRNInstancingNodeAssociatedIndexKey"
#define kRNInstancingNodeAssociatedLODStageKey "kRNInstancingNodeAssociatedLODStageKey"

namespace RN
{
	InstancingLODStage::InstancingLODStage(Model *model, size_t stage) :
		_model(model),
		_stage(stage),
		_dirty(true)
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
			gl::GenTextures(1, &_texture);
			gl::GenBuffers(1, &_buffer);
			
			gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
			gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
			gl::TexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, _buffer);
			
			gl::BindTexture(GL_TEXTURE_BUFFER, 0);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		});
	}
	
	InstancingLODStage::~InstancingLODStage()
	{
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			gl::DeleteTextures(1, &_texture);
			gl::DeleteBuffers(1, &_buffer);
		}, true);
	}
	
	void InstancingLODStage::RemoveIndex(size_t index)
	{
		auto iterator = std::find(_indices.begin(), _indices.end(), static_cast<uint32>(index));
		
		if(iterator != _indices.end())
		{
			_indices.erase(iterator);
			_dirty = true;
		}
	}
	
	void InstancingLODStage::AddIndex(size_t index)
	{
		_indices.push_back(static_cast<uint32>(index));
		_dirty = true;
	}
	
	void InstancingLODStage::UpdateData(bool dynamic)
	{
		if(_dirty && !_indices.empty())
		{
			GLenum mode = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
			
			OpenGLQueue::GetSharedInstance()->SubmitCommand([this, mode] {
				gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
				gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
				gl::BufferData(GL_TEXTURE_BUFFER, _indices.size() * sizeof(uint32), _indices.data(), mode);
				gl::BindTexture(GL_TEXTURE_BUFFER, 0);
				gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
			});
			
			_dirty = false;
		}
	}
	
	void InstancingLODStage::Render(RenderingObject &object, Renderer *renderer)
	{
		object.count = static_cast<uint32>(_indices.size());
		object.instancingIndices = _texture;
		
		size_t count = _model->GetMeshCount(_stage);
		
		for(size_t i = 0; i < count; i ++)
		{
			object.mesh     = _model->GetMeshAtIndex(_stage, i);
			object.material = _model->GetMaterialAtIndex(_stage, i);
			
			renderer->RenderObject(object);
		}
	}
	
	
	
	
	InstancingData::InstancingData(Model *model) :
		_capacity(0),
		_count(0),
		_limit(0)
	{
		_model = model->Retain();
		_pivot = nullptr;
		
		size_t count = _model->GetLODStageCount();
		
		for(size_t i = 0; i < count; i ++)
			_stages.push_back(new InstancingLODStage(_model, i));
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
			gl::GenTextures(1, &_texture);
			gl::GenBuffers(1, &_buffer);
			
			gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
			gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
			gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _buffer);
			
			gl::BindTexture(GL_TEXTURE_BUFFER, 0);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		});
		
		Reserve(50);
	}
	
	InstancingData::~InstancingData()
	{
		for(InstancingLODStage *stage : _stages)
			delete stage;
		
		OpenGLQueue::GetSharedInstance()->SubmitCommand([&] {
			gl::DeleteTextures(1, &_texture);
			gl::DeleteBuffers(1, &_buffer);
		}, true);
	}
	
	
	void InstancingData::SetPivot(Camera *pivot)
	{
		_pivot = pivot; // Retained by the InstancingNode
	}
	
	void InstancingData::SetLimit(size_t limit)
	{
		Reserve(limit);
		
		_limit     = limit;
		_needsSort = true;
	}
	
	void InstancingData::PivotMoved()
	{
		_pivotMoved = true;
		_needsSort  = true;
	}
	
	
	
	void InstancingData::UpdateData()
	{
		if(_pivot)
		{
			if(_needsSort && _limit > 0)
			{
				SortEntities();
				_needsSort = false;
			}
			
			if(_pivotMoved)
			{
				Vector3 position = _pivot->GetWorldPosition();
				size_t count = (_limit > 0) ? std::min(_sortedEntities.size(), _limit) : _sortedEntities.size();
				
				for(size_t i = 0; i < count; i ++)
					UpdateEntityLODStage(_sortedEntities[i], position);
				
				_pivotMoved = false;
			}
		}
		
		if(_dirty)
		{
			OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
			
				gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
				gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
				gl::BufferData(GL_TEXTURE_BUFFER, static_cast<GLsizei>(_matrices.size() * sizeof(Matrix)), _matrices.data(), GL_STATIC_DRAW);
				gl::BindTexture(GL_TEXTURE_BUFFER, 0);
				gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
				
			});
			
			_dirty = false;
		}
		
		for(InstancingLODStage *stage : _stages)
			stage->UpdateData((_pivot != nullptr));
	}
	
	void InstancingData::SortEntities()
	{
		if(_sortedEntities.size() < _limit)
			return;
		
		Vector3 position = _pivot->GetWorldPosition();
		
		std::sort(_sortedEntities.begin(), _sortedEntities.end(), [&](const Entity *left, const Entity *right) {
			float dist1 = left->GetWorldPosition().Distance(position);
			float dist2 = right->GetWorldPosition().Distance(position);
			
			return dist1 < dist2;
		});
		
		// Resign clipped entities
		for(size_t i = _limit; i < _sortedEntities.size(); i ++)
		{
			Entity *entity = _sortedEntities[i];
			Number *stage  = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedLODStageKey));
			
			if(stage)
			{
				size_t tstage = stage->GetUint32Value();
				size_t index  = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedIndexKey))->GetUint32Value();
				
				_stages[tstage]->RemoveIndex(index);
				
				entity->RemoveAssociatedOject(kRNInstancingNodeAssociatedLODStageKey);
			}
		}
		
		// Assign missing indices
		for(size_t i = 0; i < _limit; i ++)
		{
			Entity *entity = _sortedEntities[i];
			Number *stage  = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedLODStageKey));
			
			if(!stage)
			{
				size_t index  = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedIndexKey))->GetUint32Value();
				InsertEntityIntoLODStage(entity, index);
			}
		}
	}
	
	
	void InstancingData::Render(SceneNode *node, Renderer *renderer)
	{
		RenderingObject object(RenderingObject::Type::Instanced);
		node->FillRenderingObject(object);
		
		object.instancingData = _texture;
		
		size_t count = _stages.size();
		
		for(size_t i = 0; i < count; i ++)
		{
			if(_stages[i]->IsEmpty())
				continue;
			
			_stages[i]->Render(object, renderer);
		}
	}
		
	
	void InstancingData::Reserve(size_t capacity)
	{
		if(_capacity >= capacity)
			return;
		
		_matrices.resize(capacity * 2);
		_freeList.reserve(_freeList.size() + (capacity - _capacity));
		
		for(size_t i = 0; i < (capacity - _capacity); i ++)
		{
			_freeList.push_back(_capacity + i);
		}
		
		_capacity = capacity;
		_dirty    = true;
	}
	
	
	
	void InstancingData::InsertEntityAtIndex(Entity *entity, size_t index)
	{
		_count ++;
		
		Number *indexNum = new Number(static_cast<uint32>(index));
		entity->SetAssociatedObject(kRNInstancingNodeAssociatedIndexKey, indexNum, Object::MemoryPolicy::Retain);
		indexNum->Release();
		
		_matrices[((index * 2) + 0)] = entity->GetWorldTransform();
		_matrices[((index * 2) + 1)] = entity->GetWorldTransform().GetInverse();
		
		if(_count < _limit || _limit == 0)
			InsertEntityIntoLODStage(entity, index);
	}
	
	void InstancingData::InsertEntity(Entity *entity)
	{
		_lock.Lock();
		
		if(_entities.find(entity) == _entities.end())
		{
			_needsSort = true;
			
			_entities.insert(entity);
			_sortedEntities.push_back(entity);
			
			if(_count >= _capacity)
				Reserve(_capacity * 1.5f);
			
			size_t index = _freeList.back();
			_freeList.pop_back();
			
			InsertEntityAtIndex(entity, index);
		}
		
		_lock.Unlock();
	}
	
	void InstancingData::RemoveEntity(Entity *entity)
	{
		_lock.Lock();
		
		if(_entities.find(entity) == _entities.end())
		{
			_lock.Unlock();
			return;
		}
		
		size_t index = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedIndexKey))->GetUint32Value();
		uint32 stage = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedLODStageKey))->GetUint32Value();
		
		entity->RemoveAssociatedOject(kRNInstancingNodeAssociatedIndexKey);
		entity->RemoveAssociatedOject(kRNInstancingNodeAssociatedLODStageKey);
		
		_stages[stage]->RemoveIndex(index);
		
		_entities.erase(entity);
		_sortedEntities.erase(std::find(_sortedEntities.begin(), _sortedEntities.end(), entity));
		
		_freeList.push_back(index);
		_count --;
		
		_lock.Unlock();
	}
	
	
	
	void InstancingData::UpdateEntity(Entity *entity)
	{
		_lock.Lock();
		
		Number *indexNum = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedIndexKey));
		size_t index = indexNum->GetUint32Value();
		
		_matrices[((index * 2) + 0)] = entity->GetWorldTransform();
		_matrices[((index * 2) + 1)] = entity->GetWorldTransform().GetInverse();
		
		if(_pivot)
			UpdateEntityLODStage(entity, _pivot->GetWorldPosition());
		
		_lock.Unlock();
	}
	
	
	void InstancingData::InsertEntityIntoLODStage(Entity *entity, size_t index)
	{
		size_t stage = 0;
		
		if(_pivot)
		{
			float distance = entity->GetWorldPosition().Distance(_pivot->GetWorldPosition());
			stage = _model->GetLODStageForDistance(distance / _pivot->clipfar);
		}
		
		_stages[stage]->AddIndex(index);
		
		Number *stageNum = new Number(static_cast<uint32>(stage));
		entity->SetAssociatedObject(kRNInstancingNodeAssociatedLODStageKey, stageNum, Object::MemoryPolicy::Retain);
		stageNum->Release();
	}
	
	void InstancingData::UpdateEntityLODStage(Entity *entity, const Vector3 &position)
	{
		float distance = entity->GetWorldPosition().Distance(position);
		
		size_t newStage = _model->GetLODStageForDistance(distance / _pivot->clipfar);
		size_t oldStage = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedLODStageKey))->GetUint32Value();
		
		if(newStage != oldStage)
		{
			size_t index = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedIndexKey))->GetUint32Value();
			
			_stages[oldStage]->RemoveIndex(index);
			_stages[newStage]->AddIndex(index);
			
			Number *stageNum = new Number(static_cast<uint32>(newStage));
			entity->SetAssociatedObject(kRNInstancingNodeAssociatedLODStageKey, stageNum, Object::MemoryPolicy::Retain);
			stageNum->Release();
		}
	}
}
