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
	
	void WorldAttachment::UpdateTransform(Transform *transform, float delta)
	{
	}
	
	void WorldAttachment::StepWorld(float delta)
	{
	}
	
	void WorldAttachment::TransformsUpdated()
	{
	}
	
	void WorldAttachment::BeginCamera(Camera *camera)
	{
	}
	
	void WorldAttachment::WillFinishCamera(Camera *camera)
	{
	}
	
	void WorldAttachment::DidAddTransform(Transform *transform)
	{
	}
	
	void WorldAttachment::WillRemoveTransform(Transform *transform)
	{
	}
}