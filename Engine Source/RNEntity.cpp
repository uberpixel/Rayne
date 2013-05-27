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
		_ignoreDrawing = false;
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
		if(_model && !_ignoreDrawing)
		{
			float distance = 0.0f;
			Camera *distanceCamera = (camera->LODCamera()) ? camera->LODCamera() : camera;
			
			distance = WorldPosition().Distance(distanceCamera->WorldPosition());
			distance /= distanceCamera->clipfar;
			
			
			RenderingObject object;
			
			uint32 lodStage = _model->LODStageForDistance(distance);
			
			object.rotation = (Quaternion*)&WorldRotation();
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
