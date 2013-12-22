//
//  RNInstancingNode.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInstancingNode.h"
#include "RNRenderer.h"
#include "RNNumber.h"
#include "RNThreadPool.h"
#include "RNNull.h"
#include "RNString.h"

#define kRNInstancingNodeAssociatedIndexKey "kRNInstancingNodeAssociatedIndexKey"

namespace RN
{
	InstancingNode::InstancingNode()
	{
		Initialize();
	}
	
	InstancingNode::InstancingNode(Model *model)
	{
		Initialize();
		SetModel(model);
	}
	
	InstancingNode::~InstancingNode()
	{
		_models->Release();
		
		for(auto pair : _data)
			delete pair.second;
		
		if(_pivot)
		{
			_pivot->RemoveObserver("position", this);
			_pivot->Release();
		}
	}
	
	void InstancingNode::Initialize()
	{
		_entityClass = Entity::MetaClass();
		_models      = new Set();
		
		_pivot = nullptr;
		_mode  = 0;
		_limit = 0;
		
		SetFlags(GetFlags() | SceneNode::FlagHideChildren);
	}
	
	
	
	
	void InstancingNode::SetModel(Model *model)
	{
		Lock();
		
		_models->RemoveAllObjects();
		_models->AddObject(model);
		
		ModelsChanged();
		Unlock();
	}
	
	void InstancingNode::AddModel(Model *model)
	{
		Lock();
		
		_models->AddObject(model);
		
		ModelsChanged();
		Unlock();
	}
	
	void InstancingNode::SetModels(const Array *models)
	{
		Lock();
		
		_models->Release();
		_models = new Set(models);
		
		ModelsChanged();
		Unlock();
	}
	
	void InstancingNode::SetModels(const Set *models)
	{
		Lock();
		
		_models->Release();
		_models = new Set(models);
		
		ModelsChanged();
		Unlock();
	}
	
	void InstancingNode::ModelsChanged()
	{
		std::vector<InstancingData *> erased;
		
		for(auto pair : _data)
		{
			if(!_models->ContainsObject(pair.first))
			{
				erased.push_back(pair.second);
				_rawData.erase(std::find(_rawData.begin(), _rawData.end(), pair.second));
			}
		}
		
		for(InstancingData *data : erased)
		{
			_data.erase(data->GetModel());
			delete data;
		}
		
		_models->Enumerate([&](Object *object, bool *end) {
			
			Model *model = static_cast<Model *>(object);
			auto iterator = _data.find(model);
			
			if(iterator == _data.end())
			{
				InstancingData *data = new InstancingData(model);
				data->SetPivot(_pivot);
				data->SetLimit(_limit);
				
				_data.insert(decltype(_data)::value_type(model, data));
				_rawData.push_back(data);
			}
			
		});
	}
	
	void InstancingNode::SetPivot(Camera *pivot)
	{
		Lock();
		
		if(_pivot)
		{
			RemoveDependency(_pivot);
			
			_pivot->RemoveObserver("position", this);
			_pivot->Release();
			_pivot = nullptr;
		}
		
		if(pivot)
		{
			_pivot = pivot->Retain();
			_pivot->AddObserver("position", std::bind(&InstancingNode::PivotDidMove, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), this);
			
			AddDependency(_pivot);
		}
		
		for(InstancingData *data : _rawData)
		{
			data->SetPivot(_pivot);
		}
		
		Unlock();
	}
	
	void InstancingNode::SetLimit(size_t limit)
	{
		Lock();

		_limit = limit;
		
		for(InstancingData *data : _rawData)
		{
			data->SetLimit(_limit);
		}
		
		Unlock();
	}

	
	
	void InstancingNode::EntityDidUpdateModel(Object *object, const std::string &key, Dictionary *changes)
	{
		Object *tOld = changes->GetObjectForKey(kRNObservableOldValueKey);
		Object *tNew = changes->GetObjectForKey(kRNObservableNewValueKey);
		
		Lock();
		
		if(tOld != Null::GetNull())
		{
			Model *model = static_cast<Model *>(tOld);
			
			auto iterator = _data.find(model);
			if(iterator != _data.end())
			{
				InstancingData *data = iterator->second;
				data->RemoveEntity(static_cast<Entity *>(object));
			}
		}
		
		Entity *entity = static_cast<Entity *>(object);
		
		if(tNew != Null::GetNull())
		{
			Model *model = static_cast<Model *>(tNew);
			
			auto iterator = _data.find(model);
			if(iterator != _data.end())
			{
				InstancingData *data = iterator->second;
				data->InsertEntity(static_cast<Entity *>(object));
				
				entity->SetFlags(entity->GetFlags() | SceneNode::FlagHidden);
			}
			else
			{
				entity->SetFlags(entity->GetFlags() & ~SceneNode::FlagHidden);
			}
		}
		else
		{
			entity->SetFlags(entity->GetFlags() & ~SceneNode::FlagHidden);
		}
		
		Unlock();
	}
	
	void InstancingNode::PivotDidMove(Object *object, const std::string &key, Dictionary *changes)
	{
		for(InstancingData *data : _rawData)
			data->PivotMoved();
	}
	
	
	
	void InstancingNode::ChildDidUpdate(SceneNode *child, uint32 changes)
	{
		if(child->IsKindOfClass(_entityClass))
		{
			if(changes & ChangedPosition)
			{
				Lock();
				
				Entity *entity = static_cast<Entity *>(child);
				
				auto iterator = _data.find(entity->GetModel());
				if(iterator != _data.end())
				{
					InstancingData *data = iterator->second;
					data->UpdateEntity(entity);
				}
				
				Unlock();
			}
		}
	}
	
	void InstancingNode::WillAddChild(SceneNode *child)
	{
		if(child->IsKindOfClass(_entityClass))
		{
			Entity *entity = static_cast<Entity *>(child);
			entity->AddObserver("model", std::bind(&InstancingNode::EntityDidUpdateModel, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), this);
			
			auto iterator = _data.find(entity->GetModel());
			if(iterator != _data.end())
			{
				InstancingData *data = iterator->second;
				
				data->InsertEntity(entity);
				entity->SetFlags(entity->GetFlags() | SceneNode::FlagHidden);
			}
		}
	}
	
	void InstancingNode::WillRemoveChild(SceneNode *child)
	{
		if(child->IsKindOfClass(_entityClass))
		{
			Entity *entity = static_cast<Entity *>(child);
			entity->RemoveObserver("model", this);
			entity->SetFlags(entity->GetFlags() & ~SceneNode::FlagHidden);
			
			auto iterator = _data.find(entity->GetModel());
			if(iterator != _data.end())
			{
				InstancingData *data = iterator->second;
				data->RemoveEntity(entity);
			}
		}
	}
	
	bool InstancingNode::IsVisibleInCamera(Camera *camera)
	{
		return true;
	}
	
	void InstancingNode::Render(Renderer *renderer, Camera *camera)
	{
		for(InstancingData *data : _rawData)
		{
			data->UpdateData();
			data->Render(this, renderer);
		}
	}
}
