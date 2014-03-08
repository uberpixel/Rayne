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
		
		RNAPI void SetPosition(const Vector3 &position);
		RNAPI void SetScale(const Vector3 &scale);
		RNAPI void SetRotation(const Quaternion &rotation);
		
		RNAPI Vector3 GetPosition() const;
		RNAPI Vector3 GetScale() const;
		RNAPI Quaternion GetRotation() const;
		
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
		
		RNDeclareMeta(SceneNodeAttachment, Object);
	};
}

#endif /* __RAYNE_SCENENODEATTACHMENT_H__ */
