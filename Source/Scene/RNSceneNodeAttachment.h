//
//  RNSceneNodeAttachment.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_SCENENODEATTACHMENT_H__
#define __RAYNE_SCENENODEATTACHMENT_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "RNSceneNode.h"

namespace RN
{
	class SceneNodeAttachment : public Object
	{
	public:
		friend class SceneNode;
		RNAPI ~SceneNodeAttachment();

		RNAPI void SetWorldPosition(const PositionType &position);
		RNAPI void SetWorldScale(const Vector3 &scale);
		RNAPI void SetWorldRotation(const Quaternion &rotation);
		RNAPI void SetWorldTransform(const PositionType &position, const Quaternion &rotation);

		RNAPI PositionType GetWorldPosition() const;
		RNAPI Vector3 GetWorldScale() const;
		RNAPI Quaternion GetWorldRotation() const;

		RNAPI Vector3 GetForward() const;
		RNAPI Vector3 GetUp() const;
		RNAPI Vector3 GetRight() const;

		RNAPI SceneNode *GetParent() const;
		RNAPI virtual void Update(float delta);

	protected:
		RNAPI SceneNodeAttachment();
		RNAPI void SetLocalTransform(const PositionType &position, const Quaternion &rotation);

		virtual void WillUpdate(SceneNode::ChangeSet changeSet) {}
		virtual void DidUpdate(SceneNode::ChangeSet changeSet) {}

	private:
		struct ConsumePositionChange;

		void __WillUpdate(SceneNode::ChangeSet changeSet);
		void __DidUpdate(SceneNode::ChangeSet changeSet);

		SceneNode *_node;
		SceneNode::ChangeSet _consumeChangeSets;

		__RNDeclareMetaInternal(SceneNodeAttachment)
	};
} // namespace RN


#endif /* __RAYNE_SCENENODEATTACHMENT_H__ */
