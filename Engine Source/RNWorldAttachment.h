//
//  RNWorldAttachment.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WORLDATTACHMENT_H__
#define __RAYNE_WORLDATTACHMENT_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNSceneNode.h"
#include "RNCamera.h"

namespace RN
{
	class WorldAttachment : public Object
	{
	public:
		RNAPI virtual void StepWorld(float delta);
		RNAPI virtual void SceneNodesUpdated();
		RNAPI virtual void BeginCamera(Camera *camera);
		RNAPI virtual void WillFinishCamera(Camera *camera);
		
		RNAPI virtual void DidAddSceneNode(SceneNode *node);
		RNAPI virtual void WillRemoveSceneNode(SceneNode *node);
		
		RNDefineConstructorlessMeta(WorldAttachment, Object)
	};
}

#endif /* __RAYNE_WORLDATTACHMENT_H__ */
