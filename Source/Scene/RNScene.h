//
//  RNScene.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCENE_H__
#define __RAYNE_SCENE_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Rendering/RNRenderer.h"
#include "RNSceneNode.h"
#include "RNCamera.h"

namespace RN
{
	class Scene : public Object
	{
	public:
		RNAPI void Update(float delta);
		RNAPI void Render(Renderer *renderer);

		RNAPI void AddNode(SceneNode *node);
		RNAPI void RemoveNode(SceneNode *node);

	private:
		IntrusiveList<SceneNode> _nodes[3];
		IntrusiveList<Camera> _cameras;

		RNDeclareMeta(Scene)
	};
}


#endif /* __RAYNE_SCENE_H__ */
