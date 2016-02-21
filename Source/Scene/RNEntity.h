//
//  RNEntity.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ENTITY_H_
#define __RAYNE_ENTITY_H_

#include "../Base/RNBase.h"
#include "../Rendering/RNModel.h"
#include "../Rendering/RNRenderer.h"
#include "RNSceneNode.h"

namespace RN
{
	struct InstancingEntity;

	class Entity : public SceneNode
	{
	public:
		friend class InstancingData;

		RNAPI Entity();
		RNAPI Entity(Model *model);
		RNAPI ~Entity();

		RNAPI void SetModel(Model *model);
		Model *GetModel() const { return _model; }

		RNAPI void Render(Renderer *renderer, Camera *camera) const override;

	private:
		void ClearDrawables();

		Model *_model;
		std::vector<std::vector<Drawable *>> _drawables;
		InstancingEntity *_instancedData;

		__RNDeclareMetaInternal(Entity)
	};
}


#endif /* __RAYNE_ENTITY_H_ */
