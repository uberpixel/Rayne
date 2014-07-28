//
//  RNSceneNodeAttachment.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCENENODEATTACHMENT_H__
#define __RAYNE_SCENENODEATTACHMENT_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNVector.h"
#include "RNQuaternion.h"
#include "RNSceneNode.h"

namespace RN
{
	class World;
	
	class SceneNodeAttachment : public Object
	{
	public:
		friend class SceneNode;
		
		RNAPI SceneNodeAttachment();
		RNAPI ~SceneNodeAttachment() override;
		
		RNAPI void SetWorldPosition(const Vector3 &position);
		RNAPI void SetWorldScale(const Vector3 &scale);
		RNAPI void SetWorldRotation(const Quaternion &rotation);
		
		RNAPI Vector3 GetWorldPosition() const;
		RNAPI Vector3 GetWorldScale() const;
		RNAPI Quaternion GetWorldRotation() const;
		
		RNAPI Vector3 GetForward() const;
		RNAPI Vector3 GetUp() const;
		RNAPI Vector3 GetRight() const;
		
		RNAPI World *GetWorld() const;
		RNAPI SceneNode *GetParent() const;
		
		RNAPI virtual void Update(float delta);
		RNAPI virtual void UpdateEditMode(float delta);
		
	protected:
		RNAPI virtual void WillUpdate(SceneNode::ChangeSet changeSet) {}
		RNAPI virtual void DidUpdate(SceneNode::ChangeSet changeSet) {}
		
		RNAPI virtual void UpdateRenderingObject(RenderingObject &object) {}
		
		RNAPI virtual void WillRemoveFromParent() {}
		RNAPI virtual void DidAddToParent() {}
		
	private:
		void __WillUpdate(SceneNode::ChangeSet changeSet);
		void __DidUpdate(SceneNode::ChangeSet changeSet);
		
		SceneNode *_node;
		SceneNode::ChangeSet _consumeChangeSets;
		
		RNDeclareMeta(SceneNodeAttachment);
	};
	
	RNObjectClass(SceneNodeAttachment)
}

#endif /* __RAYNE_SCENENODEATTACHMENT_H__ */
