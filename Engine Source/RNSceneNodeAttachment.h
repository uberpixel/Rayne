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

namespace RN
{
	class World;
	class SceneNode;
	
	class SceneNodeAttachment : public Object
	{
	public:
		friend class SceneNode;
		
		SceneNodeAttachment();
		~SceneNodeAttachment() override;
		
		void SetPosition(const Vector3 &position);
		void SetScale(const Vector3 &scale);
		void SetRotation(const Quaternion &rotation);
		
		Vector3 GetPosition() const;
		Vector3 GetScale() const;
		Quaternion GetRotation() const;
		
		World *GetWorld() const;
		SceneNode *GetParent() const;
		
		virtual void Update(float delta);
		
	protected:
		virtual void WillUpdate(uint32 changeSet) {}
		virtual void DidUpdate(uint32 changeSet) {}
		
	private:
		void __WillUpdate(uint32 changeSet);
		void __DidUpdate(uint32 changeSet);
		
		SceneNode *_node;
		uint32 _consumeChangeSets;
		
		RNDefineMeta(SceneNodeAttachment, Object);
	};
}

#endif /* __RAYNE_SCENENODEATTACHMENT_H__ */
