//
//  RNSceneWithVisibilityLists.h
//  Rayne
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCENEWITHVISIBILITYLISTS_H__
#define __RAYNE_SCENEWITHVISIBILITYLISTS_H__

#include "RNScene.h"

namespace RN
{
	class SceneWithVisibilityLists : public Scene
	{
	public:
		RNAPI ~SceneWithVisibilityLists();
		
		RNAPI void AddNode(SceneNode *node) override;
		RNAPI void RemoveNode(SceneNode *node) override;

	protected:
		RNAPI SceneWithVisibilityLists();

		RNAPI void Update(float delta) override;
		RNAPI void Render(Renderer *renderer) override;

		IntrusiveList<SceneNode> _nodes[3];
		IntrusiveList<Camera> _cameras;

		__RNDeclareMetaInternal(SceneWithVisibilityLists)
	};
}


#endif /* __RAYNE_SCENEWITHVISIBILITYLISTS_H__ */
