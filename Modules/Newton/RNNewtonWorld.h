//
//  RNNewtonWorld.h
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NEWTONWORLD_H_
#define __RAYNE_NEWTONWORLD_H_

#include "RNNewton.h"

//#include "RNPhysXMaterial.h"
#include "RNNewtonShape.h"
#include "RNNewtonRigidBody.h"
//#include "RNPhysXConstraint.h"
#include "RNNewtonCharacterController.h"
#include <unordered_set>

class NewtonWorld;

namespace RN
{
	class NewtonWorld : public SceneAttachment
	{
	public:
		NDAPI NewtonWorld(const Vector3 &gravity = Vector3(0.0f, -9.81f, 0.0f), bool debug = false);
		NDAPI ~NewtonWorld();

		NDAPI void SetGravity(const Vector3 &gravity);
		NDAPI Vector3 GetGravity();

		NDAPI void Update(float delta) override;
		NDAPI void SetPaused(bool paused);

//		PXAPI PhysXContactInfo CastRay(const Vector3 &from, const Vector3 &to);

		::NewtonWorld *GetNewtonInstance() const { return _newtonInstance; }

		static NewtonWorld *GetSharedInstance() { return _sharedInstance; }

	private:
		static NewtonWorld *_sharedInstance;

		::NewtonWorld* _newtonInstance;

		bool _paused;
		Vector3 _gravity;

		RNDeclareMetaAPI(NewtonWorld, NDAPI)
	};
}


#endif /* __RAYNE_PHYSXWORLD_H_ */
