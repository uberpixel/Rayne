//
//  RNInstancingNode.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		
		InstancingNode();
		InstancingNode(Model *model);
		~InstancingNode() override;
		
		RNAPI void SetModel(Model *model);
		RNAPI void AddModel(Model *model);
		RNAPI void SetModels(const Array *models);
		RNAPI void SetModels(const Set *models);
		RNAPI void SetPivot(SceneNode *pivot);
		RNAPI void SetLimit(size_t lower, size_t upper);
		
		RNAPI bool IsVisibleInCamera(Camera *camera) override;
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		
	protected:
		RNAPI void ChildDidUpdate(SceneNode *child, uint32 changeSet) override;
		RNAPI void WillAddChild(SceneNode *child) override;
		RNAPI void WillRemoveChild(SceneNode *child) override;
		
	private:
		void Initialize();
		void SortChildren();
		void RecreateData();
		void ModelsChanged();
		
		void EntityDidUpdateModel(Object *object, const std::string&key, Dictionary *changes);
		void ModelDidUpdate(Model *model);
		
		std::unordered_map<Model *, InstancingData *> _data;
	
		Set *_models;
		
		uint32 _mode;
		size_t _minimum;
		size_t _limit;
		SceneNode *_pivot;
		
		MetaClassBase *_entityClass;
	};
}

#endif /* __RAYNE_INSTANCINGNODE_H__ */
