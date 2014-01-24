//
//  RNInstancingData.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInstancingData.h"
#include "RNLogging.h"
#include "RNOpenGLQueue.h"
#include "RNThreadPool.h"

namespace RN
{
	// ---------------------
	// MARK: -
	// MARK: __InstancingBucket
	// ---------------------
	
	struct __InstancingBucket
	{
		__InstancingBucket() :
			active(false)
		{}
		
		__InstancingBucket(Vector3 &&tposition) :
			active(false),
			position(std::move(tposition))
		{}
		
		std::vector<Entity *> nodes;
		Vector3 position;
		bool active;
	};
	
	// ---------------------
	// MARK: -
	// MARK: InstancingEntity
	// ---------------------
	
	struct InstancingEntity
	{
		InstancingEntity(size_t tindex) :
			index(tindex),
			lodStage(k::NotFound)
		{}
		
		size_t index;
		size_t lodStage;
		float  thinfactor;
		__InstancingBucket *bucket;
	};
	
	// ---------------------
	// MARK: -
	// MARK: InstancingLODStage
	// ---------------------
	
	InstancingLODStage::InstancingLODStage(Model *model, size_t stage) :
		_model(model),
		_stage(stage),
		_dirty(true),
		_indicesData(nullptr),
		_indicesSize(0)
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
		
