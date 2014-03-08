//
//  RNSceneNodeAttachment.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSceneNodeAttachment.h"

namespace RN
{
	RNDefineMeta(SceneNodeAttachment)
	
	SceneNodeAttachment::SceneNodeAttachment() :
		_node(nullptr),
		_consumeChangeSets(0)
	{}
	
	SceneNodeAttachment::~SceneNodeAttachment()
	{}
	
	
	void SceneNodeAttachment::__WillUpdate(SceneNode::ChangeSet changeSet)
	{
		if(_consumeChangeSets)
		{
			changeSet &=~ _consumeChangeSets;
			
			if(changeSet == 0)
				return;
		}
			
		WillUpdate(changeSet);
	}
	void SceneNodeAttachment::__DidUpdate(SceneNode::ChangeSet changeSet)
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
		_consumeChangeSets |= SceneNode::ChangeSet::Position;
		_node->SetWorldPosition(position);
		_consumeChangeSets &= ~SceneNode::ChangeSet::Position;
	}
	void SceneNodeAttachment::SetScale(const Vector3 &scale)
	{
		_consumeChangeSets |= SceneNode::ChangeSet::Position;
		_node->SetWorldScale(scale);
		_consumeChangeSets &= ~SceneNode::ChangeSet::Position;
	}
	void SceneNodeAttachment::SetRotation(const Quaternion &rotation)
	{
		_consumeChangeSets |= SceneNode::ChangeSet::Position;
		_node->SetWorldRotation(rotation);
		_consumeChangeSets &= ~SceneNode::ChangeSet::Position;
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
	void SceneNodeAttachment::UpdateEditMode(float delta)
	{}
}
