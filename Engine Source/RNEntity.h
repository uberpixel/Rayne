//
//  RNEntity.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ENTITY_H__
#define __RAYNE_ENTITY_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNTransform.h"
#include "RNCamera.h"
#include "RNMaterial.h"
#include "RNModel.h"
#include "RNSkeleton.h"
#include "RNMesh.h"

namespace RN
{
	class World;
	class Entity : public Transform
	{
	friend class World;
	public:
		Entity();
		virtual ~Entity();
		
		virtual void Update(float delta);
		virtual bool IsVisibleInCamera(Camera *camera);
		
		void SetModel(Model *model);
		void SetSkeleton(class Skeleton *skeleton);
		void SetAction(const std::function<void (Entity *, float)>& action);
		
		Model *Model() const { return _model; }
		Skeleton *Skeleton() const { return _skeleton; }

	private:
		class Model *_model;
		class Skeleton *_skeleton;
		
		std::function<void (Entity *, float)> _action;
		
		RNDefineMeta(Entity, Transform)
	};
}

#endif /* __RAYNE_ENTITY_H__ */
