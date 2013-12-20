//
//  RNInstancingData.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		gl::GenTextures(1, &_texture);
		gl::GenBuffers(1, &_buffer);
		
		gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, _buffer);
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
	}
	
	InstancingLODStage::~InstancingLODStage()
	{
		gl::DeleteTextures(1, &_texture);
		gl::DeleteBuffers(1, &_buffer);
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
	
	void InstancingLODStage::UpdateData()
	{
		if(_dirty && !_indices.empty())
		{
			gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
			gl::BufferData(GL_TEXTURE_BUFFER, _indices.size() * sizeof(uint32), _indices.data(), GL_STATIC_DRAW);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
			
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
	
	
	
	
	InstancingData::InstancingData(Model *model)
	{
		_model = model->Retain();
		_pivot = nullptr;
		
		_count = 0;
		_used  = 0;
		
		size_t count = _model->GetLODStageCount();
		
		for(size_t i = 0; i < count; i ++)
			_stages.push_back(new InstancingLODStage(_model, i));
		
		gl::GenTextures(1, &_texture);
		gl::GenBuffers(1, &_buffer);
		
		gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _buffer);
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		gl::BindTexture(GL_TEXTURE_BUFFER, 0);
		
		Reserve(50);
	}
	
	InstancingData::~InstancingData()
	{
		for(InstancingLODStage *stage : _stages)
			delete stage;
		
		gl::DeleteTextures(1, &_texture);
		gl::DeleteBuffers(1, &_buffer);
	}
	
	void InstancingData::SetPivot(Camera *pivot)
	{
		_pivot = pivot; // Retained by the InstancingNode
	}
	
	void InstancingData::UpdateData()
	{
		if(_dirty)
		{
			gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
			gl::BufferData(GL_TEXTURE_BUFFER, static_cast<GLsizei>(_matrices.size() * sizeof(Matrix)), _matrices.data(), GL_STATIC_DRAW);
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
			
			_dirty = false;
		}
		
		for(InstancingLODStage *stage : _stages)
			stage->UpdateData();
		
		gl::Flush();
	}
	
	
	void InstancingData::Render(SceneNode *node, Renderer *renderer)
	{
		RenderingObject object(RenderingObject::Type::Instanced);
		node->FillRenderingObject(object);
		
		object.instancingData = _texture;
		
		size_t count = _stages.size();
		
		for(size_t i = 1; i < count; i ++)
		{
			if(_stages[i]->IsEmpty())
				continue;
			
			_stages[i]->Render(object, renderer);
		}
	}
		
	
	void InstancingData::Reserve(size_t count)
	{
		if(_count >= count)
			return;
		
		_matrices.resize(count * 2);
		_freeList.reserve(_freeList.size() + (count - _count));
		
		for(size_t i = 0; i < (count - _count); i ++)
		{
			_freeList.push_back(_count + i);
		}
		
		_count = count;
		_dirty = true;
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
		gl::BufferData(GL_TEXTURE_BUFFER, static_cast<GLsizei>(_matrices.size() * sizeof(Matrix)), nullptr, GL_STATIC_DRAW);
		gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
	}
	
	
	
	void InstancingData::InsertEntity(Entity *entity)
	{
		_lock.Lock();
		
		if(_entities.find(entity) == _entities.end())
		{
			_entities.insert(entity);
			
			if(_used >= _count)
				Reserve(_count * 1.5f);
		
			
			size_t stage = 0;
			size_t index = _freeList.back();
				
			_freeList.pop_back();
			_used ++;
			
			if(_pivot)
			{
				float distance = entity->GetWorldPosition().Distance(_pivot->GetWorldPosition());
				stage = _model->GetLODStageForDistance(distance / _pivot->clipfar);
			}
			
			_stages[stage]->AddIndex(index);
			
			Number *indexNum = new Number(static_cast<uint32>(index));
			Number *stageNum = new Number(static_cast<uint32>(stage));
			
			entity->SetAssociatedObject(kRNInstancingNodeAssociatedIndexKey, indexNum, Object::MemoryPolicy::Retain);
			entity->SetAssociatedObject(kRNInstancingNodeAssociatedLODStageKey, stageNum, Object::MemoryPolicy::Retain);
		
			indexNum->Release();
			stageNum->Release();
			
			_matrices[((index * 2) + 0)] = entity->GetWorldTransform();
			_matrices[((index * 2) + 1)] = entity->GetWorldTransform().GetInverse();
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
		_freeList.push_back(index);
		
		_lock.Unlock();
	}
	
	void InstancingData::UpdateEntity(Entity *entity)
	{
		_lock.Lock();
		
		size_t index = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedIndexKey))->GetUint32Value();
		
		_matrices[((index * 2) + 0)] = entity->GetWorldTransform();
		_matrices[((index * 2) + 1)] = entity->GetWorldTransform().GetInverse();
		
		if(_pivot)
			UpdateEntityLODStange(entity, _pivot->GetWorldPosition());
		
		_lock.Unlock();
	}
	
	
	
	void InstancingData::PivotMoved()
	{
		_lock.Lock();
		
		Vector3 position = _pivot->GetWorldPosition();
			
		for(auto entity : _entities)
			UpdateEntityLODStange(entity, position);
		
		_lock.Unlock();
	}
	
	void InstancingData::UpdateEntityLODStange(Entity *entity, const Vector3 &position)
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
