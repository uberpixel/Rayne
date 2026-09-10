//
//  RNJoltConstraint.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//
#include "RNJoltConstraint.h"
#include "RNJoltScaledMotorConstraint.h"
#include "RNJoltInternals.h"
#include "RNJoltWorld.h"

#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/Constraints/TwoBodyConstraint.h>
#include <Jolt/Physics/Constraints/MotorSettings.h>
#include <Jolt/Physics/Constraints/SpringSettings.h>

namespace RN
{
	RNDefineMeta(JoltConstraint, Object)
	RNDefineMeta(JoltPointConstraint, JoltConstraint)
	RNDefineMeta(JoltFixedConstraint, JoltConstraint)
	RNDefineMeta(JoltDistanceConstraint, JoltConstraint)
	RNDefineMeta(JoltSliderConstraint, JoltConstraint)
	RNDefineMeta(JoltHingeConstraint, JoltConstraint)
	RNDefineMeta(JoltSixDOFConstraint, JoltConstraint)

	JoltConstraint::JoltConstraint() :
		_constraint(nullptr),
		_bodyPairCollisionBody1(JPH::BodyID::cInvalidBodyID),
		_bodyPairCollisionBody2(JPH::BodyID::cInvalidBodyID),
		_bodyPairCollisionDisabled(false),
		_bodyPairConnectedBodyCollisionFilteringEnabled(false),
		_collisionsEnabled(false),
		_connectedBodyCollisionFilteringEnabled(false)
	{
	}

	void JoltConstraint::SetSolverIterationCount(uint32 positionIterations, uint32 velocityIterations)
	{
		if(!_constraint) return;
		if(velocityIterations > 0) _constraint->SetNumVelocityStepsOverride(velocityIterations);
		if(positionIterations > 0) _constraint->SetNumPositionStepsOverride(positionIterations);
	}

	JoltConstraint::~JoltConstraint()
	{
		ResetStoredBodyPairCollisionState();

		if(_constraint)
		{
			//Remove from physics system which will also release the constraint
			if(JoltWorld *world = JoltWorld::GetSharedInstance())
			{
				_constraint->SetEnabled(false);
				world->GetJoltInstance()->RemoveConstraint(_constraint);
			}
		}
	}

	void JoltConstraint::InvalidateJoltConstraint(JPH::Constraint *constraint)
	{
		if(_constraint != constraint) return;

		ResetStoredBodyPairCollisionState();
		_constraint = nullptr;
	}

	void JoltConstraint::SetConstraint(JPH::Constraint *constraint, const JPH::BodyID &bodyID1, const JPH::BodyID &bodyID2)
	{
		if(_constraint == constraint) return;
		ResetStoredBodyPairCollisionState();

		if(_constraint)
		{
			JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
			physics->RemoveConstraint(_constraint);
			_constraint = nullptr;
		}
		_constraint = constraint;
		if(_constraint)
		{
			_constraint->SetUserData(reinterpret_cast<uint64>(static_cast<JoltConstraintOwner *>(this)));
			JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
			physics->AddConstraint(_constraint);
		}

		if(_constraint && !bodyID1.IsInvalid() && !bodyID2.IsInvalid() && bodyID1 != bodyID2)
		{
			_bodyPairCollisionBody1 = bodyID1.GetIndexAndSequenceNumber();
			_bodyPairCollisionBody2 = bodyID2.GetIndexAndSequenceNumber();
			UpdateBodyPairCollisionState();
		}
	}

	void JoltConstraint::ActivateConstrainedBodies()
	{
		if(!_constraint || _constraint->GetType() != JPH::EConstraintType::TwoBodyConstraint) return;

		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		bodyInterface.ActivateConstraint(static_cast<JPH::TwoBodyConstraint *>(_constraint));
	}

	void JoltConstraint::SetEnabled(bool enabled)
	{
		if(_constraint) _constraint->SetEnabled(enabled);
	}

	void JoltConstraint::SetCollisionsEnabled(bool enabled)
	{
		if(_collisionsEnabled == enabled) return;

		_collisionsEnabled = enabled;
		UpdateBodyPairCollisionState();
	}

	void JoltConstraint::SetConnectedBodyCollisionFilteringEnabled(bool enabled)
	{
		if(_connectedBodyCollisionFilteringEnabled == enabled) return;

		_connectedBodyCollisionFilteringEnabled = enabled;
		UpdateBodyPairCollisionState();
	}

	void JoltConstraint::ResetStoredBodyPairCollisionState()
	{
		if(_bodyPairConnectedBodyCollisionFilteringEnabled)
		{
			if(JoltWorld *world = JoltWorld::GetSharedInstance())
			{
				world->SetConnectedBodyCollisionFilteringEnabled(JPH::BodyID(_bodyPairCollisionBody1), JPH::BodyID(_bodyPairCollisionBody2), false);
			}
			_bodyPairConnectedBodyCollisionFilteringEnabled = false;
		}

		if(_bodyPairCollisionDisabled)
		{
			SetStoredBodyPairCollisionEnabled(true);
			_bodyPairCollisionDisabled = false;
		}

		_bodyPairCollisionBody1 = JPH::BodyID::cInvalidBodyID;
		_bodyPairCollisionBody2 = JPH::BodyID::cInvalidBodyID;
	}

	void JoltConstraint::UpdateBodyPairCollisionState()
	{
		if(!HasStoredBodyPair()) return;

		bool shouldDisable = !_collisionsEnabled;
		if(_bodyPairCollisionDisabled != shouldDisable)
		{
			SetStoredBodyPairCollisionEnabled(!shouldDisable);
			_bodyPairCollisionDisabled = shouldDisable;
		}

		bool shouldUseConnectedBodyCollisionFiltering = shouldDisable && _connectedBodyCollisionFilteringEnabled;
		if(_bodyPairConnectedBodyCollisionFilteringEnabled == shouldUseConnectedBodyCollisionFiltering) return;

		if(JoltWorld *world = JoltWorld::GetSharedInstance())
		{
			world->SetConnectedBodyCollisionFilteringEnabled(JPH::BodyID(_bodyPairCollisionBody1), JPH::BodyID(_bodyPairCollisionBody2), shouldUseConnectedBodyCollisionFiltering);
		}
		_bodyPairConnectedBodyCollisionFilteringEnabled = shouldUseConnectedBodyCollisionFiltering;
	}

