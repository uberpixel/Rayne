//
//  RNJoltConstraint.h
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTCONSTRAINT_H_
#define __RAYNE_JOLTCONSTRAINT_H_

#include "RNJolt.h"
#include "RNJoltDynamicBody.h"

namespace JPH
{
	class BodyID;
	class Constraint;
}

namespace RN
{
	class JoltConstraint : public Object, private JoltConstraintOwner
	{
	public:
		JTAPI JPH::Constraint *GetJoltConstraint() const { return _constraint; }
		JTAPI void SetEnabled(bool enabled);
		JTAPI void SetCollisionsEnabled(bool enabled);
		JTAPI bool GetCollisionsEnabled() const { return _collisionsEnabled; }
		JTAPI void SetConnectedBodyCollisionFilteringEnabled(bool enabled);
		JTAPI bool GetConnectedBodyCollisionFilteringEnabled() const { return _connectedBodyCollisionFilteringEnabled; }
		JTAPI void SetSolverIterationCount(uint32 positionIterations, uint32 velocityIterations); //0 resets to the default

	protected:
		JoltConstraint();
		~JoltConstraint() override;

		void SetConstraint(JPH::Constraint *constraint, const JPH::BodyID &bodyID1, const JPH::BodyID &bodyID2);
		void ActivateConstrainedBodies();
		void ResetStoredBodyPairCollisionState();
		void UpdateBodyPairCollisionState();
		void SetStoredBodyPairCollisionEnabled(bool enabled);
		bool HasStoredBodyPair() const;
		void InvalidateJoltConstraint(JPH::Constraint *constraint) override;

		JPH::Constraint *_constraint;
		uint32 _bodyPairCollisionBody1;
		uint32 _bodyPairCollisionBody2;
		bool _bodyPairCollisionDisabled;
		bool _bodyPairConnectedBodyCollisionFilteringEnabled;
		bool _collisionsEnabled;
		bool _connectedBodyCollisionFilteringEnabled;

		RNDeclareMetaAPI(JoltConstraint, JTAPI)
	};

	class JoltPointConstraint : public JoltConstraint
	{
	public:
		JTAPI JoltPointConstraint(JoltDynamicBody *body1, const JoltPosition &globalPoint1, JoltDynamicBody *body2, const JoltPosition &globalPoint2);
		JTAPI static JoltPointConstraint *WithBodiesAndGlobalPoints(JoltDynamicBody *body1, const JoltPosition &globalPoint1, JoltDynamicBody *body2, const JoltPosition &globalPoint2);

		RNDeclareMetaAPI(JoltPointConstraint, JTAPI)
	};

	class JoltFixedConstraint : public JoltConstraint
	{
	public:
		JTAPI JoltFixedConstraint(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2);
		JTAPI static JoltFixedConstraint *WithBodiesAndGlobalFrames(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2);

		RNDeclareMetaAPI(JoltFixedConstraint, JTAPI)
	};

	class JoltDistanceConstraint : public JoltConstraint
	{
	public:
		JTAPI JoltDistanceConstraint(JoltDynamicBody *body1, const JoltPosition &globalPoint1, JoltDynamicBody *body2, const JoltPosition &globalPoint2, float minDistance, float maxDistance);
		JTAPI static JoltDistanceConstraint *WithBodiesAndGlobalPoints(JoltDynamicBody *body1, const JoltPosition &globalPoint1, JoltDynamicBody *body2, const JoltPosition &globalPoint2, float minDistance, float maxDistance);

		RNDeclareMetaAPI(JoltDistanceConstraint, JTAPI)
	};

	class JoltSliderConstraint : public JoltConstraint
	{
	public:
		enum class Axis
		{
			X,
			Y,
			Z
		};

		JTAPI JoltSliderConstraint(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2, Axis axis = Axis::X);
		JTAPI static JoltSliderConstraint *WithBodiesAndGlobalFrames(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2, Axis axis = Axis::X);

		JTAPI void SetLimits(float limitMin, float limitMax);
		JTAPI void SetMotorState(int state); // 0=Off,1=Velocity,2=Position
		JTAPI void SetTargetPosition(float position);
		JTAPI void SetTargetVelocity(float velocity);
		JTAPI void SetMotorParams(float frequency, float damping, float maxForce);
		JTAPI void SetLimitsSpringParams(float frequency, float damping);
		JTAPI void SetMaxFrictionForce(float maxFriction);
		JTAPI float GetCurrentPosition() const;

