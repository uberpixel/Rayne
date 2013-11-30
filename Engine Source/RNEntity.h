//
//  RNEntity.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ENTITY_H__
#define __RAYNE_ENTITY_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNSceneNode.h"
#include "RNCamera.h"
#include "RNMaterial.h"
#include "RNModel.h"
#include "RNSkeleton.h"
#include "RNMesh.h"

namespace RN
{
	class World;
	class InstancingNode;
	
	class Entity : public SceneNode
	{
	friend class World;
	friend class InstancingNode;
	public:
		RNAPI Entity();
		RNAPI ~Entity() override;
		
		RNAPI void SetModel(Model *model);
		RNAPI void SetSkeleton(class Skeleton *skeleton);
		
		RNAPI Hit CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode = Hit::HitMode::IgnoreNone) override;
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		RNAPI bool IsVisibleInCamera(Camera *camera) override;
		
		Model *GetModel() { return _model; }
		Skeleton *GetSkeleton() { return _skeleton; }

	private:
		Observable<Model *>_model;
		Observable<Skeleton *>_skeleton;
		
		bool _ignoreDrawing;
		
		RNDefineMetaWithTraits(Entity, SceneNode, MetaClassTraitCronstructable)
	};
}

#endif /* __RAYNE_ENTITY_H__ */
