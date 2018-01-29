//
//  RNNewtonConstraint.h
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NEWTONCONSTRAINT_H_
#define __RAYNE_NEWTONCONSTRAINT_H_

#include "RNNewton.h"
#include "RNNewtonRigidBody.h"

class NewtonJoint;
class dCustomKinematicController;
class KCJoint;

namespace RN
{
	class NewtonConstraint : public Object
	{
	public:
		NDAPI NewtonConstraint(NewtonJoint *constraint);
		NDAPI NewtonJoint *GetNewtonConstraint() const { return _constraint; }
			
	protected:
		NewtonConstraint();
		~NewtonConstraint() override;

		NewtonJoint *_constraint;
			
		RNDeclareMetaAPI(NewtonConstraint, NDAPI)
	};
		
	class NewtonFixedConstraint : public NewtonConstraint
	{
	public:
		NDAPI NewtonFixedConstraint(NewtonRigidBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, NewtonRigidBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2);
			
		NDAPI static NewtonFixedConstraint *WithBodiesAndOffsets(NewtonRigidBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, NewtonRigidBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2);
			
		RNDeclareMetaAPI(NewtonFixedConstraint, NDAPI)
	};

	class NewtonKinematicConstraint : public NewtonConstraint
	{
	public:
		NDAPI NewtonKinematicConstraint(NewtonRigidBody *body, const RN::Vector3 &offset);
		NDAPI ~NewtonKinematicConstraint();

		NDAPI static NewtonKinematicConstraint *WithBodyAndPose(NewtonRigidBody *body, const RN::Vector3 &offset);

		NDAPI void SetPose(Vector3 position, Quaternion rotation);

	private:
		dCustomKinematicController *_joint;

		RNDeclareMetaAPI(NewtonKinematicConstraint, NDAPI)
	};

	class NewtonKinematicConstraint2 : public NewtonConstraint
	{
	public:
		NDAPI NewtonKinematicConstraint2(NewtonRigidBody *body, const RN::Vector3 &offset);
		NDAPI ~NewtonKinematicConstraint2();

		NDAPI static NewtonKinematicConstraint2 *WithBodyAndPose(NewtonRigidBody *body, const RN::Vector3 &offset);

		NDAPI void SetPose(Vector3 position, Quaternion rotation);

	private:
		KCJoint *_joint;

		RNDeclareMetaAPI(NewtonKinematicConstraint2, NDAPI)
	};
}

#endif /* defined(__RAYNE_NEWTONCONSTRAINT_H_) */
