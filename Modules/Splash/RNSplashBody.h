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
		SPAPI SplashBody(SplashShape *shape);
			
		SPAPI ~SplashBody();
			
		SPAPI static SplashBody *WithShape(SplashShape *shape);
			
		SPAPI void SetLinearVelocity(const Vector3 &velocity);
		SPAPI void SetAngularVelocity(const Vector3 &velocity);

		SPAPI void SetPositionOffset(RN::Vector3 offset);
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet);
		
		void InsertIntoWorld(SplashWorld *world);
		void RemoveFromWorld(SplashWorld *world);
			
	private:
		SplashShape *_shape;
			
		RNDeclareMetaAPI(SplashBody, SPAPI)
	};
}

#endif /* defined(__RAYNE_SPLASHBODY_H_) */
