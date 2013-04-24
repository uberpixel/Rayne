//
//  RNWorldAttachment.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNWorldAttachment.h"

namespace RN
{
	RNDeclareMeta(WorldAttachment)
	
	void WorldAttachment::StepWorld(float delta)
	{
	}
	
	void WorldAttachment::SceneNodesUpdated()
	{
	}
	
	void WorldAttachment::BeginCamera(Camera *camera)
	{
	}
	
	void WorldAttachment::WillFinishCamera(Camera *camera)
	{
	}
	
	void WorldAttachment::DidAddSceneNode(SceneNode *node)
	{
	}
	
	void WorldAttachment::WillRemoveSceneNode(SceneNode *node)
	{
	}
}