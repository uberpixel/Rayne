//
//  RNSceneManager.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneManager.h"

namespace RN
{
	RNDeclareMeta(SceneManager)
	RNDeclareMeta(GenericSceneManager)
	
	SceneManager::SceneManager()
	{
		_renderer = Renderer::SharedInstance();
	}
	
	SceneManager::~SceneManager()
	{
	}
	
	
	
	GenericSceneManager::GenericSceneManager()
	{
		_entityClass = Entity::MetaClass();
	}
	
	GenericSceneManager::~GenericSceneManager()
	{
	}
	
	void GenericSceneManager::AddTransform(Transform *transform)
	{
		_transforms.insert(transform);
	}
	
	void GenericSceneManager::RemoveTransform(Transform *transform)
	{
		_transforms.erase(transform);
	}
	
	void GenericSceneManager::UpdateTransform(Transform *transform)
	{
	}
	
	
	
	void GenericSceneManager::RenderTransform(Camera *camera, Transform *transform)
	{
		if(transform->IsVisibleInCamera(camera))
		{
			if(transform->IsKindOfClass(_entityClass))
			{
				Entity *entity = static_cast<Entity *>(transform);
				if(entity->Model())
				{
					float distance = entity->WorldPosition().Distance(camera->WorldPosition());
					distance /= camera->clipfar;
					
					Model *model = entity->Model();
					uint32 lodStage = model->LODStageForDistance(distance);
					
					RenderingObject object;
					
					object.transform = (Matrix *)&entity->WorldTransform();
					object.skeleton  = entity->Skeleton();
					
					uint32 count = model->Meshes(lodStage);
					for(uint32 i=0; i<count; i++)
					{
						object.mesh = model->MeshAtIndex(lodStage, i);
						object.material = model->MaterialAtIndex(lodStage, i);
						
						_renderer->RenderObject(object);
					}
				}
			}
			
			if(transform->IsKindOfClass(_lightClass))
			{
				_renderer->RenderLight(static_cast<Light *>(transform));
			}
			
			for(machine_uint i=0; i<transform->Childs(); i++)
			{
				Transform *child = transform->ChildAtIndex(i);
				RenderTransform(camera, child);
			}
		}
	}

	void GenericSceneManager::RenderTransforms(Camera *camera)
	{
		for(auto i=_transforms.begin(); i!=_transforms.end(); i++)
		{
			Transform *transform = *i;
			if(!transform->Parent())
			{
				RenderTransform(camera, transform);
			}
		}
	}
}
