//
//  RNSplashBody.h
//  Rayne-Splash
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SPLASHBODY_H_
#define __RAYNE_SPLASHBODY_H_

#include "RNSplash.h"
#include "RNSplashShape.h"

namespace RN
{
	class SplashWorld;
	class SplashBody : public SceneNodeAttachment
	{
	public:
		SPAPI SplashBody(SplashShape *shape, float mass);
			
		SPAPI ~SplashBody();
			
		SPAPI static SplashBody *WithShape(SplashShape *shape, float mass);
			
		SPAPI void SetLinearVelocity(const Vector3 &velocity);
		SPAPI void SetAngularVelocity(const Vector3 &velocity);

		SPAPI const Vector3 &GetLinearVelocity() const { return _linearVelocity; }
		SPAPI const Vector3 &GetAngularVelocity() const { return _angularVelocity; }

		SPAPI void AccelerateToTarget(const Vector3 &targetPosition, const Quaternion &targetRotation, float delta);

		SPAPI void AddForce(const RN::Vector3 force);

		SPAPI void SetPositionOffset(const Vector3 &offset);

		SPAPI void Update(float delta) override;
		SPAPI void CalculateVelocities(float delta);
		SPAPI void PrepareCollision(float delta);
		SPAPI void Collide(SplashBody *other, float delta);
		SPAPI void Move(float delta);
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
			
	private:
		Vector3 _offset;
		SplashShape *_shape;
		SplashShape *_transformedShape;

		float _mass;
		Vector3 _linearAcceleration;
		Vector3 _linearVelocity;
		Vector3 _angularVelocity;
			
		RNDeclareMetaAPI(SplashBody, SPAPI)
	};
}

#endif /* defined(__RAYNE_SPLASHBODY_H_) */
