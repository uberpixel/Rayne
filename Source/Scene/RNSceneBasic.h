//
//  RNSceneBasic.h
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCENEBASIC_H__
#define __RAYNE_SCENEBASIC_H__

#include "RNScene.h"

namespace RN
{
	class SceneBasic : public Scene
	{
	public:
		RNAPI ~SceneBasic();

		RNAPI void AddNode(SceneNode *node) override;
		RNAPI void RemoveNode(SceneNode *node) override;

	protected:
		RNAPI SceneBasic();

		RNAPI void Update(float delta) override;
		RNAPI void Render(Renderer *renderer) override;

	private:
		IntrusiveList<SceneNode> _nodes[3];
		IntrusiveList<Camera> _cameras;
		Array *_nodesToRemove;

		__RNDeclareMetaInternal(SceneBasic)
	};
}


#endif /* __RAYNE_SCENEBASIC_H__ */
