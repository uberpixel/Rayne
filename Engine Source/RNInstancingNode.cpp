//
//  RNInstancingNode.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInstancingNode.h"
#include "RNRenderer.h"
#include "RNNumber.h"
#include "RNThreadPool.h"
#include "RNNull.h"
#include "RNString.h"
#include "RNMessage.h"
#include "RNWorldCoordinator.h"
#include "RNOpenGLQueue.h"

#define kRNInstancingNodeAssociatedIndexKey "kRNInstancingNodeAssociatedIndexKey"

namespace RN
{
	RNDefineMeta(InstancingNode, SceneNode)
	
	InstancingNode::InstancingNode() :
		_clipRange("Clip range", 64.0f, &InstancingNode::GetClipRange, &InstancingNode::SetClippingRange),
		_thinRange("Thin range", 128.0f, &InstancingNode::GetThinRange, &InstancingNode::SetThinningRange),
		_cellSize("Cell size", 32.0f, &InstancingNode::GetCellSize, &InstancingNode::SetCellSize)
	{
		AddObservables({ &_clipRange, &_thinRange, &_cellSize });
		Initialize();
	}
	
	InstancingNode::InstancingNode(Model *model) :
		InstancingNode()
	{
		SetModel(model);
	}
	
	InstancingNode::~InstancingNode()
	{
		MessageCenter::GetSharedInstance()->RemoveObserver(this);
		_models->Release();
		
		for(auto pair : _data)
			delete pair.second;
		
		if(_pivot)
		{
			_pivot->RemoveObserver("position", this);
			_pivot->Release();
		}
	}
	
	void InstancingNode::CleanUp()
	{
		const Array *children = GetChildren();
		children->Enumerate<SceneNode>([&](SceneNode *node, size_t index, bool &stop) {
			node->RemoveObserver("model", this);
		});
		
		SceneNode::CleanUp();
	}
	
	
	void InstancingNode::Initialize()
	{
		_entityClass = Entity::MetaClass();
		_models      = new Set();
		_pivot       = nullptr;
		
		SetFlags(GetFlags() | Flags::HideChildren);
		
		MessageCenter::GetSharedInstance()->AddObserver(kRNWorldCoordinatorDidStepWorldMessage, [this](Message *mesage) {
			
			for(InstancingData *data : _rawData)
			{
				data->UpdateData();
			}
			
		}, this);
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
		
		_models->Enumerate([&](Object *object, bool &stop) {
			
			Model *model = static_cast<Model *>(object);
			auto iterator = _data.find(model);
			
			if(iterator == _data.end())
			{
				InstancingData *data = new InstancingData(model);
				UpdateData(data);
				
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
	
	
	void InstancingNode::SetMode(Mode mode)
	{
		Lock();
		
		_mode = mode;
		
		for(InstancingData *data : _rawData)
			UpdateData(data);
		
		Unlock();
	}
	
	void InstancingNode::SetClippingRange(float range)
	{
		Lock();
		
		_clipRange = range;
		
		for(InstancingData *data : _rawData)
			data->SetClipping((_mode & Mode::Clipping), _clipRange);
		
		Unlock();
	}
	
	void InstancingNode::SetThinningRange(float range)
	{
		Lock();
		
		_thinRange = range;
		
		for(InstancingData *data : _rawData)
			data->SetThinningRange((_mode & Mode::Thinning), _thinRange);
		
		Unlock();
	}
	
	void InstancingNode::SetCellSize(float size)
	{
		Lock();
		
		_cellSize = size;
		
		for(InstancingData *data : _rawData)
			data->SetCellSize(_cellSize);
		
		Unlock();
	}
	
	
	void InstancingNode::UpdateData(InstancingData *data)
	{
		data->SetPivot(_pivot);
		data->SetCellSize(_cellSize);
		data->SetClipping((_mode & Mode::Clipping), _clipRange);
		data->SetThinningRange((_mode & Mode::Thinning), _thinRange);
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
				
				entity->SetFlags(entity->GetFlags() | SceneNode::Flags::Hidden);
			}
			else
			{
				entity->SetFlags(entity->GetFlags() & ~SceneNode::Flags::Hidden);
			}
		}
		else
		{
			entity->SetFlags(entity->GetFlags() & ~SceneNode::Flags::Hidden);
		}
		
		Unlock();
	}
	
	void InstancingNode::PivotDidMove(Object *object, const std::string &key, Dictionary *changes)
	{
		for(InstancingData *data : _rawData)
			data->PivotMoved();
	}
	
	
	
	void InstancingNode::ChildDidUpdate(SceneNode *child, ChangeSet changes)
	{
		if(child->IsKindOfClass(_entityClass))
		{
			if(changes & ChangeSet::Position)
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
				entity->SetFlags(entity->GetFlags() | Flags::Hidden);
			}
		}
	}
	
	void InstancingNode::WillRemoveChild(SceneNode *child)
	{
		if(child->IsKindOfClass(_entityClass))
		{
			Entity *entity = static_cast<Entity *>(child);
			entity->RemoveObserver("model", this);
			entity->SetFlags(entity->GetFlags() & ~Flags::Hidden);
			
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
			data->Render(this, renderer);
		}
	}
}
