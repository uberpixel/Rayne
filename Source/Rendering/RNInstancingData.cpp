//
//  RNInstancingData.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNInstancingData.h"
#include "../Debug/RNLogger.h"
#include "../Threads/RNWorkGroup.h"

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
			lodStage(nullptr)
		{}

		size_t index;
		Model::LODStage *lodStage;
		float  thinfactor;
		__InstancingBucket *bucket;
	};

	// ---------------------
	// MARK: -
	// MARK: InstancingLODStage
	// ---------------------

	InstancingLODStage::InstancingLODStage(Model::LODStage *stage) :
		_stage(stage),
		_dirty(true),
		_indicesBuffer(nullptr),
		_indicesSize(50)
	{
		_indicesBuffer = new DynamicGPUBuffer(_indicesSize * sizeof(uint32));

		Renderer *renderer = Renderer::GetActiveRenderer();
		size_t count = stage->GetCount();

		for(size_t i = 0; i < count; i ++)
		{
			Drawable *drawable = renderer->CreateDrawable();

			drawable->Update(_stage->GetMeshAtIndex(i), _stage->GetMaterialAtIndex(i)->Copy(), nullptr);
			_drawables.push_back(drawable);
		}
	}

	InstancingLODStage::~InstancingLODStage()
	{
		_indicesBuffer->Release();

		for(Drawable *drawable : _drawables)
		{
			drawable->material->Release(); // The drawable "owns" the material through us, since it is a copy of the models material
			delete drawable;
		}
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
			if(_indicesSize < _indices.size())
			{
				_indicesBuffer->Resize(static_cast<size_t>(_indices.size() * 1.5f) * sizeof(uint32));
				_indicesSize = _indicesBuffer->GetLength() / sizeof(uint32);
			}

			_indicesBuffer->Advance();

			uint32 *buffer = reinterpret_cast<uint32 *>(_indicesBuffer->GetBuffer());
			std::copy(_indices.begin(), _indices.end(), buffer);

			_indicesBuffer->InvalidateRange(Range(0, _indices.size() * sizeof(uint32)));
			_dirty = false;
		}
	}

	void InstancingLODStage::Render(const SceneNode *node, Renderer *renderer, GPUBuffer *buffer) const
	{
		Array *buffers = Array::WithObjects({ _indicesBuffer->GetGPUBuffer(), buffer });

		for(Drawable *drawable : _drawables)
		{
			drawable->count = _indices.size();
			drawable->material->SetVertexBuffers(buffers);
			drawable->Update(node);

			renderer->SubmitDrawable(drawable);
		}
	}

	// ---------------------
	// MARK: -
	// MARK: InstancingData
	// ---------------------

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
		_model = model->Copy();
		_pivot = nullptr;

		size_t count = _model->GetLODStageCount();
		_hasLODStages = (count > 1);

		for(size_t i = 0; i < count; i ++)
		{
			Model::LODStage *stage = _model->GetLODStage(i);

			for(size_t j = 0; j < stage->GetCount(); j ++)
			{
				Material *material = stage->GetMaterialAtIndex(j);
				MaterialDescriptor descriptor = material->GetDescriptor();

				Shader *vertex = descriptor.vertexShader;
				Shader *fragment = descriptor.fragmentShader;

				vertex = vertex->GetLibrary()->GetShaderWithName(vertex->GetName()->StringByAppendingString(RNCSTR("_instanced")));
				fragment = fragment->GetLibrary()->GetShaderWithName(fragment->GetName()->StringByAppendingString(RNCSTR("_instanced")));

				descriptor.SetShaderProgram(ShaderProgram::WithVertexAndFragmentShaders(vertex, fragment));

				stage->ReplaceMaterial(Material::WithDescriptor(descriptor), j);
			}

			_stages.push_back(new InstancingLODStage(stage));
		}

		_buffer = new DynamicGPUBuffer(50);

		SetClipping(true, 64);
		Reserve(50);
	}

	InstancingData::~InstancingData()
	{
		for(InstancingLODStage *stage : _stages)
			delete stage;

		_buffer->Release();
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
		if(Math::Compare(cellSize, _buckets.GetSpacing()))
			return;

		_activeBuckets.clear();
		_activeEntites.clear();

		_buckets.SetSpacing(cellSize, false);
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
		_dirty = true;
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
			_buffer->Resize(static_cast<size_t>(_matrices.size() * sizeof(Matrix)));
			_buffer->Advance();

			Matrix *buffer = reinterpret_cast<Matrix *>(_buffer->GetBuffer());
			std::copy(_matrices.begin(), _matrices.end(), buffer);
			_buffer->Invalidate();

			_dirty = false;
			_dirtyIndices = false;
		}
		else if(_dirtyIndices)
		{
			Matrix *buffer = reinterpret_cast<Matrix *>(_buffer->GetBuffer());
			std::copy(_matrices.begin(), _matrices.end(), buffer);
			_buffer->Invalidate();

			_dirtyIndices = false;
		}

		for(InstancingLODStage *stage : _stages)
			stage->UpdateData((_pivot != nullptr));
	}

	void InstancingData::Render(const SceneNode *node, Renderer *renderer)
	{
		size_t count = _stages.size();

		for(size_t i = 0; i < count; i ++)
		{
			if(_stages[i]->IsEmpty())
				continue;

			_stages[i]->Render(node, renderer, _buffer->GetGPUBuffer());
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
				if(data->lodStage)
				{
					_stages[data->lodStage->GetIndex()]->RemoveIndex(data->index);
					data->lodStage = nullptr;
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

		clipRange = std::max(static_cast<float>(_buckets.GetSpacing()), clipRange);
		_buckets.Query(AABB(position, clipRange), buckets);

		// Remove not actually visible buckets
		{
			std::vector<InstancingBucket> temp;

			for(InstancingBucket &bucket : buckets)
			{
				if(bucket->position.GetSquaredDistance(position) < clipRange * clipRange && _pivot->InFrustum(bucket->position, _buckets.GetSpacing()))
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

		std::vector<Entity *> entities;

		// Activate non active buckets
		for(auto &bucket : buckets)
		{
			if(!bucket->active)
			{
				_activeEntites.insert(bucket->nodes.begin(), bucket->nodes.end());
				_activeBuckets.insert(_activeBuckets.end(), bucket);

				bucket->active = true;
			}

			entities.insert(entities.end(), bucket->nodes.begin(), bucket->nodes.end());
		}

		// Update active entities
		WorkGroup *group = new WorkGroup();
		WorkQueue *queue = WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default);

		size_t count = std::thread::hardware_concurrency();

		size_t perThread = entities.size() / count;
		size_t leftOver  = entities.size() % count;

		for(size_t i = 0; i < count; i ++)
		{
			group->Perform(queue, [&, i] {

				size_t total = perThread + ((i == count - 1) ? leftOver : 0);
				Entity **entityData = entities.data() + (i * perThread);

				std::vector<std::pair<size_t, size_t>> resigned;
				std::vector<Entity *> inserted;

				for(size_t i = 0; i < total; i ++)
				{
					Entity *entity = entityData[i];

					InstancingEntity *data = reinterpret_cast<InstancingEntity *>(entity->_instancedData);
					float distance = position.GetSquaredDistance(entity->GetWorldPosition());

					bool clipped;
					float _clipRange2 = _clipRange * _clipRange;

					if(_thinning)
					{
						if(distance <= _clipRange2)
						{
							clipped = false;
						}
						else
						{
							distance -= _clipRange2;
							distance /= _thinRange*_thinRange;
							distance = 1.0f-distance;
							distance *= distance*distance;
							distance = 1.0f - distance;
							clipped = (distance > data->thinfactor);
						}
					}
					else
					{
						clipped = (distance > _clipRange2);
					}

					if(clipped && data->lodStage)
					{
						resigned.push_back(std::make_pair(data->index, data->lodStage->GetIndex()));

						data->lodStage = nullptr;
						data->index = k::NotFound;
					}
					else if(!clipped && !data->lodStage)
					{
						inserted.push_back(entity);
					}
				}

				if(!resigned.empty() || !inserted.empty())
				{
					for(auto &index : resigned)
					{
						ResignIndex(index.first);
						_stages[index.second]->RemoveIndex(index.first);
					}

					for(Entity *entity : inserted)
					{
						InsertEntityIntoLODStage(entity);
					}
				}

			});
		}

		group->Wait();
		group->Release();
	}




	void InstancingData::InsertEntity(Entity *entity)
	{
		LockGuard<SpinLock> lock(_lock);

		_needsClipping = true;
		_entities.push_back(entity);


		InstancingEntity *data = new InstancingEntity(k::NotFound);
		entity->_instancedData = data;

		// Calculate the thin stage
		data->thinfactor = _random.GetRandomFloat();

		// Insert into the right bucket
		Vector3 position = entity->GetPosition();
		auto &bucket = _buckets[position];

		if(!bucket)
		{
			bucket = std::make_shared<__InstancingBucket>(_buckets.TranslateVector(position));
			bucket->position *= _buckets.GetSpacing();
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
				if(data->lodStage)
					_stages[data->lodStage->GetIndex()]->RemoveIndex(data->index);

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
			bucket = std::make_shared<__InstancingBucket>(_buckets.TranslateVector(position));
			bucket->position *= _buckets.GetSpacing();
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

		if((_pivot && _hasLODStages) && data->lodStage)
			UpdateEntityLODStage(entity, _pivot->GetWorldPosition());
	}




	void InstancingData::InsertEntityIntoLODStage(Entity *entity)
	{
		InstancingEntity *data = reinterpret_cast<InstancingEntity *>(entity->_instancedData);
		Model::LODStage *stage = _model->GetLODStage(0);

		if(_pivot && _hasLODStages)
		{
			float distance = entity->GetWorldPosition().GetDistance(_pivot->GetWorldPosition());
			stage = _model->GetLODStageForDistance(distance / _pivot->GetClipFar());
		}

		if(data->index == k::NotFound)
			data->index = GetIndex(entity);

		_stages[stage->GetIndex()]->AddIndex(data->index);
		data->lodStage = stage;
	}

	void InstancingData::UpdateEntityLODStage(Entity *entity, const Vector3 &position)
	{
		InstancingEntity *data = reinterpret_cast<InstancingEntity *>(entity->_instancedData);

		float distance = entity->GetWorldPosition().GetDistance(position);
		Model::LODStage *stage = _model->GetLODStageForDistance(distance / _pivot->GetClipFar());

		if(stage != data->lodStage)
		{
			if(data->lodStage)
				_stages[data->lodStage->GetIndex()]->RemoveIndex(data->index);

			_stages[stage->GetIndex()]->AddIndex(data->index);
			data->lodStage = stage;
		}
	}
}