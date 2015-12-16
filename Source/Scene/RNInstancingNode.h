//
//  RNInstancingNode.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INSTANCINGNODE_H_
#define __RAYNE_INSTANCINGNODE_H_

#include "../Base/RNBase.h"
#include "../Rendering/RNInstancingData.h"
#include "../Objects/RNSet.h"
#include "../Objects/RNArray.h"
#include "RNSceneNode.h"
#include "RNEntity.h"

namespace RN
{
	class InstancingNode : public SceneNode
	{
	public:
		RN_OPTIONS(Mode, uint32,
				   IncludeYAxis = (1 << 0),
				   Thinning = (1 << 1),
				   Clipping = (1 << 2));

		RNAPI InstancingNode();
		RNAPI InstancingNode(Model *model);
		RNAPI ~InstancingNode() override;

		RNAPI void SetModel(Model *model);
		RNAPI void AddModel(Model *model);
		RNAPI void SetModels(const Array *models);
		RNAPI void SetModels(const Set *models);
		RNAPI void SetPivot(Camera *pivot);
		RNAPI void SetMode(Mode mode);
		RNAPI void SetCellSize(float size);
		RNAPI void SetClippingRange(float range);
		RNAPI void SetThinningRange(float range);

		RNAPI void Update(float delta) override;

		RNAPI bool CanRender(Renderer *renderer, Camera *camera) const override;
		RNAPI void Render(Renderer *renderer, Camera *camera) const override;

		RNAPI float GetCellSize() const { return _cellSize; }
		RNAPI float GetClipRange() const { return _clipRange; }
		RNAPI float GetThinRange() const { return _thinRange; }
		RNAPI Camera *GetPivot() const { return _pivot; }
		RNAPI Mode GetMode() const { return _mode; }

	protected:
		RNAPI void Dealloc() override;
		RNAPI void ChildDidUpdate(SceneNode *child, ChangeSet changeSet) override;
		RNAPI void WillAddChild(SceneNode *child) override;
		RNAPI void WillRemoveChild(SceneNode *child) override;

	private:
		void ModelsChanged();
		void UpdateData(InstancingData *data);

		void EntityDidUpdateModel(Object *object, const std::string &key, Dictionary *changes);
		void PivotDidMove(Object *object, const std::string &key, Dictionary *changes);

		std::unordered_map<Model *, InstancingData *> _data;
		std::vector<InstancingData *> _rawData;

		Set *_models;
		Mode _mode;

		Observable<float, InstancingNode> _clipRange;
		Observable<float, InstancingNode> _thinRange;
		Observable<float, InstancingNode> _cellSize;

		Camera *_pivot;

		MetaClass *_entityClass;

		RNDeclareMeta(InstancingNode)
	};
}


#endif /* __RAYNE_INSTANCINGNODE_H_ */