		delete [] _indicesData;
	}
	
	void InstancingLODStage::RemoveIndex(size_t index)
	{
		_indices.erase(static_cast<uint32>(index));
		_dirty = true;
	}
	
	void InstancingLODStage::AddIndex(size_t index)
	{
		_indices.insert(static_cast<uint32>(index));
		_dirty = true;
	}
	
	void InstancingLODStage::Clear()
	{
		_indices.clear();
		_dirty = true;
	}
	
	void InstancingLODStage::UpdateData(bool dynamic)
	{
		if(_dirty && !_indices.empty())
		{
			GLenum mode = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
			
			if(_indicesSize < _indices.size())
			{
				delete [] _indicesData;
				
				_indicesData = new uint32[_indices.size()];
				_indicesSize = _indices.size();
			}
			
			size_t i = 0;
			
			for(uint32 index : _indices)
				_indicesData[i ++] = index;
			
			OpenGLQueue::GetSharedInstance()->SubmitCommand([this, mode] {
				gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
				gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
				gl::BufferData(GL_TEXTURE_BUFFER, _indices.size() * sizeof(uint32), _indicesData, mode);
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
	
	// ---------------------
	// MARK: -
	// MARK: InstancingData
	// ---------------------
	
	static ThreadPool::SmallObjectsAllocator __instancingAllocator;
	
	InstancingData::InstancingData(Model *model) :
		_capacity(0),
		_count(0),
		_clipping(false),
		_buckets(32.0f, false),
		_needsRecreation(false),
		_dirtyIndices(false),
		_thinning(true),
		_thinRange(32.0f)
	{
		_model = model->Retain();
		_pivot = nullptr;
		
		size_t count = _model->GetLODStageCount();
		_hasLODStages = (count > 1);
		
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
		
		SetClipping(true, 64);
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
	
	void InstancingData::SetClipping(bool clipping, float distance)
	{
		if(_clipping != clipping)
		{
			_clipping = clipping;
			_needsRecreation = true;
		}
		
		_clipRange = distance;
	}
	
	void InstancingData::SetCellSize(float cellSize)
	{
		if(Math::Compare(cellSize, _buckets.get_spacing()))
			return;
			
		_activeBuckets.clear();
		_activeEntites.clear();
		
		_buckets.set_spacing(cellSize, false);
		_needsRecreation = true;
		
		
		for(Entity *entity : _entities)
		{
			InstancingEntity *data = static_cast<InstancingEntity *>(entity->_instancedData);
			Vector3 position = entity->GetPosition();
			auto &bucket = _buckets[position];
			
			if(!bucket)
				bucket = std::make_shared<__InstancingBucket>();
			
			bucket->nodes.push_back(entity);
			data->bucket = bucket.get();
		}
	}
	
	void InstancingData::SetThinningRange(bool thinning, float thinRange)
	{
		if(_thinning != thinning)
		{
			_thinning  = thinning;
			_needsClipping = true;
		}
			
		_thinRange = thinRange;
	}
	
	void InstancingData::PivotMoved()
	{
		_pivotMoved    = true;
		_needsClipping = true;
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
	
	void InstancingData::UpdateData()
	{
		RN_ASSERT((_clipping && _pivot) || (!_clipping), "When enabling clipping, you must also set a pivot!");
		
		if(_needsRecreation)
		{
			// Remove everything \o/
			for(InstancingBucket &bucket : _activeBuckets)
			{
				bucket->active = false;
			}
			
			_activeBuckets.clear();
			_activeEntites.clear();
			
			for(InstancingLODStage *stage : _stages)
				stage->Clear();
			
			// Insert data if needed
			if(!_clipping)
			{
				for(Entity *entity : _entities)
					InsertEntityIntoLODStage(entity);
			}
			
			_needsRecreation = false;
		}
		
		if(_pivot)
		{
			if(_clipping && _needsClipping)
			{
				ClipEntities();
				_needsClipping = false;
			}
			
			if(_pivotMoved)
			{
				if(_hasLODStages)
				{
					Vector3 position = _pivot->GetWorldPosition();
					
					for(Entity *entity : _activeEntites)
						UpdateEntityLODStage(entity, position);
				}
				
				_pivotMoved = false;
			}
		}
		
		if(_dirty)
		{
			OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
			
				LockGuard<decltype(_lock)> lock(_lock);
				
				gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
				gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
				gl::BufferData(GL_TEXTURE_BUFFER, static_cast<GLsizei>(_matrices.size() * sizeof(Matrix)), _matrices.data(), GL_STATIC_DRAW);
				gl::BindTexture(GL_TEXTURE_BUFFER, 0);
				gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
				
			});
			
			_dirty = false;
			_dirtyIndices = false;
		}
		else if(_dirtyIndices)
		{
			OpenGLQueue::GetSharedInstance()->SubmitCommand([this] {
				
				LockGuard<decltype(_lock)> lock(_lock);
				
				gl::BindTexture(GL_TEXTURE_BUFFER, _texture);
				gl::BindBuffer(GL_TEXTURE_BUFFER, _buffer);
				gl::BufferSubData(GL_TEXTURE_BUFFER, 0, static_cast<GLsizei>(_matrices.size() * sizeof(Matrix)), _matrices.data());
				gl::BindTexture(GL_TEXTURE_BUFFER, 0);
				gl::BindBuffer(GL_TEXTURE_BUFFER, 0);
				
				_dirtyIndices = false;
				
			});
		}
		
		for(InstancingLODStage *stage : _stages)
			stage->UpdateData((_pivot != nullptr));
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
	
	
	
	
	size_t InstancingData::GetIndex(Entity *entity)
	{
		if(_count >= _capacity)
			Reserve(_capacity * 1.7f);
		
		size_t index = _freeList.back();
		
		_freeList.pop_back();
		_count ++;
		
		_matrices[((index * 2) + 0)] = std::move(entity->GetWorldTransform());
		_matrices[((index * 2) + 1)] = std::move(entity->GetWorldTransform().GetInverse());
		
		_dirtyIndices = true;
		return index;
	}
	
	void InstancingData::ResignIndex(size_t index)
	{
		_count --;
		_freeList.push_back(index);
	}
	
	
	
	void InstancingData::ResignBucket(InstancingBucket &bucket)
	{
		for(Entity *entity : bucket->nodes)
		{
			InstancingEntity *data = reinterpret_cast<InstancingEntity *>(entity->_instancedData);
			
			if(data->index != k::NotFound)
			{
				if(data->lodStage != k::NotFound)
				{
					_stages[data->lodStage]->RemoveIndex(data->index);
					data->lodStage = k::NotFound;
				}
				
				ResignIndex(data->index);
				data->index = k::NotFound;
			}
			
			_activeEntites.erase(entity);
		}
		
		bucket->active = false;
	}
	
	void InstancingData::ClipEntities()
	{
		// Get all buckets within the clip range
		Vector3 position = _pivot->GetWorldPosition();
		
		std::vector<InstancingBucket> buckets;
		float clipRange = _clipRange;
		
		if(_thinning)
			clipRange += _thinRange;
		
		clipRange = std::max(static_cast<float>(_buckets.get_spacing()), clipRange);
		_buckets.query(AABB(position, clipRange), buckets);
		
		// Remove not actually visible buckets
		{
			std::vector<InstancingBucket> temp;
			
			for(InstancingBucket &bucket : buckets)
			{
				if(bucket->position.Distance(position) < clipRange)
					temp.push_back(bucket);
			}
			
			std::swap(temp, buckets);
		}
		
		
		// Resign no longer active buckets
		for(auto i = _activeBuckets.begin(); i != _activeBuckets.end();)
		{
			auto iterator = std::find(buckets.begin(), buckets.end(), *i);
			if(iterator == buckets.end())
			{
				ResignBucket(*i);
				i = _activeBuckets.erase(i);
				continue;
			}
			
			i ++;
		}
		
		// Activate non active buckets
		for(auto &bucket : buckets)
		{
			if(!bucket->active)
			{
				_activeEntites.insert(bucket->nodes.begin(), bucket->nodes.end());
				_activeBuckets.insert(_activeBuckets.end(), bucket);
				
				bucket->active = true;
			}
		}
		
		
		// Update active entities
		ThreadPool::Batch *batch = ThreadPool::GetSharedInstance()->CreateBatch(__instancingAllocator);
		SpinLock lock;
		
		batch->Reserve(_activeEntites.size());
		
		for(Entity *entity : _activeEntites)
		{
			batch->AddTask([&, entity] {
			
				InstancingEntity *data = reinterpret_cast<InstancingEntity *>(entity->_instancedData);
				float distance = position.Distance(entity->GetWorldPosition_NoLock());
				
				bool clipped = true;
				
				if(_thinning)
				{
					if(distance <= _clipRange)
					{
						clipped = false;
					}
					else if(distance <= clipRange)
					{
						distance -= _clipRange;
						distance /= _thinRange;
						
						clipped = (distance > data->thinfactor);
					}
				}
				else
				{
					clipped = (distance > _clipRange);
				}
				
				if(clipped && data->lodStage != k::NotFound)
				{
					lock.Lock();
					ResignIndex(data->index);
					_stages[data->lodStage]->RemoveIndex(data->index);
					lock.Unlock();
					
					data->lodStage = k::NotFound;
					data->index = k::NotFound;
				}
				else if(!clipped && data->lodStage == k::NotFound)
				{
					lock.Lock();
					InsertEntityIntoLODStage(entity);
					lock.Unlock();
				}
					
			});
		}
		
		batch->Commit();
		batch->Wait();
		batch->Release();
	}
	
	
	
	
	void InstancingData::InsertEntity(Entity *entity)
	{
		LockGuard<SpinLock> lock(_lock);
		
		_needsClipping = true;
		_entities.push_back(entity);
		
		
		InstancingEntity *data = new InstancingEntity(k::NotFound);
		entity->_instancedData = data;
		
		// Calculate the thin stage
		data->thinfactor = _random.RandomFloat();
		
		// Insert into the right bucket
		Vector3 position = entity->GetPosition();
		auto &bucket = _buckets[position];
		
		if(!bucket)
		{
			bucket = std::make_shared<__InstancingBucket>(_buckets.translate_vector(position));
			bucket->position *= _buckets.get_spacing();
		}
		
		if(bucket->active)
			_activeEntites.insert(entity);
		
		bucket->nodes.push_back(entity);
		data->bucket = bucket.get();
		
		// Enable the entity
		if(!_clipping)
		{
			data->index = GetIndex(entity);
			
			if(!_needsRecreation) // This would just be thrown anyways at the end of the frame
				InsertEntityIntoLODStage(entity);
		}
	}
	
	void InstancingData::RemoveEntity(Entity *entity)
	{
		LockGuard<SpinLock> lock(_lock);
		
		InstancingEntity *data = static_cast<InstancingEntity *>(entity->_instancedData);
		data->bucket->nodes.erase(std::find(data->bucket->nodes.begin(), data->bucket->nodes.end(), entity));
		
		if(data->index != k::NotFound)
		{
			if(data->bucket->active)
			{
				if(data->lodStage != k::NotFound)
					_stages[data->lodStage]->RemoveIndex(data->index);
				
				_freeList.push_back(data->index);
				_activeEntites.erase(entity);
			}
			
			ResignIndex(data->index);
		}
		
		delete data;
	}
	
	void InstancingData::UpdateEntity(Entity *entity)
	{
		LockGuard<SpinLock> lock(_lock);
		InstancingEntity *data = reinterpret_cast<InstancingEntity *>(entity->_instancedData);
		
		Vector3 position = entity->GetPosition();
		InstancingBucket &bucket = _buckets[position];
		
		if(!bucket)
		{
			bucket = std::make_shared<__InstancingBucket>(_buckets.translate_vector(position));
			bucket->position *= _buckets.get_spacing();
		}
		
		if(bucket.get() != data->bucket)
		{
			data->bucket->nodes.erase(std::find(data->bucket->nodes.begin(), data->bucket->nodes.end(), entity));
			data->bucket = bucket.get();
			
			bucket->nodes.push_back(entity);
			
			if(bucket->active)
			{
				data->index = (data->index == k::NotFound) ? GetIndex(entity) : data->index;
				_activeEntites.insert(entity);
			}
		}
		
		if(data->index != k::NotFound)
		{
			_matrices[((data->index * 2) + 0)] = entity->GetWorldTransform();
			_matrices[((data->index * 2) + 1)] = entity->GetWorldTransform().GetInverse();
			
			_dirtyIndices = true;
		}
		
		if((_pivot && _hasLODStages) && data->lodStage != k::NotFound)
			UpdateEntityLODStage(entity, _pivot->GetWorldPosition());
	}
	
	
	
	
	void InstancingData::InsertEntityIntoLODStage(Entity *entity)
	{
		InstancingEntity *data = reinterpret_cast<InstancingEntity *>(entity->_instancedData);
		size_t stage = 0;
		
		if(_pivot && _hasLODStages)
		{
			float distance = entity->GetWorldPosition().Distance(_pivot->GetWorldPosition());
			stage = _model->GetLODStageForDistance(distance / _pivot->clipfar);
		}
		
		if(data->index == k::NotFound)
			data->index = GetIndex(entity);
		
		_stages[stage]->AddIndex(data->index);
		data->lodStage = stage;
	}
	
	void InstancingData::UpdateEntityLODStage(Entity *entity, const Vector3 &position)
	{
		InstancingEntity *data = reinterpret_cast<InstancingEntity *>(entity->_instancedData);
		
		float distance = entity->GetWorldPosition().Distance(position);
		size_t newStage = _model->GetLODStageForDistance(distance / _pivot->clipfar);
		
		if(newStage != data->lodStage)
		{
			if(data->lodStage != k::NotFound)
				_stages[data->lodStage]->RemoveIndex(data->index);
			
			_stages[newStage]->AddIndex(data->index);
			data->lodStage = newStage;
		}
	}
}
