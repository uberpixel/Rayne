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
	InstancingLODStageData::InstancingLODStageData(Model *model, size_t stage, size_t index)
	{
		_mesh     = model->GetMeshAtIndex(static_cast<uint32>(stage), static_cast<uint32>(index));
		_material = model->GetMaterialAtIndex(static_cast<uint32>(stage), static_cast<uint32>(index));
	}
	
	InstancingLODStage::InstancingLODStage(Model *model, size_t stage) :
		_dirty(true)
	{
		gl::GenTextures(1, &_texture);
		gl::GenBuffers(1, &_buffer);
		
		gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
		gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
		gl::TexBuffer(GL_TEXTURE_BUFFER, GL_R16UI, _buffer);
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
		gl::BindTexture(GL_TEXTURE_BUFFER, 0);
		
		size_t count = model->GetMeshCount(static_cast<uint32>(stage));
		
		for(size_t i = 0; i < count; i ++)
			_data.emplace_back(model, stage, i);
	}
	
	
	InstancingLODStage::~InstancingLODStage()
	{
		gl::DeleteTextures(1, &_texture);
		gl::DeleteBuffers(1, &_buffer);
	}
	
	void InstancingLODStage::RemoveIndex(size_t index)
	{
		_indices.erase(std::find(_indices.begin(), _indices.end(), static_cast<uint16>(index)));
		_dirty = true;
	}
	
	void InstancingLODStage::AddIndex(size_t index)
	{
		_indices.push_back(index);
		_dirty = true;
	}
	
	void InstancingLODStage::UpdateData()
	{
		if(_dirty)
		{
			gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
			gl::BufferData(GL_TEXTURE_BUFFER, _indices.size() * sizeof(uint16), _indices.data(), GL_STATIC_DRAW);
			
			_dirty = false;
		}
	}
	
	void InstancingLODStage::Render(RenderingObject &object, Renderer *renderer)
	{
		object.count = static_cast<uint32>(_indices.size());
		object.instancingIndices = _texture;
		
		for(auto &data : _data)
		{
			object.mesh     = data._mesh;
			object.material = data._material;
			
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
		{
			_stages.push_back(new InstancingLODStage(_model, i));
		}
		
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
		gl::DeleteTextures(1, &_texture);
		gl::DeleteBuffers(1, &_buffer);
	}
	
	void InstancingData::UpdateData()
	{
		if(_dirty)
		{
			size_t size = _count * 2 * sizeof(Matrix);
			
			gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
			gl::BufferSubData(GL_TEXTURE_BUFFER, 0, static_cast<GLsizei>(size), _matrices.data());
			gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
			
			_dirty = false;
		}
		
		for(InstancingLODStage *stage : _stages)
			stage->UpdateData();
	}
	
	void InstancingData::Render(SceneNode *node, Renderer *renderer)
	{
		RenderingObject object(RenderingObject::Type::Instanced);
		node->FillRenderingObject(object);
		
		object.instancingData = _texture;
		
		for(InstancingLODStage *stage : _stages)
		{
			if(stage->IsEmpty())
				continue;
			
			stage->Render(object, renderer);
		}
	}
		
	
	void InstancingData::Reserve(size_t count)
	{
		if(_count >= count)
			return;
		
		_usage.resize(count);
		_matrices.resize(count * 2);
		
		std::fill(_usage.begin() + _count, _usage.end(), false);
		
		_count = count;
		_dirty = true;
		
		size_t size = _count * 2 * sizeof(Matrix);
		
		gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
		gl::BufferData(GL_TEXTURE_BUFFER, static_cast<GLsizei>(size), nullptr, GL_DYNAMIC_DRAW);
		gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
	}
	
	void InstancingData::InsertEntity(Entity *entity)
	{
		if(_entities.find(entity) == _entities.end())
		{
			_entities.insert(entity);
			
			size_t index;
			auto iterator = (_used < _count) ? std::find(_usage.begin(), _usage.end(), false) : _usage.end();
			
			if(iterator != _usage.end())
			{
				index = std::distance(_usage.begin(), iterator);
				
				_usage[index] = true;
				_used  ++;
			}
			else
			{
				index = _matrices.size();
				Reserve(_count * 1.5f);
				
				_usage[index] = true;
				_used  ++;
			}
			
			size_t stage = 0;
			
			if(_pivot)
				stage = _model->GetLODStageForDistance(entity->GetWorldPosition().Distance(_pivot->GetWorldPosition()));
			
			_stages[stage]->AddIndex(index);
			
			entity->SetAssociatedObject(kRNInstancingNodeAssociatedIndexKey, Number::WithUint32(static_cast<uint32>(index)), Object::MemoryPolicy::Retain);
			entity->SetAssociatedObject(kRNInstancingNodeAssociatedLODStageKey, Number::WithUint32(static_cast<uint32>(stage)), Object::MemoryPolicy::Retain);
		
			UpdateEntity(entity);
		}
	}
	
	void InstancingData::RemoveEntity(Entity *entity)
	{
		if(_entities.find(entity) == _entities.end())
			return;
		
		size_t index = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedIndexKey))->GetUint32Value();
		uint32 stage = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedLODStageKey))->GetUint32Value();
		
		entity->RemoveAssociatedOject(kRNInstancingNodeAssociatedIndexKey);
		entity->RemoveAssociatedOject(kRNInstancingNodeAssociatedLODStageKey);
		
		_stages[stage]->RemoveIndex(index);
		
		_entities.erase(entity);
		_usage[index] = false;
	}
	
	void InstancingData::UpdateEntity(Entity *entity)
	{
		size_t index = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedIndexKey))->GetUint32Value();
		
		_matrices[((index * 2) + 0)] = entity->GetWorldTransform();
		_matrices[((index * 2) + 1)] = entity->GetWorldTransform().GetInverse();
		
		UpdateEntityLODStange(entity);
	}
	
	void InstancingData::PivotMoved()
	{
		if(!_pivot)
			return;
			
		for(auto entity : _entities)
			UpdateEntityLODStange(entity);
	}
	
	void InstancingData::UpdateEntityLODStange(Entity *entity)
	{
		if(_pivot)
		{
			size_t oldStage = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedLODStageKey))->GetUint32Value();
			size_t newStage = _model->GetLODStageForDistance(entity->GetWorldPosition().Distance(_pivot->GetWorldPosition()));
			
			if(newStage != oldStage)
			{
				size_t index = static_cast<Number *>(entity->GetAssociatedObject(kRNInstancingNodeAssociatedIndexKey))->GetUint32Value();
				
				_stages[oldStage]->RemoveIndex(index);
				_stages[newStage]->AddIndex(index);
			}
		}
	}
}
