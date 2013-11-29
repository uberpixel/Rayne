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
#include "RNHit.h"

namespace RN
{
	RNDeclareMeta(Entity)
	
	Entity::Entity() :
		_model("model", Object::MemoryPolicy::Retain, std::bind(&Entity::GetModel, this), std::bind(&Entity::SetModel, this, std::placeholders::_1)),
		_skeleton("sekelton", Object::MemoryPolicy::Retain, std::bind(&Entity::GetSkeleton, this), std::bind(&Entity::SetSkeleton, this, std::placeholders::_1))
	{
		AddObservable(&_model);
		AddObservable(&_skeleton);
		
		_model = nullptr;
		_skeleton = nullptr;
		
		_ignoreDrawing = false;
	}
	
	Entity::~Entity()
	{
		SafeRelease(_model);
		SafeRelease(_skeleton);
	}
	
	void Entity::Render(Renderer *renderer, Camera *camera)
	{
		SceneNode::Render(renderer, camera);
		
		if(_model && !_ignoreDrawing)
		{
			float distance = 0.0f;
			Camera *distanceCamera = (camera->GetLODCamera()) ? camera->GetLODCamera() : camera;
			
			distance = GetWorldPosition().Distance(distanceCamera->GetWorldPosition());
			distance /= distanceCamera->clipfar;
			
			
			RenderingObject object;
			FillRenderingObject(object);
			
			uint32 lodStage = _model->GetLODStageForDistance(distance);
			object.skeleton = GetSkeleton();
			
			uint32 count = _model->GetMeshCount(lodStage);
			for(uint32 i=0; i<count; i++)
			{
				object.mesh = _model->GetMeshAtIndex(lodStage, i);
				object.material = _model->GetMaterialAtIndex(lodStage, i);
				
				renderer->RenderObject(object);
			}
		}
	}
	
	void Entity::SetModel(Model *model)
	{
		Lock();
		
		if(_model)
			_model->Release();
		
		_model = model ? model->Retain() : 0;
		
		if(_model)
			SetBoundingBox(_model->GetBoundingBox(), true);
		
		Unlock();
		DidUpdate(ChangedGeneric);
	}
	
	void Entity::SetSkeleton(Skeleton *skeleton)
	{
		Lock();
		
		if(_skeleton)
			_skeleton->Release();
		
		_skeleton = skeleton ? (Skeleton *)skeleton->Retain() : nullptr;
		
		Unlock();
		DidUpdate(ChangedGeneric);
	}
	
	Hit Entity::CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode)
	{
		Hit hit;
		
		if(!_model)
			return hit;
			
		if(GetBoundingSphere().IntersectsRay(position, direction))
		{
			Matrix matModelInv = GetWorldTransform().GetInverse();
			
			Vector3 temppos = matModelInv.Transform(position);
			Vector4 tempdir = matModelInv.Transform(Vector4(direction, 0.0f));
			
			size_t meshcount = _model->GetMeshCount(0);
			
			for(int i = 0; i < meshcount; i++)
			{
				Hit result = _model->GetMeshAtIndex(0, i)->IntersectsRay(temppos, Vector3(tempdir), mode);
				
				result.node = this;
				result.meshid = i;
				
				if(result.distance >= 0.0f)
				{
					if(hit.distance < 0.0f)
						hit = result;
					
					if(result.distance < hit.distance)
						hit = result;
				}
			}
		}
		return hit;
	}
}
