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
		RNAPI Entity();
		RNAPI Entity(Model *model);
		RNAPI ~Entity();

		RNAPI void SetModel(Model *model);
		Model *GetModel() const { return _model; }
		RNAPI void SetDrawableRenderingEnabled(size_t index, bool enabled);

		RNAPI bool CanRender(Renderer *renderer, Camera *camera) const override;
		RNAPI void Render(Renderer *renderer, Camera *camera) const override;

		RNAPI void RefreshDrawableSources();

	private:
		void ClearDrawables();
		bool IsDrawableRenderingEnabled(size_t index) const;
		void RebuildRenderableDrawables();

		Model *_model;
		std::vector<uint8> _drawableRenderingEnabled;
#if RN_MODEL_LOD_DISABLED
		std::vector<Drawable *> _drawables;
		Drawable::PreparedDrawList _renderableDrawables;
#else
		std::vector<std::vector<Drawable *>> _drawables;
		std::deque<Drawable::PreparedDrawList> _renderableDrawables;
#endif

		__RNDeclareMetaInternal(Entity)
	};
} // namespace RN


#endif /* __RAYNE_ENTITY_H_ */
