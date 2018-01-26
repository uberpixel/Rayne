//
//  RNNewtonConstraint.cpp
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNNewtonConstraint.h"
#include "Newton.h"
#include "RNNewtonWorld.h"

#include "dCustomKinematicController.h"

namespace RN
{
	RNDefineMeta(NewtonConstraint, Object)
	RNDefineMeta(NewtonFixedConstraint, NewtonConstraint)
	RNDefineMeta(NewtonKinematicConstraint, NewtonConstraint)
		
	NewtonConstraint::NewtonConstraint() :
		_constraint(nullptr)
	{}
		
	NewtonConstraint::NewtonConstraint(NewtonJoint *constraint) :
		_constraint(constraint)
	{}
		
	NewtonConstraint::~NewtonConstraint()
	{
		if(_constraint)
		{
			::NewtonWorld *netwonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
			NewtonDestroyJoint(netwonInstance, _constraint);
		}
	}
	
	
	NewtonFixedConstraint::NewtonFixedConstraint(NewtonRigidBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, NewtonRigidBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2)
	{
		//TODO: Implement
//		::NewtonWorld *netwonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
//		_constraint = NewtonConstraintCreateUserJoint(netwonInstance, 0, SubmitCallback, body1->GetNewtonBody(), body2->GetNewtonBody());
	}
		
	NewtonFixedConstraint *NewtonFixedConstraint::WithBodiesAndOffsets(NewtonRigidBody *body1, const RN::Vector3 &offset1, const RN::Quaternion &rotation1, NewtonRigidBody *body2, const RN::Vector3 &offset2, const RN::Quaternion &rotation2)
	{
		NewtonFixedConstraint *constraint = new NewtonFixedConstraint(body1, offset1, rotation1, body2, offset2, rotation2);
		return constraint->Autorelease();
	}


	NewtonKinematicConstraint::NewtonKinematicConstraint(class NewtonRigidBody* body, const RN::Vector3& offset)
	{
		_joint = new dCustomKinematicController(body->GetNewtonBody(), dVector(offset.x, offset.y, offset.z, 1.0));

		float mass, Ixx, Iyy, Izz;
		NewtonBodyGetMass(body->GetNewtonBody(), &mass, &Ixx, &Iyy, &Izz);
		_joint->SetMaxAngularFriction(80.0f);
		_joint->SetMaxLinearFriction(40.0f);
	}

	NewtonKinematicConstraint::~NewtonKinematicConstraint()
	{
		delete _joint;
	}

	void NewtonKinematicConstraint::SetPose(Vector3 position, Quaternion rotation)
	{
		_joint->SetTargetPosit(dVector(position.x, position.y, position.z, 1.0));
		_joint->SetTargetRotation(dQuaternion(rotation.w, rotation.x, rotation.y, rotation.z));
	}

	NewtonKinematicConstraint* NewtonKinematicConstraint::WithBodyAndPose(class NewtonRigidBody* body, const RN::Vector3& offset)
	{
		NewtonKinematicConstraint *constraint = new NewtonKinematicConstraint(body, offset);
		return constraint->Autorelease();
	}
}
