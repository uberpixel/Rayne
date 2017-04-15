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
#include "RNLight.h"
#include "RNSceneAttachment.h"

namespace RN
{
	class Scene : public Object
	{
	public:
		friend class Kernel;
		friend class SceneManager;

		RNAPI ~Scene();

		RNAPI void AddNode(SceneNode *node);
		RNAPI void RemoveNode(SceneNode *node);

		RNAPI void AddAttachment(SceneAttachment *attachment);
		RNAPI void RemoveAttachment(SceneAttachment *attachment);

	protected:
		RNAPI Scene();

		RNAPI virtual void WillBecomeActive();
		RNAPI virtual void DidBecomeActive();

		RNAPI virtual void WillResignActive();
		RNAPI virtual void DidResignActive();

		RNAPI virtual void WillUpdate(float delta);
		RNAPI virtual void DidUpdate(float delta);

		RNAPI virtual void WillRender(Renderer *renderer);
		RNAPI virtual void DidRender(Renderer *renderer);

	private:
		void Update(float delta);
		void Render(Renderer *renderer);

		IntrusiveList<SceneNode> _nodes[3];
		IntrusiveList<Camera> _cameras;
		Array *_lights;
		Array *_attachments;

		__RNDeclareMetaInternal(Scene)
	};
}


#endif /* __RAYNE_SCENE_H__ */
