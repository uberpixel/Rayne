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

	struct SceneNodeAttachment::ConsumePositionChange
	{
		explicit ConsumePositionChange(SceneNode::ChangeSet &changeSets) :
			changeSets(changeSets), previous(changeSets)
		{
			changeSets |= SceneNode::ChangeSet::Position;
		}

		~ConsumePositionChange() { changeSets = previous; }

		SceneNode::ChangeSet &changeSets;
		SceneNode::ChangeSet previous;
	};

	SceneNodeAttachment::SceneNodeAttachment() :
		_node(nullptr), _consumeChangeSets(0)
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

	void SceneNodeAttachment::__WillUpdate(SceneNode::ChangeSet changeSet)
	{
		if(_consumeChangeSets)
		{
			changeSet &= ~_consumeChangeSets;

			if(changeSet == 0)
				return;
		}

		WillUpdate(changeSet);
	}
	void SceneNodeAttachment::__DidUpdate(SceneNode::ChangeSet changeSet)
	{
		if(_consumeChangeSets)
		{
			changeSet &= ~_consumeChangeSets;

			if(changeSet == 0)
				return;
		}

		DidUpdate(changeSet);
	}


	void SceneNodeAttachment::SetWorldPosition(const PositionType &position)
	{
		RN_DEBUG_ASSERT(_node, "SceneNodeAttachment must be attached to a SceneNode before setting world position.");
		const ConsumePositionChange consumePositionChange(_consumeChangeSets);
		_node->SetWorldPosition(position);
	}
	void SceneNodeAttachment::SetWorldScale(const Vector3 &scale)
	{
		RN_DEBUG_ASSERT(_node, "SceneNodeAttachment must be attached to a SceneNode before setting world scale.");
		const ConsumePositionChange consumePositionChange(_consumeChangeSets);
		_node->SetWorldScale(scale);
	}
	void SceneNodeAttachment::SetWorldRotation(const Quaternion &rotation)
	{
		RN_DEBUG_ASSERT(_node, "SceneNodeAttachment must be attached to a SceneNode before setting world rotation.");
		const ConsumePositionChange consumePositionChange(_consumeChangeSets);
		_node->SetWorldRotation(rotation);
	}
	void SceneNodeAttachment::SetWorldTransform(const PositionType &position, const Quaternion &rotation)
	{
		RN_DEBUG_ASSERT(_node, "SceneNodeAttachment must be attached to a SceneNode before setting world transform.");
		const ConsumePositionChange consumePositionChange(_consumeChangeSets);
		_node->SetWorldTransform(position, rotation);
	}
	void SceneNodeAttachment::SetLocalTransform(const PositionType &position, const Quaternion &rotation)
	{
		RN_DEBUG_ASSERT(_node, "SceneNodeAttachment must be attached before setting local transform.");
		const ConsumePositionChange consumePositionChange(_consumeChangeSets);
		_node->SetTransform(position, rotation);
	}
	PositionType SceneNodeAttachment::GetWorldPosition() const
	{
		RN_DEBUG_ASSERT(_node, "SceneNodeAttachment must be attached to a SceneNode before getting world position.");
		return _node->GetWorldPosition();
	}
	Vector3 SceneNodeAttachment::GetWorldScale() const
	{
		RN_DEBUG_ASSERT(_node, "SceneNodeAttachment must be attached to a SceneNode before getting world scale.");
		return _node->GetWorldScale();
	}
	Quaternion SceneNodeAttachment::GetWorldRotation() const
	{
		RN_DEBUG_ASSERT(_node, "SceneNodeAttachment must be attached to a SceneNode before getting world rotation.");
		return _node->GetWorldRotation();
	}

	Vector3 SceneNodeAttachment::GetForward() const
	{
		RN_DEBUG_ASSERT(_node, "SceneNodeAttachment must be attached to a SceneNode before getting forward vector.");
		return _node->GetForward();
	}
	Vector3 SceneNodeAttachment::GetUp() const
	{
		RN_DEBUG_ASSERT(_node, "SceneNodeAttachment must be attached to a SceneNode before getting up vector.");
		return _node->GetUp();
	}
	Vector3 SceneNodeAttachment::GetRight() const
	{
		RN_DEBUG_ASSERT(_node, "SceneNodeAttachment must be attached to a SceneNode before getting right vector.");
		return _node->GetRight();
	}
} // namespace RN
