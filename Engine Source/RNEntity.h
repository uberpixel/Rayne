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
#include "RNMaterial.h"
#include "RNModel.h"
#include "RNMesh.h"

namespace RN
{
	class World;
	class Camera;
	class RenderingPipeline;
	
	class Entity : public Object, public Transform
	{
	friend class World;
	friend class RenderingPipeline;
	public:
		typedef enum
		{
			TypeObject,
			TypeLight
		} EntityType;
		
		Entity();
		Entity(Entity *other);
		
		virtual ~Entity();
		
		virtual void Update(float delta);
		virtual bool IsVisibleInCamera(Camera *camera);
		
		void SetModel(Model *model);
		void SetSkeleton(class Skeleton *skeleton);
		
		EntityType Type() const {return _type;}
		Model *Model() const { return _model; }
		Skeleton *Skeleton() const { return _skeleton; }
		
	protected:		
		Entity(EntityType type);

	private:
		class Model *_model;
		class Skeleton *_skeleton;
		EntityType _type;
	};
}

#endif /* __RAYNE_ENTITY_H__ */
