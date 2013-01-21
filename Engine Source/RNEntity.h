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
	class Entity : public Object, public Transform
	{
	public:
		Entity();
		Entity(Entity *other);
		
		virtual ~Entity();
		
		virtual void Update(float delta);
		virtual void PostUpdate();
		const RenderingIntent& Intent();
		
		void SetMesh(Mesh *mesh);
		void SetMaterial(Material *material);
		
		Mesh *Mesh() const { return _mesh; }
		Material *Material() const { return _material; }
		
	protected:
		RenderingIntent _renderingIntent;
		
	private:
		class Mesh *_mesh;
		class Material *_material;
	};
}

#endif /* __RAYNE_ENTITY_H__ */
