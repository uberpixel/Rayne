//
//  RNInstancingNode.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INSTANCINGNODE_H__
#define __RAYNE_INSTANCINGNODE_H__

#include "RNBase.h"
#include "RNSceneNode.h"
#include "RNSet.h"
#include "RNCountedSet.h"
#include "RNEntity.h"
#include "RNInstancingData.h"

namespace RN
{
	class InstancingNode : public SceneNode
	{
	public:
		enum
		{
			ModeIncludeZAxis = (1 << 0)
		};
		
		RNAPI InstancingNode();
		RNAPI InstancingNode(Model *model);
		RNAPI ~InstancingNode() override;
		
		RNAPI void SetModel(Model *model);
		RNAPI void AddModel(Model *model);
		RNAPI void SetModels(const Array *models);
		RNAPI void SetModels(const Set *models);
		RNAPI void SetPivot(Camera *pivot);
		RNAPI void SetClipping(bool clipping, float clippingRange = 64);
		RNAPI void SetCellSize(float size);
		
		RNAPI bool IsVisibleInCamera(Camera *camera) override;
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		
		RNAPI bool GetIsClipping() const { return _clipping; }
		RNAPI float GetCellSize() const { return _cellSize; }
		
	protected:
		RNAPI void ChildDidUpdate(SceneNode *child, uint32 changeSet) override;
		RNAPI void WillAddChild(SceneNode *child) override;
		RNAPI void WillRemoveChild(SceneNode *child) override;
		
	private:
		void Initialize();
		void SortChildren();
		void RecreateData();
		void ModelsChanged();
		
		void EntityDidUpdateModel(Object *object, const std::string &key, Dictionary *changes);
		void PivotDidMove(Object *object, const std::string &key, Dictionary *changes);
		void ModelDidUpdate(Model *model);
		
		std::unordered_map<Model *, InstancingData *> _data;
		std::vector<InstancingData *> _rawData;
	
		Set *_models;
		uint32 _mode;
		
		bool _clipping;
		float _clipRange;
		float _cellSize;
		
		Camera *_pivot;
		
		MetaClassBase *_entityClass;
	};
}

#endif /* __RAYNE_INSTANCINGNODE_H__ */
