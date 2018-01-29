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
#include "KCJoint.h"

namespace RN
{
	RNDefineMeta(NewtonConstraint, Object)
	RNDefineMeta(NewtonFixedConstraint, NewtonConstraint)
	RNDefineMeta(NewtonKinematicConstraint, NewtonConstraint)
	RNDefineMeta(NewtonKinematicConstraint2, NewtonConstraint)
		
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


	NewtonKinematicConstraint2::NewtonKinematicConstraint2(class NewtonRigidBody* body, const RN::Vector3& offset)
	{
		::NewtonWorld *netwonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();

		_joint = new KCJoint();
		_joint->m_pickMode = 1;
		_joint->m_maxLinearFriction = 1000;
		_joint->m_maxAngularFriction = 100;
		_joint->m_body0 = body->GetNewtonBody();
		NewtonBodyGetPosition(_joint->m_body0, &_joint->m_targetPosit.m_x);
		_joint->m_joint = NewtonConstraintCreateUserJoint(netwonInstance, 6, KCJoint::KCJointCallback, _joint->m_body0, 0);
		NewtonUserJointSetSolverModel(_joint->m_joint, 2);
		NewtonJointSetUserData(_joint->m_joint, _joint);
		NewtonBodySetAutoSleep(_joint->m_body0, 0);
	}

	NewtonKinematicConstraint2::~NewtonKinematicConstraint2()
	{
		::NewtonWorld *netwonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		NewtonDestroyJoint(netwonInstance, _joint->m_joint);
		delete _joint;
	}

	void NewtonKinematicConstraint2::SetPose(Vector3 position, Quaternion rotation)
	{
		_joint->m_targetPosit = dVector(position.x, position.y, position.z, 1.0);
		_joint->m_targetRot = dQuaternion(rotation.w, rotation.x, rotation.y, rotation.z);
		NewtonBodySetSleepState(_joint->m_body0, 0);
	}

	NewtonKinematicConstraint2* NewtonKinematicConstraint2::WithBodyAndPose(class NewtonRigidBody* body, const RN::Vector3& offset)
	{
		NewtonKinematicConstraint2 *constraint = new NewtonKinematicConstraint2(body, offset);
		return constraint->Autorelease();
	}
}
