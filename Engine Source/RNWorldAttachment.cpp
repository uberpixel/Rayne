//
//  RNWorldAttachment.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorldAttachment.h"

namespace RN
{
	RNDefineMeta(WorldAttachment)
	
	void WorldAttachment::StepWorld(float delta)
	{}
	
	void WorldAttachment::StepWorldEditMode(float delta)
	{}
	
	void WorldAttachment::DidBeginCamera(Camera *camera)
	{}
	
	void WorldAttachment::WillFinishCamera(Camera *camera)
	{}
	
	void WorldAttachment::DidAddSceneNode(SceneNode *node)
	{}
	
	void WorldAttachment::WillRemoveSceneNode(SceneNode *node)
	{}
	
	void WorldAttachment::WillRenderSceneNode(SceneNode *node)
	{}
	
	void WorldAttachment::SceneNodeDidUpdate(SceneNode *node, SceneNode::ChangeSet changeSet)
	{}
}
