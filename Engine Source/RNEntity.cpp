//
//  RNEntity.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		_skeleton("sekelton", Object::MemoryPolicy::Retain, std::bind(&Entity::GetSkeleton, this), std::bind(&Entity::SetSkeleton, this, std::placeholders::_1)),
		_instancedData(nullptr)
	{
		AddObservable(&_model);
		AddObservable(&_skeleton);
	}
	
	Entity::Entity(Model *model) :
		Entity()
	{
		SetModel(model);
	}
	
	Entity::Entity(Model *model, const Vector3 &position) :
		Entity()
	{
		SetModel(model);
		SetPosition(position);
	}
	
	Entity::~Entity()
	{
		SafeRelease(_skeleton);
		SafeRelease(_model);
	}
	
	void Entity::Render(Renderer *renderer, Camera *camera)
	{
		SceneNode::Render(renderer, camera);
		
		if(_model)
		{
			float distance = 0.0f;
			Camera *distanceCamera = camera->GetLODCamera();
			
			distance = GetWorldPosition().Distance(distanceCamera->GetWorldPosition());
			distance /= distanceCamera->GetClipFar();
			
			
			RenderingObject object;
			FillRenderingObject(object);
			
			size_t lodStage = _model->GetLODStageForDistance(distance);
			object.skeleton = GetSkeleton();
			
			size_t count = _model->GetMeshCount(lodStage);
			for(size_t i = 0; i < count; i ++)
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
	
		_model.WillChangeValue();
		
		SafeRelease(_model);
		_model = SafeRetain(model);
		
		_model.DidChangeValue();
		
		if(_model)
			SetBoundingBox(_model->GetBoundingBox(), true);
		
		if(_model->GetSkeleton() != nullptr)
			SetSkeleton(Skeleton::WithSkeleton(_model->GetSkeleton()));
		
		Unlock();
	}
	
	void Entity::SetSkeleton(Skeleton *skeleton)
	{
		Lock();
		
		_skeleton.WillChangeValue();

		SafeRelease(_skeleton);
		_skeleton = SafeRetain(skeleton);
		
		_skeleton.DidChangeValue();

		Unlock();
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
