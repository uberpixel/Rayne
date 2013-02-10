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
#include "RNRendering.h"
#include "RNTransform.h"
#include "RNMaterial.h"
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
		virtual void PostUpdate();
		virtual bool IsVisibleInCamera(Camera *camera);
		
		void SetModel(Model *model);
		
		EntityType Type() const {return _type;}
		Model *Model() const { return _model; }
		
	protected:		
		Entity(EntityType type);

	private:
		class Model *_model;
		EntityType _type;
	};
}

#endif /* __RAYNE_ENTITY_H__ */
