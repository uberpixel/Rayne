//
//  RNSceneNodeAttachment.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneNodeAttachment.h"

namespace RN
{
	RNDefineMeta(SceneNodeAttachment, Object)

	SceneNodeAttachment::SceneNodeAttachment() : _node(nullptr)
	{

	}

	SceneNodeAttachment::~SceneNodeAttachment()
	{

	}

	void SceneNodeAttachment::Update(float delta)
	{

	}

	SceneNode *SceneNodeAttachment::GetParent() const
	{
		return _node;
	}
}
