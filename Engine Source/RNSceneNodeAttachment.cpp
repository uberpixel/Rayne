//
//  RNSceneNodeAttachment.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneNodeAttachment.h"
#include "RNSceneNode.h"

namespace RN
{
	RNDeclareMeta(SceneNodeAttachment)
	
	SceneNodeAttachment::SceneNodeAttachment() :
		_node(nullptr),
		_consumeChangeSets(0)
	{}
	
	SceneNodeAttachment::~SceneNodeAttachment()
	{}
	
	
	void SceneNodeAttachment::__WillUpdate(uint32 changeSet)
	{
		if(_consumeChangeSets)
		{
			changeSet &=~ _consumeChangeSets;
			
			if(changeSet == 0)
				return;
		}
			
		WillUpdate(changeSet);
	}
	void SceneNodeAttachment::__DidUpdate(uint32 changeSet)
	{
		if(_consumeChangeSets)
		{
			changeSet &=~ _consumeChangeSets;
			
			if(changeSet == 0)
				return;
		}
		
		DidUpdate(changeSet);
	}
	
	
	void SceneNodeAttachment::SetPosition(const Vector3 &position)
	{
		_consumeChangeSets |= SceneNode::ChangedPosition;
		_node->SetWorldPosition(position);
		_consumeChangeSets &= ~SceneNode::ChangedPosition;
	}
	void SceneNodeAttachment::SetScale(const Vector3 &scale)
	{
		_consumeChangeSets |= SceneNode::ChangedPosition;
		_node->SetWorldScale(scale);
		_consumeChangeSets &= ~SceneNode::ChangedPosition;
	}
	void SceneNodeAttachment::SetRotation(const Quaternion &rotation)
	{
		_consumeChangeSets |= SceneNode::ChangedPosition;
		_node->SetWorldRotation(rotation);
		_consumeChangeSets &= ~SceneNode::ChangedPosition;
	}
	
	Vector3 SceneNodeAttachment::GetPosition() const
	{
		return _node->GetWorldPosition();
	}
	Vector3 SceneNodeAttachment::GetScale() const
	{
		return _node->GetWorldScale();
	}
	Quaternion SceneNodeAttachment::GetRotation() const
	{
		return _node->GetWorldRotation();
	}
	
	
	World *SceneNodeAttachment::GetWorld() const
	{
		return _node->GetWorld();
	}
	SceneNode *SceneNodeAttachment::GetParent() const
	{
		return _node;
	}
	
	void SceneNodeAttachment::Update(float delta)
	{}
}
