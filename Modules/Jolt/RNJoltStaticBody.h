//
//  RNJoltStaticBody.h
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTSTATICBODY_H_
#define __RAYNE_JOLTSTATICBODY_H_

#include "RNJoltCollisionObject.h"

namespace JPH
{
	class BodyID;
}

namespace RN
{
	class JoltShape;
	class JoltStaticBody : public JoltCollisionObject
	{
	public:
		JTAPI JoltStaticBody(JoltShape *shape, JoltWorld *world = nullptr);
		JTAPI ~JoltStaticBody() override;

		JTAPI static JoltStaticBody *WithShape(JoltShape *shape, JoltWorld *world = nullptr);

		JTAPI void UpdatePosition() override;

		JTAPI void SetCollisionFilter(uint32 group, uint32 mask) override;

		JTAPI void SetFriction(float friction);
		JTAPI void SetRestitution(float restitution);
		JTAPI void InvalidateContactCache();

		JTAPI JPH::BodyID *GetJoltActor() const { return _actor; }

	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;

	private:
		JoltShape *_shape;
		JPH::BodyID *_actor;

		RNDeclareMetaAPI(JoltStaticBody, JTAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_JOLTSTATICBODY_H_) */
