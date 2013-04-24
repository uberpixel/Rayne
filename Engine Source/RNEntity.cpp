//
//  RNEntity.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNEntity.h"
#include "RNKernel.h"
#include "RNWorld.h"

namespace RN
{
	RNDeclareMeta(Entity)
	
	Entity::Entity()
	{
		_model = 0;
		_skeleton = 0;
	}
	
	Entity::~Entity()
	{
		if(_model)
			_model->Release();
		
		if(_skeleton)
			_skeleton->Release();
	}
	
	void Entity::Render(Renderer *renderer, Camera *camera)
	{
		if(_model)
		{
			float distance = WorldPosition().Distance(camera->WorldPosition());
			distance /= camera->clipfar;
			
			uint32 lodStage = _model->LODStageForDistance(distance);
			RenderingObject object;
			
			object.transform = (Matrix *)&WorldTransform();
			object.skeleton  = Skeleton();
			
			uint32 count = _model->Meshes(lodStage);
			for(uint32 i=0; i<count; i++)
			{
				object.mesh = _model->MeshAtIndex(lodStage, i);
				object.material = _model->MaterialAtIndex(lodStage, i);
				
				renderer->RenderObject(object);
			}
		}
	}
	
	void Entity::SetModel(class Model *model)
	{
		if(_model)
			_model->Release();
		
		_model = model ? model->Retain() : 0;
		
		if(_model)
		{
			SetBoundingBox(_model->BoundingBox(), true);
		}
	}
	
	void Entity::SetSkeleton(class Skeleton *skeleton)
	{
		if(_skeleton)
			_skeleton->Release();
		
		_skeleton = skeleton ? (class Skeleton *)skeleton->Retain() : 0;
	}
}