		RNDeclareMetaAPI(JoltSliderConstraint, JTAPI)

	private:
		static Vector3 GetAxisVector(Axis axis);
		static Vector3 GetNormalVector(Axis axis);
	};

	class JoltHingeConstraint : public JoltConstraint
	{
	public:
		enum class Axis
		{
			X,
			Y,
			Z
		};

		JTAPI JoltHingeConstraint(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2, Axis axis = Axis::X);
		JTAPI static JoltHingeConstraint *WithBodiesAndGlobalFrames(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2, Axis axis = Axis::X);

		JTAPI void SetLimits(float limitMin, float limitMax);
		JTAPI void SetMotorState(int state); // 0=Off,1=Velocity,2=Position,3=PositionAndVelocity
		JTAPI void SetTargetAngle(float angle);
		JTAPI void SetTargetAngularVelocity(float velocity);
		JTAPI void SetTargetOrientationCS(const Quaternion &q_cs);
		JTAPI void SetTargetOrientationBS(const Quaternion &q_bs);
		JTAPI void SetMotorParams(float frequency, float damping, float maxTorque);
		JTAPI void SetLimitsSpringParams(float frequency, float damping);
		JTAPI void SetMaxFrictionTorque(float maxFriction);
		JTAPI float GetCurrentAngle() const;
		JTAPI float GetTotalMotorImpulse() const;

		RNDeclareMetaAPI(JoltHingeConstraint, JTAPI)

	private:
		Quaternion _constraintToBody1;
		Quaternion _constraintToBody2;

		static Vector3 GetAxisVector(Axis axis);
		static Vector3 GetNormalVector(Axis axis);
	};

	class JoltSixDOFConstraint : public JoltConstraint
	{
	public:
		enum class Axis
		{
			TranslationX,
			TranslationY,
			TranslationZ,
			RotationX,
			RotationY,
			RotationZ
		};

		JTAPI JoltSixDOFConstraint(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2);
		JTAPI JoltSixDOFConstraint(uint32 bodyID1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, uint32 bodyID2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2);
		JTAPI static JoltSixDOFConstraint *WithBodiesAndGlobalFrames(JoltDynamicBody *body1, const JoltPosition &globalPosition1, const Quaternion &worldRotation1, JoltDynamicBody *body2, const JoltPosition &globalPosition2, const Quaternion &worldRotation2);

		JTAPI void SetMotorState(Axis axis, int state); // 0=Off,1=Velocity,2=Position,3=PositionAndVelocity
		JTAPI void SetTargetPositionCS(const Vector3 &p_cs);
		JTAPI void SetTargetVelocityCS(const Vector3 &v_cs);
		JTAPI void SetTargetAngularVelocityCS(const Vector3 &w_cs);
		JTAPI void SetTargetOrientationCS(const Quaternion &q_cs);
		JTAPI void SetTargetOrientationBS(const Quaternion &q_bs);

		JTAPI void SetLinearMotorParams(float frequency, float damping, float maxForce);
		JTAPI void SetLinearMotorStiffnessParams(float stiffness, float damping, float maxForce);
		JTAPI void SetAngularMotorParams(float frequency, float damping, float maxTorque);
		JTAPI void SetAngularMotorStiffnessParams(float stiffness, float damping, float maxTorque);

		JTAPI Vector3 GetTotalMotorTranslationImpulse() const;
		JTAPI Vector3 GetTotalMotorRotationImpulse() const;

		// Limits and springs/friction wrappers
		JTAPI void SetTranslationLimits(const Vector3 &limitMin, const Vector3 &limitMax);
		JTAPI void SetRotationLimits(const Vector3 &limitMin, const Vector3 &limitMax);
		JTAPI void SetTranslationSpringParams(float frequency, float damping); // apply to X/Y/Z
		JTAPI void SetTranslationSpringParamsAxis(Axis axis, float frequency, float damping); // axis must be TranslationX/Y/Z
		JTAPI void SetMaxFriction(Axis axis, float maxFriction);

	private:
		void RebuildWithLimits(const Vector3 *translationLimitMin, const Vector3 *translationLimitMax, const Vector3 *rotationLimitMin, const Vector3 *rotationLimitMax);

		RNDeclareMetaAPI(JoltSixDOFConstraint, JTAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_JOLTCONSTRAINT_H_) */
