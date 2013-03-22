//
//  RNWorld.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorld.h"
#include "RNKernel.h"
#include "RNTransform.h"
#include "RNEntity.h"
#include "RNLightEntity.h"
#include "RNAutoreleasePool.h"
#include "RNThreadPool.h"

namespace RN
{
	World::World()
	{
		_kernel = Kernel::SharedInstance();
		_kernel->SetWorld(this);
		
		_renderer = Renderer::SharedInstance();
		
		_cameraClass = Camera::MetaClass();
		_entityClass = Entity::MetaClass();
		_lightEntityClass = LightEntity::MetaClass();
	}
	
	World::~World()
	{
		_kernel->SetWorld(0);
	}
	
	
	
	void World::Update(float delta)
	{}
	
	void World::TransformsUpdated()
	{}
	
	
	void World::VisitTransform(Camera *camera, Transform *transform)
	{
		if(transform->IsKindOfClass(_entityClass))
		{
			Entity *entity = static_cast<Entity *>(transform);
			
			if(entity->IsVisibleInCamera(camera))
			{
				if(entity->Model())
				{
					Model *model = entity->Model();
					
					RenderingObject object;
					
					object.transform = (Matrix *)&entity->WorldTransform();
					object.skeleton  = entity->Skeleton();
					
					uint32 count = model->Meshes();
					for(uint32 i=0; i<count; i++)
					{
						object.mesh = model->MeshAtIndex(i);
						object.material = model->MaterialForMesh(object.mesh);
						
						_renderer->RenderObject(object);
					}
				}
				
				if(entity->IsKindOfClass(_lightEntityClass))
				{
					_renderer->RenderLight(static_cast<LightEntity *>(transform));
				}
			}
		}
		
		for(machine_uint i=0; i<transform->Childs(); i++)
		{
			Transform *child = transform->ChildAtIndex(i);
			VisitTransform(camera, child);
		}
	}
	
	void World::StepWorld(float delta)
	{
		Update(delta);
		ApplyTransformUpdates();
		
		uint32 size = (uint32)_transforms.size();
		uint32 j = 0;
		
		ThreadPool *pool = ThreadCoordinator::SharedInstance()->GlobalPool();
		std::vector<std::future<void>> results(size);
		
		// Add the Transform updates to the thread pool
		pool->BeginTaskBatch();
		
		for(auto i=_transforms.begin(); i!=_transforms.end(); i++, j++)
		{
			Transform *transform = *i;
			transform->Retain();
			
			results[j] = pool->AddTask([transform, delta]() {
				transform->Update(delta);
				transform->Release();
			});
		}
		
		pool->EndTaskBatch();
		
		// Wait for the updates to complete
		std::for_each(results.begin(), results.end(), std::mem_fn(&std::future<void>::wait));
		
		ApplyTransformUpdates();
		TransformsUpdated();
		
		// Iterate over all cameras and render the visible nodes
		for(auto i=_cameras.begin(); i!=_cameras.end(); i++)
		{
			Camera *camera = *i;
			camera->PostUpdate();
			
			_renderer->BeginCamera(camera);
			
			for(auto j=_transforms.begin(); j!=_transforms.end(); j++)
			{
				Transform *transform = *j;
				VisitTransform(camera, transform);
			}
			
			_renderer->FlushCamera();
		}
	}
	
	void World::ApplyTransformUpdates()
	{
		if(_addedTransforms.size() > 0)
		{
			for(Transform *transform : _addedTransforms)
			{
				if(_transforms.find(transform) == _transforms.end())
				{
					_transforms.insert(transform);
					
					if(transform->IsKindOfClass(_cameraClass))
					{
						Camera *camera = (Camera *)transform;
						_cameras.push_back(camera);
					}
				}
			}
			
			_addedTransforms.clear();
		}
	}
	
	void World::AddTransform(Transform *transform)
	{
		if(!transform)
			return;
		
		_addedTransforms.push_back(transform);
		return;
	}
	
	void World::RemoveTransform(Transform *transform)
	{
		if(!transform)
			return;
		
		auto iterator = _transforms.find(transform);
		if(iterator != _transforms.end())
		{
			_transforms.erase(iterator);
			
			if(transform->IsKindOfClass(_cameraClass))
			{
				Camera *camera = static_cast<Camera *>(transform);
				_cameras.erase(std::remove(_cameras.begin(), _cameras.end(), camera), _cameras.end());
			}
			
			transform->Release();
		}
		
		return;
	}
}
