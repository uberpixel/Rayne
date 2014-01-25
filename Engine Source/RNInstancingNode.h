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
		struct Mode : public Enum<uint32>
		{
			Mode()
			{}
			Mode(int value) :
				Enum(value)
			{}
			
			enum
			{
				IncludeYAxis = (1 << 0),
				Thinning = (1 << 1),
				Clipping = (1 << 2)
			};
		};
		
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
		
		RNAPI bool IsVisibleInCamera(Camera *camera) override;
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		
		RNAPI float GetCellSize() const { return _cellSize; }
		RNAPI float GetClipRange() const { return _clipRange; }
		RNAPI float GetThinRange() const { return _thinRange; }
		RNAPI Mode GetMode() const { return _mode; }
		
	protected:
		RNAPI void ChildDidUpdate(SceneNode *child, ChangeSet changeSet) override;
		RNAPI void WillAddChild(SceneNode *child) override;
		RNAPI void WillRemoveChild(SceneNode *child) override;
		
	private:
		void Initialize();
		void ModelsChanged();
		void UpdateData(InstancingData *data);
		
		void EntityDidUpdateModel(Object *object, const std::string &key, Dictionary *changes);
		void PivotDidMove(Object *object, const std::string &key, Dictionary *changes);
		
		std::unordered_map<Model *, InstancingData *> _data;
		std::vector<InstancingData *> _rawData;
	
		Set *_models;
		Mode _mode;
		
		float _clipRange;
		float _thinRange;
		float _cellSize;
		
		Camera *_pivot;
		
		MetaClassBase *_entityClass;
	};
}

#endif /* __RAYNE_INSTANCINGNODE_H__ */