	void JoltConstraint::SetStoredBodyPairCollisionEnabled(bool enabled)
	{
		if(!HasStoredBodyPair()) return;

		if(JoltWorld *world = JoltWorld::GetSharedInstance())
		{
			world->SetBodyPairCollisionEnabled(JPH::BodyID(_bodyPairCollisionBody1), JPH::BodyID(_bodyPairCollisionBody2), enabled);
		}
	}

	bool JoltConstraint::HasStoredBodyPair() const
	{
		return _bodyPairCollisionBody1 != JPH::BodyID::cInvalidBodyID &&
			_bodyPairCollisionBody2 != JPH::BodyID::cInvalidBodyID &&
			_bodyPairCollisionBody1 != _bodyPairCollisionBody2;
	}

	static JPH::Quat ToJoltQuat(const Quaternion &q)
	{
		return JPH::Quat(q.x, q.y, q.z, q.w);
	}

	static JPH::Vec3 ToJoltVec3(const Vector3 &v)
	{
		return JPH::Vec3(v.x, v.y, v.z);
	}

	JoltPointConstraint::JoltPointConstraint(JoltDynamicBody *body1, const JoltPosition &globalPoint1, JoltDynamicBody *body2, const JoltPosition &globalPoint2)
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		JPH::BodyID bodyID1 = body1 && body1->GetJoltActor() ? *body1->GetJoltActor() : JPH::BodyID();
		JPH::BodyID bodyID2 = body2 && body2->GetJoltActor() ? *body2->GetJoltActor() : JPH::BodyID();
		RN_ASSERT(!bodyID1.IsInvalid() && !bodyID2.IsInvalid(), "Invalid bodies for constraint creation");

		JPH::PointConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1 = JoltConversions::ToJoltPosition(globalPoint1);
		settings.mPoint2 = JoltConversions::ToJoltPosition(globalPoint2);
		SetConstraint(bodyInterface.CreateConstraint(&settings, bodyID1, bodyID2), bodyID1, bodyID2);
	}

	JoltPointConstraint *JoltPointConstraint::WithBodiesAndGlobalPoints(JoltDynamicBody *body1, const JoltPosition &globalPoint1, JoltDynamicBody *body2, const JoltPosition &globalPoint2)
	{
		JoltPointConstraint *constraint = new JoltPointConstraint(body1, globalPoint1, body2, globalPoint2);
		return constraint->Autorelease();
	}

	JoltFixedConstraint::JoltFixedConstraint(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2)
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		JPH::BodyID bodyID1 = body1 && body1->GetJoltActor() ? *body1->GetJoltActor() : JPH::BodyID();
		JPH::BodyID bodyID2 = body2 && body2->GetJoltActor() ? *body2->GetJoltActor() : JPH::BodyID();
		RN_ASSERT(!bodyID1.IsInvalid() && !bodyID2.IsInvalid(), "Invalid bodies for constraint creation");

		JPH::FixedConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mAutoDetectPoint = false;
		settings.mPoint1 = JoltConversions::ToJoltPosition(globalPosition1);
		settings.mPoint2 = JoltConversions::ToJoltPosition(globalPosition2);
		settings.mAxisX1 = JoltConversions::ToJoltVector(worldRotation1.GetRotatedVector(Vector3(1.0f, 0.0f, 0.0f)));
		settings.mAxisY1 = JoltConversions::ToJoltVector(worldRotation1.GetRotatedVector(Vector3(0.0f, 1.0f, 0.0f)));
		settings.mAxisX2 = JoltConversions::ToJoltVector(worldRotation2.GetRotatedVector(Vector3(1.0f, 0.0f, 0.0f)));
		settings.mAxisY2 = JoltConversions::ToJoltVector(worldRotation2.GetRotatedVector(Vector3(0.0f, 1.0f, 0.0f)));
		SetConstraint(bodyInterface.CreateConstraint(&settings, bodyID1, bodyID2), bodyID1, bodyID2);
	}

	JoltFixedConstraint *JoltFixedConstraint::WithBodiesAndGlobalFrames(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2)
	{
		JoltFixedConstraint *constraint = new JoltFixedConstraint(body1, globalPosition1, worldRotation1, body2, globalPosition2, worldRotation2);
		return constraint->Autorelease();
	}

	JoltDistanceConstraint::JoltDistanceConstraint(JoltDynamicBody *body1, const JoltPosition &globalPoint1, JoltDynamicBody *body2, const JoltPosition &globalPoint2, float minDistance, float maxDistance)
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		JPH::BodyID bodyID1 = body1 && body1->GetJoltActor() ? *body1->GetJoltActor() : JPH::BodyID();
		JPH::BodyID bodyID2 = body2 && body2->GetJoltActor() ? *body2->GetJoltActor() : JPH::BodyID();
		RN_ASSERT(!bodyID1.IsInvalid() && !bodyID2.IsInvalid(), "Invalid bodies for constraint creation");

		JPH::DistanceConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1 = JoltConversions::ToJoltPosition(globalPoint1);
		settings.mPoint2 = JoltConversions::ToJoltPosition(globalPoint2);
		settings.mMinDistance = minDistance;
		settings.mMaxDistance = maxDistance;
		SetConstraint(bodyInterface.CreateConstraint(&settings, bodyID1, bodyID2), bodyID1, bodyID2);
	}

	JoltDistanceConstraint *JoltDistanceConstraint::WithBodiesAndGlobalPoints(JoltDynamicBody *body1, const JoltPosition &globalPoint1, JoltDynamicBody *body2, const JoltPosition &globalPoint2, float minDistance, float maxDistance)
	{
		JoltDistanceConstraint *constraint = new JoltDistanceConstraint(body1, globalPoint1, body2, globalPoint2, minDistance, maxDistance);
		return constraint->Autorelease();
	}

	Vector3 JoltSliderConstraint::GetAxisVector(Axis axis)
	{
		if(axis == Axis::Y) return Vector3(0.0f, 1.0f, 0.0f);
		if(axis == Axis::Z) return Vector3(0.0f, 0.0f, 1.0f);
		return Vector3(1.0f, 0.0f, 0.0f);
	}

	Vector3 JoltSliderConstraint::GetNormalVector(Axis axis)
	{
		if(axis == Axis::Y) return Vector3(1.0f, 0.0f, 0.0f);
		return Vector3(0.0f, 1.0f, 0.0f);
	}

	JoltSliderConstraint::JoltSliderConstraint(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2, Axis axis)
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		JPH::BodyID bodyID1 = body1 && body1->GetJoltActor() ? *body1->GetJoltActor() : JPH::BodyID();
		JPH::BodyID bodyID2 = body2 && body2->GetJoltActor() ? *body2->GetJoltActor() : JPH::BodyID();
		RN_ASSERT(!bodyID1.IsInvalid() && !bodyID2.IsInvalid(), "Invalid bodies for constraint creation");

		auto getSafeRotation = [](const Quaternion &rotation) -> Quaternion {
			if(!rotation.IsValid()) return Quaternion();

			Quaternion result(rotation);
			result.Normalize();
			return result.IsValid() ? result : Quaternion();
		};
		auto getAxis = [](const Quaternion &rotation, const Vector3 &fallbackAxis) -> Vector3 {
			Vector3 axis = rotation.GetRotatedVector(fallbackAxis);
			if(!axis.IsValid() || axis.GetSquaredLength() <= k::EpsilonFloat) return fallbackAxis;

			axis.Normalize();
			return axis.IsValid() ? axis : fallbackAxis;
		};

		Quaternion normalizedWorldRotation1 = getSafeRotation(worldRotation1);
		Quaternion normalizedWorldRotation2 = getSafeRotation(worldRotation2);
		Vector3 sliderAxis = GetAxisVector(axis);
		Vector3 normalAxis = GetNormalVector(axis);

		JPH::SliderConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mAutoDetectPoint = false;
		settings.mPoint1 = JoltConversions::ToJoltPosition(globalPosition1);
		settings.mSliderAxis1 = JoltConversions::ToJoltVector(getAxis(normalizedWorldRotation1, sliderAxis));
		settings.mNormalAxis1 = JoltConversions::ToJoltVector(getAxis(normalizedWorldRotation1, normalAxis));
		settings.mPoint2 = JoltConversions::ToJoltPosition(globalPosition2);
		settings.mSliderAxis2 = JoltConversions::ToJoltVector(getAxis(normalizedWorldRotation2, sliderAxis));
		settings.mNormalAxis2 = JoltConversions::ToJoltVector(getAxis(normalizedWorldRotation2, normalAxis));

		SetConstraint(bodyInterface.CreateConstraint(&settings, bodyID1, bodyID2), bodyID1, bodyID2);
	}

	JoltSliderConstraint *JoltSliderConstraint::WithBodiesAndGlobalFrames(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2, Axis axis)
	{
		JoltSliderConstraint *constraint = new JoltSliderConstraint(body1, globalPosition1, worldRotation1, body2, globalPosition2, worldRotation2, axis);
		return constraint->Autorelease();
	}

	void JoltSliderConstraint::SetLimits(float limitMin, float limitMax)
	{
		if(!_constraint) return;
		if(limitMin <= -1000000.0f && limitMax >= 1000000.0f)
		{
			limitMin = -FLT_MAX;
			limitMax = FLT_MAX;
		}
		if(limitMin > limitMax)
		{
			float temporary = limitMin;
			limitMin = limitMax;
			limitMax = temporary;
		}
		if(limitMin > 0.0f) limitMin = 0.0f;
		if(limitMax < 0.0f) limitMax = 0.0f;

		JPH::SliderConstraint *slider = static_cast<JPH::SliderConstraint *>(_constraint);
		bool shouldActivate = !slider->HasLimits() || slider->GetLimitsMin() != limitMin || slider->GetLimitsMax() != limitMax;
		slider->SetLimits(limitMin, limitMax);
		if(shouldActivate) ActivateConstrainedBodies();
	}

	void JoltSliderConstraint::SetMotorState(int state)
	{
		if(!_constraint) return;
		JPH::SliderConstraint *slider = static_cast<JPH::SliderConstraint *>(_constraint);
		JPH::EMotorState motorState = JPH::EMotorState::Off;
		if(state == 1) motorState = JPH::EMotorState::Velocity;
		else if(state == 2) motorState = JPH::EMotorState::Position;

		bool shouldActivate = slider->GetMotorState() != motorState;
		slider->SetMotorState(motorState);
		if(shouldActivate && motorState != JPH::EMotorState::Off) ActivateConstrainedBodies();
	}

	void JoltSliderConstraint::SetTargetPosition(float position)
	{
		if(!_constraint) return;
		JPH::SliderConstraint *slider = static_cast<JPH::SliderConstraint *>(_constraint);
		bool shouldActivate = slider->GetTargetPosition() != position;
		slider->SetTargetPosition(position);
		if(shouldActivate) ActivateConstrainedBodies();
	}

	void JoltSliderConstraint::SetTargetVelocity(float velocity)
	{
		if(!_constraint) return;
		JPH::SliderConstraint *slider = static_cast<JPH::SliderConstraint *>(_constraint);
		bool shouldActivate = slider->GetTargetVelocity() != velocity || velocity * velocity > k::EpsilonFloat;
		slider->SetTargetVelocity(velocity);
		if(shouldActivate) ActivateConstrainedBodies();
	}

	void JoltSliderConstraint::SetMotorParams(float frequency, float damping, float maxForce)
	{
		if(!_constraint) return;
		if(frequency < 0.0f) frequency = 0.0f;
		if(damping < 0.0f) damping = 0.0f;
		if(maxForce < 0.0f) maxForce = 0.0f;

		JPH::SliderConstraint *slider = static_cast<JPH::SliderConstraint *>(_constraint);
		JPH::MotorSettings &motorSettings = slider->GetMotorSettings();
		motorSettings.mSpringSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
		motorSettings.mSpringSettings.mFrequency = frequency;
		motorSettings.mSpringSettings.mDamping = damping;
		motorSettings.SetForceLimit(maxForce);
	}

	void JoltSliderConstraint::SetLimitsSpringParams(float frequency, float damping)
	{
		if(!_constraint) return;
		if(frequency < 0.0f) frequency = 0.0f;
		if(damping < 0.0f) damping = 0.0f;

		JPH::SliderConstraint *slider = static_cast<JPH::SliderConstraint *>(_constraint);
		JPH::SpringSettings springSettings;
		springSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
		springSettings.mFrequency = frequency;
		springSettings.mDamping = damping;
		slider->SetLimitsSpringSettings(springSettings);
	}

	void JoltSliderConstraint::SetMaxFrictionForce(float maxFriction)
	{
		if(!_constraint) return;
		if(maxFriction < 0.0f) maxFriction = 0.0f;

		JPH::SliderConstraint *slider = static_cast<JPH::SliderConstraint *>(_constraint);
		bool shouldActivate = slider->GetMaxFrictionForce() != maxFriction && maxFriction > k::EpsilonFloat;
		slider->SetMaxFrictionForce(maxFriction);
		if(shouldActivate) ActivateConstrainedBodies();
	}

	float JoltSliderConstraint::GetCurrentPosition() const
	{
		if(!_constraint) return 0.0f;
		return static_cast<JPH::SliderConstraint *>(_constraint)->GetCurrentPosition();
	}

	Vector3 JoltHingeConstraint::GetAxisVector(Axis axis)
	{
		if(axis == Axis::Y) return Vector3(0.0f, 1.0f, 0.0f);
		if(axis == Axis::Z) return Vector3(0.0f, 0.0f, 1.0f);
		return Vector3(1.0f, 0.0f, 0.0f);
	}

	Vector3 JoltHingeConstraint::GetNormalVector(Axis axis)
	{
		if(axis == Axis::Y) return Vector3(1.0f, 0.0f, 0.0f);
		return Vector3(0.0f, 1.0f, 0.0f);
	}

	JoltHingeConstraint::JoltHingeConstraint(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2, Axis axis)
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		JPH::BodyID bodyID1 = body1 && body1->GetJoltActor() ? *body1->GetJoltActor() : JPH::BodyID();
		JPH::BodyID bodyID2 = body2 && body2->GetJoltActor() ? *body2->GetJoltActor() : JPH::BodyID();
		RN_ASSERT(!bodyID1.IsInvalid() && !bodyID2.IsInvalid(), "Invalid bodies for constraint creation");

		auto getSafeRotation = [](const Quaternion &rotation) -> Quaternion {
			if(!rotation.IsValid()) return Quaternion();

			Quaternion result(rotation);
			result.Normalize();
			return result.IsValid() ? result : Quaternion();
		};
		auto getAxis = [](const Quaternion &rotation, const Vector3 &fallbackAxis) -> Vector3 {
			Vector3 axis = rotation.GetRotatedVector(fallbackAxis);
			if(!axis.IsValid() || axis.GetSquaredLength() <= k::EpsilonFloat) return fallbackAxis;

			axis.Normalize();
			return axis.IsValid() ? axis : fallbackAxis;
		};

		Quaternion normalizedWorldRotation1 = getSafeRotation(worldRotation1);
		Quaternion normalizedWorldRotation2 = getSafeRotation(worldRotation2);
		_constraintToBody1 = JoltConversions::ToEngineRotation(bodyInterface.GetRotation(bodyID1)).GetConjugated() * normalizedWorldRotation1;
		_constraintToBody1.Normalize();
		_constraintToBody2 = JoltConversions::ToEngineRotation(bodyInterface.GetRotation(bodyID2)).GetConjugated() * normalizedWorldRotation2;
		_constraintToBody2.Normalize();
		Vector3 hingeAxis = GetAxisVector(axis);
		Vector3 normalAxis = GetNormalVector(axis);

		JPH::HingeConstraintSettings settings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		settings.mPoint1 = JoltConversions::ToJoltPosition(globalPosition1);
		settings.mHingeAxis1 = JoltConversions::ToJoltVector(getAxis(normalizedWorldRotation1, hingeAxis));
		settings.mNormalAxis1 = JoltConversions::ToJoltVector(getAxis(normalizedWorldRotation1, normalAxis));
		settings.mPoint2 = JoltConversions::ToJoltPosition(globalPosition2);
		settings.mHingeAxis2 = JoltConversions::ToJoltVector(getAxis(normalizedWorldRotation2, hingeAxis));
		settings.mNormalAxis2 = JoltConversions::ToJoltVector(getAxis(normalizedWorldRotation2, normalAxis));

		SetConstraint(bodyInterface.CreateConstraint(&settings, bodyID1, bodyID2), bodyID1, bodyID2);
	}

	JoltHingeConstraint *JoltHingeConstraint::WithBodiesAndGlobalFrames(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2, Axis axis)
	{
		JoltHingeConstraint *constraint = new JoltHingeConstraint(body1, globalPosition1, worldRotation1, body2, globalPosition2, worldRotation2, axis);
		return constraint->Autorelease();
	}

	void JoltHingeConstraint::SetLimits(float limitMin, float limitMax)
	{
		if(!_constraint) return;
		if(limitMin > limitMax)
		{
			float temporary = limitMin;
			limitMin = limitMax;
			limitMax = temporary;
		}
		if(limitMin < -k::Pi) limitMin = -k::Pi;
		if(limitMin > 0.0f) limitMin = 0.0f;
		if(limitMax < 0.0f) limitMax = 0.0f;
		if(limitMax > k::Pi) limitMax = k::Pi;

		JPH::HingeConstraint *hinge = static_cast<JPH::HingeConstraint *>(_constraint);
		bool shouldActivate = hinge->GetLimitsMin() != limitMin || hinge->GetLimitsMax() != limitMax;
		hinge->SetLimits(limitMin, limitMax);
		if(shouldActivate) ActivateConstrainedBodies();
	}

	void JoltHingeConstraint::SetMotorState(int state)
	{
		if(!_constraint) return;
		JPH::HingeConstraint *hinge = static_cast<JPH::HingeConstraint *>(_constraint);
		JPH::EMotorState motorState = JPH::EMotorState::Off;
		if(state == 1) motorState = JPH::EMotorState::Velocity;
		else if(state == 2) motorState = JPH::EMotorState::Position;
		else if(state == 3) motorState = JPH::EMotorState::PositionAndVelocity;

		bool shouldActivate = hinge->GetMotorState() != motorState;
		hinge->SetMotorState(motorState);
		if(shouldActivate && motorState != JPH::EMotorState::Off) ActivateConstrainedBodies();
	}

	void JoltHingeConstraint::SetTargetAngle(float angle)
	{
		if(!_constraint) return;
		JPH::HingeConstraint *hinge = static_cast<JPH::HingeConstraint *>(_constraint);
		bool shouldActivate = hinge->GetTargetAngle() != angle;
		hinge->SetTargetAngle(angle);
		if(shouldActivate) ActivateConstrainedBodies();
	}

	void JoltHingeConstraint::SetTargetAngularVelocity(float velocity)
	{
		if(!_constraint) return;
		JPH::HingeConstraint *hinge = static_cast<JPH::HingeConstraint *>(_constraint);
		bool shouldActivate = hinge->GetTargetAngularVelocity() != velocity || velocity * velocity > k::EpsilonFloat;
		hinge->SetTargetAngularVelocity(velocity);
		if(shouldActivate) ActivateConstrainedBodies();
	}

	void JoltHingeConstraint::SetTargetOrientationCS(const Quaternion &q_cs)
	{
		SetTargetOrientationBS(_constraintToBody1 * q_cs * _constraintToBody2.GetConjugated());
	}

	void JoltHingeConstraint::SetTargetOrientationBS(const Quaternion &q_bs)
	{
		if(!_constraint) return;
		JPH::HingeConstraint *hinge = static_cast<JPH::HingeConstraint *>(_constraint);
		float previousTarget = hinge->GetTargetAngle();
		hinge->SetTargetOrientationBS(ToJoltQuat(q_bs));
		if(hinge->GetTargetAngle() != previousTarget) ActivateConstrainedBodies();
	}

	void JoltHingeConstraint::SetMotorParams(float frequency, float damping, float maxTorque)
	{
		if(!_constraint) return;
		if(frequency < 0.0f) frequency = 0.0f;
		if(damping < 0.0f) damping = 0.0f;
		if(maxTorque < 0.0f) maxTorque = 0.0f;

		JPH::HingeConstraint *hinge = static_cast<JPH::HingeConstraint *>(_constraint);
		JPH::MotorSettings &motorSettings = hinge->GetMotorSettings();
		motorSettings.mSpringSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
		motorSettings.mSpringSettings.mFrequency = frequency;
		motorSettings.mSpringSettings.mDamping = damping;
		motorSettings.SetTorqueLimit(maxTorque);
	}

	void JoltHingeConstraint::SetLimitsSpringParams(float frequency, float damping)
	{
		if(!_constraint) return;
		if(frequency < 0.0f) frequency = 0.0f;
		if(damping < 0.0f) damping = 0.0f;

		JPH::HingeConstraint *hinge = static_cast<JPH::HingeConstraint *>(_constraint);
		JPH::SpringSettings springSettings;
		springSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
		springSettings.mFrequency = frequency;
		springSettings.mDamping = damping;
		hinge->SetLimitsSpringSettings(springSettings);
	}

	void JoltHingeConstraint::SetMaxFrictionTorque(float maxFriction)
	{
		if(!_constraint) return;
		if(maxFriction < 0.0f) maxFriction = 0.0f;

		JPH::HingeConstraint *hinge = static_cast<JPH::HingeConstraint *>(_constraint);
		bool shouldActivate = hinge->GetMaxFrictionTorque() != maxFriction && maxFriction > k::EpsilonFloat;
		hinge->SetMaxFrictionTorque(maxFriction);
		if(shouldActivate) ActivateConstrainedBodies();
	}

	float JoltHingeConstraint::GetCurrentAngle() const
	{
		if(!_constraint) return 0.0f;
		return static_cast<JPH::HingeConstraint *>(_constraint)->GetCurrentAngle();
	}

	float JoltHingeConstraint::GetTotalMotorImpulse() const
	{
		if(!_constraint) return 0.0f;
		return static_cast<JPH::HingeConstraint *>(_constraint)->GetTotalLambdaMotor();
	}

	JoltSixDOFConstraint::JoltSixDOFConstraint(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2) :
		JoltSixDOFConstraint(body1 ? body1->GetJoltBodyID() : 0xffffffffU, globalPosition1, worldRotation1, body2 ? body2->GetJoltBodyID() : 0xffffffffU, globalPosition2, worldRotation2)
	{}

	JoltSixDOFConstraint::JoltSixDOFConstraint(uint32 firstBodyID, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, uint32 secondBodyID, const JoltPosition &globalPosition2, const Quaternion &worldRotation2, bool enableBody2MassScaling) :
		_enableBody2MassScaling(enableBody2MassScaling)
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		JPH::BodyID bodyID1(firstBodyID);
		JPH::BodyID bodyID2(secondBodyID);
		RN_ASSERT(!bodyID1.IsInvalid() && !bodyID2.IsInvalid(), "Invalid bodies for constraint creation");

		JoltScaledMotorConstraintSettings scaledSettings;
		JPH::SixDOFConstraintSettings regularSettings;
		JPH::SixDOFConstraintSettings &settings = enableBody2MassScaling ? scaledSettings.motorSettings : regularSettings;
		settings.mSpace = JPH::EConstraintSpace::WorldSpace;
		auto getSafeRotation = [](const Quaternion &rotation) -> Quaternion {
			if(!rotation.IsValid()) return Quaternion();

			Quaternion result(rotation);
			result.Normalize();
			return result.IsValid() ? result : Quaternion();
		};
		auto getAxis = [](const Quaternion &rotation, const Vector3 &fallbackAxis) -> Vector3 {
			Vector3 axis = rotation.GetRotatedVector(fallbackAxis);
			if(!axis.IsValid() || axis.GetSquaredLength() <= k::EpsilonFloat) return fallbackAxis;

			axis.Normalize();
			return axis.IsValid() ? axis : fallbackAxis;
		};
		Quaternion normalizedWorldRotation1 = getSafeRotation(worldRotation1);
		Quaternion normalizedWorldRotation2 = getSafeRotation(worldRotation2);

		settings.mPosition1 = JoltConversions::ToJoltPosition(globalPosition1);
		settings.mAxisX1 = JoltConversions::ToJoltVector(getAxis(normalizedWorldRotation1, Vector3(1.0f, 0.0f, 0.0f)));
		settings.mAxisY1 = JoltConversions::ToJoltVector(getAxis(normalizedWorldRotation1, Vector3(0.0f, 1.0f, 0.0f)));
		settings.mPosition2 = JoltConversions::ToJoltPosition(globalPosition2);
		settings.mAxisX2 = JoltConversions::ToJoltVector(getAxis(normalizedWorldRotation2, Vector3(1.0f, 0.0f, 0.0f)));
		settings.mAxisY2 = JoltConversions::ToJoltVector(getAxis(normalizedWorldRotation2, Vector3(0.0f, 1.0f, 0.0f)));

		// Allow all DOFs; motors will drive to targets each tick
		settings.MakeFreeAxis(JPH::SixDOFConstraintSettings::TranslationX);
		settings.MakeFreeAxis(JPH::SixDOFConstraintSettings::TranslationY);
		settings.MakeFreeAxis(JPH::SixDOFConstraintSettings::TranslationZ);
		settings.MakeFreeAxis(JPH::SixDOFConstraintSettings::RotationX);
		settings.MakeFreeAxis(JPH::SixDOFConstraintSettings::RotationY);
		settings.MakeFreeAxis(JPH::SixDOFConstraintSettings::RotationZ);

		SetConstraint(bodyInterface.CreateConstraint(enableBody2MassScaling ? static_cast<JPH::TwoBodyConstraintSettings *>(&scaledSettings) : &settings, bodyID1, bodyID2), bodyID1, bodyID2);

		// Default motor params
		SetLinearMotorParams(30.0f, 6.0f, 5000.0f);
		SetAngularMotorParams(40.0f, 8.0f, 2000.0f);

		// Enable motors (position)
		SetMotorState(Axis::TranslationX, 2);
		SetMotorState(Axis::TranslationY, 2);
		SetMotorState(Axis::TranslationZ, 2);
		SetMotorState(Axis::RotationX, 2);
		SetMotorState(Axis::RotationY, 2);
		SetMotorState(Axis::RotationZ, 2);
	}

	JPH::SixDOFConstraint *JoltSixDOFConstraint::GetMotorConstraint() const
	{
		return _enableBody2MassScaling ? static_cast<JoltScaledMotorConstraint *>(_constraint)->GetMotor() : static_cast<JPH::SixDOFConstraint *>(_constraint);
	}

	void JoltSixDOFConstraint::SetMotorBody2MassScale(float scale)
	{
		if(!_constraint || !_enableBody2MassScaling || !std::isfinite(scale)) return;
		static_cast<JoltScaledMotorConstraint *>(_constraint)->SetBody2MassScale(scale);
	}

	JoltSixDOFConstraint *JoltSixDOFConstraint::WithBodiesAndGlobalFrames(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2)
	{
		JoltSixDOFConstraint *c = new JoltSixDOFConstraint(body1, globalPosition1, worldRotation1, body2, globalPosition2, worldRotation2);
		return c->Autorelease();
	}

	void JoltSixDOFConstraint::SetMotorState(Axis axis, int state)
	{
		if(!_constraint) return;
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		JPH::EMotorState s = JPH::EMotorState::Off;
		if(state == 1) s = JPH::EMotorState::Velocity;
		else if(state == 2) s = JPH::EMotorState::Position;
		else if(state == 3) s = JPH::EMotorState::PositionAndVelocity;
		bool shouldActivate = six->GetMotorState(static_cast<JPH::SixDOFConstraint::EAxis>(static_cast<int>(axis))) != s;
		six->SetMotorState(static_cast<JPH::SixDOFConstraintSettings::EAxis>(static_cast<int>(axis)), s);
		if(shouldActivate && s != JPH::EMotorState::Off) ActivateConstrainedBodies();
	}

	void JoltSixDOFConstraint::SetTargetPositionCS(const Vector3 &p_cs)
	{
		if(!_constraint) return;
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		JPH::Vec3 currentTarget = six->GetTargetPositionCS();
		bool shouldActivate = Vector3(currentTarget.GetX(), currentTarget.GetY(), currentTarget.GetZ()) != p_cs;
		six->SetTargetPositionCS(ToJoltVec3(p_cs));
		if(shouldActivate) ActivateConstrainedBodies();
	}
	void JoltSixDOFConstraint::SetTargetVelocityCS(const Vector3 &v_cs)
	{
		if(!_constraint) return;
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		JPH::Vec3 currentTarget = six->GetTargetVelocityCS();
		bool shouldActivate = Vector3(currentTarget.GetX(), currentTarget.GetY(), currentTarget.GetZ()) != v_cs || v_cs.GetSquaredLength() > k::EpsilonFloat;
		six->SetTargetVelocityCS(ToJoltVec3(v_cs));
		if(shouldActivate) ActivateConstrainedBodies();
	}
	void JoltSixDOFConstraint::SetTargetAngularVelocityCS(const Vector3 &w_cs)
	{
		if(!_constraint) return;
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		JPH::Vec3 currentTarget = six->GetTargetAngularVelocityCS();
		bool shouldActivate = Vector3(currentTarget.GetX(), currentTarget.GetY(), currentTarget.GetZ()) != w_cs || w_cs.GetSquaredLength() > k::EpsilonFloat;
		six->SetTargetAngularVelocityCS(ToJoltVec3(w_cs));
		if(shouldActivate) ActivateConstrainedBodies();
	}
	void JoltSixDOFConstraint::SetTargetOrientationCS(const Quaternion &q_cs)
	{
		if(!_constraint) return;
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		JPH::Quat currentTarget = six->GetTargetOrientationCS();
		bool shouldActivate = Quaternion(currentTarget.GetX(), currentTarget.GetY(), currentTarget.GetZ(), currentTarget.GetW()) != q_cs;
		six->SetTargetOrientationCS(ToJoltQuat(q_cs));
		if(shouldActivate) ActivateConstrainedBodies();
	}
	void JoltSixDOFConstraint::SetTargetOrientationBS(const Quaternion &q_bs)
	{
		if(!_constraint) return;
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		JPH::Quat previousTarget = six->GetTargetOrientationCS();
		six->SetTargetOrientationBS(ToJoltQuat(q_bs));
		JPH::Quat currentTarget = six->GetTargetOrientationCS();
		bool shouldActivate = Quaternion(previousTarget.GetX(), previousTarget.GetY(), previousTarget.GetZ(), previousTarget.GetW()) != Quaternion(currentTarget.GetX(), currentTarget.GetY(), currentTarget.GetZ(), currentTarget.GetW());
		if(shouldActivate) ActivateConstrainedBodies();
	}

	void JoltSixDOFConstraint::SetLinearMotorParams(float frequency, float damping, float maxForce)
	{
		if(!_constraint) return;
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		for(int a = 0; a < 3; ++a)
		{
			JPH::MotorSettings &m = six->GetMotorSettings(static_cast<JPH::SixDOFConstraint::EAxis>(a));
			m.mSpringSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
			m.mSpringSettings.mFrequency = frequency;
			m.mSpringSettings.mDamping = damping;
			m.SetForceLimit(maxForce);
		}
	}
	void JoltSixDOFConstraint::SetAngularMotorParams(float frequency, float damping, float maxTorque)
	{
		if(!_constraint) return;
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		for(int a = 3; a < 6; ++a)
		{
			JPH::MotorSettings &m = six->GetMotorSettings(static_cast<JPH::SixDOFConstraint::EAxis>(a));
			m.mSpringSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
			m.mSpringSettings.mFrequency = frequency;
			m.mSpringSettings.mDamping = damping;
			m.SetTorqueLimit(maxTorque);
		}
	}

	void JoltSixDOFConstraint::SetLinearMotorStiffnessParams(float stiffness, float damping, float maxForce)
	{
		if(!_constraint) return;
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		for(int a = 0; a < 3; ++a)
		{
			JPH::MotorSettings &m = six->GetMotorSettings(static_cast<JPH::SixDOFConstraint::EAxis>(a));
			m.mSpringSettings.mMode = JPH::ESpringMode::StiffnessAndDamping;
			m.mSpringSettings.mStiffness = stiffness;
			m.mSpringSettings.mDamping = damping;
			m.SetForceLimit(maxForce);
		}
	}

	void JoltSixDOFConstraint::SetAngularMotorStiffnessParams(float stiffness, float damping, float maxTorque)
	{
		if(!_constraint) return;
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		for(int a = 3; a < 6; ++a)
		{
			JPH::MotorSettings &m = six->GetMotorSettings(static_cast<JPH::SixDOFConstraint::EAxis>(a));
			m.mSpringSettings.mMode = JPH::ESpringMode::StiffnessAndDamping;
			m.mSpringSettings.mStiffness = stiffness;
			m.mSpringSettings.mDamping = damping;
			m.SetTorqueLimit(maxTorque);
		}
	}

	Vector3 JoltSixDOFConstraint::GetTotalMotorTranslationImpulse() const
	{
		if(!_constraint) return Vector3();

		JPH::Vec3 impulse = _enableBody2MassScaling ? static_cast<JoltScaledMotorConstraint *>(_constraint)->GetMotorTranslationImpulse() : GetMotorConstraint()->GetTotalLambdaMotorTranslation();
		return Vector3(impulse.GetX(), impulse.GetY(), impulse.GetZ());
	}

	Vector3 JoltSixDOFConstraint::GetTotalMotorRotationImpulse() const
	{
		if(!_constraint) return Vector3();

		JPH::Vec3 impulse = _enableBody2MassScaling ? static_cast<JoltScaledMotorConstraint *>(_constraint)->GetMotorRotationImpulse() : GetMotorConstraint()->GetTotalLambdaMotorRotation();
		return Vector3(impulse.GetX(), impulse.GetY(), impulse.GetZ());
	}

	void JoltSixDOFConstraint::SetTranslationLimits(const Vector3 &limitMin, const Vector3 &limitMax)
	{
		if(!_constraint) return;
		RebuildWithLimits(&limitMin, &limitMax, nullptr, nullptr);
	}

	void JoltSixDOFConstraint::SetRotationLimits(const Vector3 &limitMin, const Vector3 &limitMax)
	{
		if(!_constraint) return;
		RebuildWithLimits(nullptr, nullptr, &limitMin, &limitMax);
	}

	void JoltSixDOFConstraint::RebuildWithLimits(const Vector3 *translationLimitMin, const Vector3 *translationLimitMax, const Vector3 *rotationLimitMin, const Vector3 *rotationLimitMax)
	{
		if(!_constraint || _enableBody2MassScaling) return;

		auto configureAxis = [](JPH::SixDOFConstraintSettings *settings, JPH::SixDOFConstraintSettings::EAxis axis, float limitMin, float limitMax) {
			if(limitMin <= -1000000.0f && limitMax >= 1000000.0f)
			{
				settings->MakeFreeAxis(axis);
				return;
			}

			if(limitMin >= limitMax || limitMin > 0.0f || limitMax < 0.0f)
			{
				settings->MakeFixedAxis(axis);
				return;
			}

			settings->SetLimitedAxis(axis, limitMin, limitMax);
		};

		JPH::SixDOFConstraint *six = GetMotorConstraint();
		JPH::BodyID bodyID1(_bodyPairCollisionBody1);
		JPH::BodyID bodyID2(_bodyPairCollisionBody2);
		if(bodyID1.IsInvalid() || bodyID2.IsInvalid()) return;

		JPH::EMotorState motorState[6];
		for(int i = 0; i < 6; i++)
		{
			motorState[i] = six->GetMotorState(static_cast<JPH::SixDOFConstraint::EAxis>(i));
		}

		JPH::Vec3 targetVelocity = six->GetTargetVelocityCS();
		JPH::Vec3 targetAngularVelocity = six->GetTargetAngularVelocityCS();
		JPH::Vec3 targetPosition = six->GetTargetPositionCS();
		JPH::Quat targetOrientation = six->GetTargetOrientationCS();
		JPH::SpringSettings limitsSpringSettings[3] = {
			six->GetLimitsSpringSettings(JPH::SixDOFConstraint::EAxis::TranslationX),
			six->GetLimitsSpringSettings(JPH::SixDOFConstraint::EAxis::TranslationY),
			six->GetLimitsSpringSettings(JPH::SixDOFConstraint::EAxis::TranslationZ)
		};

		JPH::Ref<JPH::ConstraintSettings> constraintSettings = six->GetConstraintSettings();
		JPH::SixDOFConstraintSettings *settings = static_cast<JPH::SixDOFConstraintSettings *>(constraintSettings.GetPtr());
		for(int i = 0; i < 3; i++)
		{
			settings->mLimitsSpringSettings[i] = limitsSpringSettings[i];
		}

		if(translationLimitMin && translationLimitMax)
		{
			configureAxis(settings, JPH::SixDOFConstraintSettings::TranslationX, translationLimitMin->x, translationLimitMax->x);
			configureAxis(settings, JPH::SixDOFConstraintSettings::TranslationY, translationLimitMin->y, translationLimitMax->y);
			configureAxis(settings, JPH::SixDOFConstraintSettings::TranslationZ, translationLimitMin->z, translationLimitMax->z);
		}

		if(rotationLimitMin && rotationLimitMax)
		{
			configureAxis(settings, JPH::SixDOFConstraintSettings::RotationX, rotationLimitMin->x, rotationLimitMax->x);
			configureAxis(settings, JPH::SixDOFConstraintSettings::RotationY, rotationLimitMin->y, rotationLimitMax->y);
			configureAxis(settings, JPH::SixDOFConstraintSettings::RotationZ, rotationLimitMin->z, rotationLimitMax->z);
		}

		JPH::BodyInterface &bodyInterface = JoltWorld::GetSharedInstance()->GetJoltInstance()->GetBodyInterface();
		SetConstraint(bodyInterface.CreateConstraint(settings, bodyID1, bodyID2), bodyID1, bodyID2);

		six = GetMotorConstraint();
		for(int i = 0; i < 6; i++)
		{
			six->SetMotorState(static_cast<JPH::SixDOFConstraint::EAxis>(i), motorState[i]);
		}

		six->SetTargetVelocityCS(targetVelocity);
		six->SetTargetAngularVelocityCS(targetAngularVelocity);
		six->SetTargetPositionCS(targetPosition);
		six->SetTargetOrientationCS(targetOrientation);
	}

	void JoltSixDOFConstraint::SetTranslationSpringParams(float frequency, float damping)
	{
		if(!_constraint) return;
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		JPH::SpringSettings s;
		s.mMode = JPH::ESpringMode::FrequencyAndDamping;
		s.mFrequency = frequency;
		s.mDamping = damping;
		for(int a = 0; a < 3; ++a)
		{
			six->SetLimitsSpringSettings(static_cast<JPH::SixDOFConstraint::EAxis>(a), s);
		}
	}

	void JoltSixDOFConstraint::SetTranslationSpringParamsAxis(Axis axis, float frequency, float damping)
	{
		if(!_constraint) return;
		int a = static_cast<int>(axis);
		if(a < 0 || a > 2) return; // only translation axes support spring limits
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		JPH::SpringSettings s;
		s.mMode = JPH::ESpringMode::FrequencyAndDamping;
		s.mFrequency = frequency;
		s.mDamping = damping;
		six->SetLimitsSpringSettings(static_cast<JPH::SixDOFConstraint::EAxis>(a), s);
	}

	void JoltSixDOFConstraint::SetMaxFriction(Axis axis, float maxFriction)
	{
		if(!_constraint || _enableBody2MassScaling) return;
		JPH::SixDOFConstraint *six = GetMotorConstraint();
		six->SetMaxFriction(static_cast<JPH::SixDOFConstraint::EAxis>(static_cast<int>(axis)), maxFriction);
	}
}
