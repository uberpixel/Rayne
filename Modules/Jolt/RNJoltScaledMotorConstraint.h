#ifndef __RAYNE_JOLTSCALEDMOTORCONSTRAINT_H_
#define __RAYNE_JOLTSCALEDMOTORCONSTRAINT_H_

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/StateRecorder.h>
#include <Jolt/Physics/Constraints/ConstraintPart/SpringPart.h>
#include <Jolt/Physics/Body/Body.h>

namespace RN
{
	// Motor-only six DOF constraint. Scaling is local to these solver rows: it
	// never changes the body's mass for contacts or other constraints.
	class JoltScaledMotorConstraint : public JPH::TwoBodyConstraint
	{
	public:
		using EAxis = JPH::SixDOFConstraint::EAxis;
		JoltScaledMotorConstraint(JPH::Body &body1, JPH::Body &body2, const JPH::SixDOFConstraintSettings &settings) :
			JPH::TwoBodyConstraint(body1, body2, settings), _motor(new JPH::SixDOFConstraint(body1, body2, settings)) {}
		JPH::SixDOFConstraint *GetMotor() const { return _motor; }
		JPH::EConstraintSubType GetSubType() const override { return JPH::EConstraintSubType::User2; }
		void NotifyShapeChanged(const JPH::BodyID &body, JPH::Vec3Arg delta) override { _motor->NotifyShapeChanged(body, delta); }
		JPH::Mat44 GetConstraintToBody1Matrix() const override { return _motor->GetConstraintToBody1Matrix(); }
		JPH::Mat44 GetConstraintToBody2Matrix() const override { return _motor->GetConstraintToBody2Matrix(); }
		// Out-of-line key function: emit this Jolt subclass's vtable only in the
		// Jolt-only translation unit, where RTTI is disabled like the Jolt library.
		JPH::Ref<JPH::ConstraintSettings> GetConstraintSettings() const override;
		void SaveState(JPH::StateRecorder &stream) const override { JPH::TwoBodyConstraint::SaveState(stream); stream.Write(_body2MassScale); _motor->SaveState(stream); }
		void RestoreState(JPH::StateRecorder &stream) override { JPH::TwoBodyConstraint::RestoreState(stream); stream.Read(_body2MassScale); _motor->RestoreState(stream); ResetWarmStart(); }
#ifdef JPH_DEBUG_RENDERER
		void DrawConstraint(JPH::DebugRenderer *renderer) const override { _motor->DrawConstraint(renderer); }
#endif
		void SetBody2MassScale(float scale) { _body2MassScale = JPH::Clamp(scale, 0.0f, 1.0f); }

		void SetupVelocityConstraint(float delta) override
		{
			if(_body2MassScale == 1.0f)
			{
				if(_scaled) _motor->ResetWarmStart();
				_scaled = false;
				_motor->SetupVelocityConstraint(delta);
				return;
			}
			_scaled = true;
			auto frame1 = mBody1->GetCenterOfMassTransform() * GetConstraintToBody1Matrix();
			auto frame2 = mBody2->GetCenterOfMassTransform() * GetConstraintToBody2Matrix();
			JPH::Vec3 offset = JPH::Vec3(frame2.GetTranslation() - frame1.GetTranslation());
			JPH::Vec3 r1 = JPH::Vec3(frame2.GetTranslation() - mBody1->GetCenterOfMassPosition());
			JPH::Vec3 r2 = JPH::Vec3(frame2.GetTranslation() - mBody2->GetCenterOfMassPosition());
			JPH::Quat rotation = frame1.GetQuaternion().Conjugated() * frame2.GetQuaternion();
			JPH::Quat difference = (rotation.Conjugated() * _motor->GetTargetOrientationCS()).EnsureWPositive();
			JPH::Vec3 rotationError = -2.0f * difference.GetXYZ();
			_inverseMass1 = mBody1->IsDynamic() ? mBody1->GetMotionPropertiesUnchecked()->GetInverseMass() : 0.0f;
			_inverseMass2 = mBody2->IsDynamic() ? mBody2->GetMotionPropertiesUnchecked()->GetInverseMass() * _body2MassScale : 0.0f;
			JPH::Mat44 inertia1 = mBody1->IsDynamic() ? mBody1->GetInverseInertia() : JPH::Mat44::sZero();
			JPH::Mat44 inertia2 = mBody2->IsDynamic() ? mBody2->GetInverseInertia() * _body2MassScale : JPH::Mat44::sZero();
			for(int i = 0; i < 6; ++i)
			{
				Row &row = _rows[i];
				auto axis = static_cast<EAxis>(i);
				bool linear = i < 3;
				row.axis = linear ? frame1.GetColumn3(i) : frame2.GetColumn3(i - 3);
				row.angular1 = linear ? r1.Cross(row.axis) : row.axis;
				row.angular2 = linear ? r2.Cross(row.axis) : row.axis;
				row.inertia1 = inertia1.Multiply3x3(row.angular1);
				row.inertia2 = inertia2.Multiply3x3(row.angular2);
				float inverseMass = row.angular1.Dot(row.inertia1) + row.angular2.Dot(row.inertia2) + (linear ? _inverseMass1 + _inverseMass2 : 0.0f);
				row.lambda = row.effectiveMass = 0.0f;
				auto state = _motor->GetMotorState(axis);
				if(inverseMass <= 0.0f || state == JPH::EMotorState::Off) continue;
				auto &motor = _motor->GetMotorSettings(axis);
				float targetVelocity = linear ? _motor->GetTargetVelocityCS()[i] : _motor->GetTargetAngularVelocityCS()[i - 3];
				float bias = state == JPH::EMotorState::Position ? 0.0f : -targetVelocity;
				float error = linear ? row.axis.Dot(offset) - _motor->GetTargetPositionCS()[i] : rotationError[i - 3];
				if(state != JPH::EMotorState::Velocity && motor.mSpringSettings.HasStiffnessOrDamping())
					row.spring.CalculateSpringPropertiesWithSettings(delta, inverseMass, bias, error, motor.mSpringSettings, row.effectiveMass);
				else
				{
					row.spring.CalculateSpringPropertiesWithBias(bias);
					row.effectiveMass = 1.0f / inverseMass;
				}
				row.minLambda = delta * (linear ? motor.mMinForceLimit : motor.mMinTorqueLimit);
				row.maxLambda = delta * (linear ? motor.mMaxForceLimit : motor.mMaxTorqueLimit);
			}
		}

		void ResetWarmStart() override
		{
			_motor->ResetWarmStart();
			for(auto &row : _rows) row.lambda = 0.0f;
		}
		void WarmStartVelocityConstraint(float ratio) override
		{
			if(!_scaled) _motor->WarmStartVelocityConstraint(ratio);
		}
		bool SolveVelocityConstraint(float delta) override
		{
			if(!_scaled) return _motor->SolveVelocityConstraint(delta);
			bool changed = false;
			for(int i = 0; i < 6; ++i)
			{
				Row &row = _rows[i];
				if(row.effectiveMass == 0.0f) continue;
				float speed = row.angular1.Dot(mBody1->GetAngularVelocity()) - row.angular2.Dot(mBody2->GetAngularVelocity());
				if(i < 3) speed += row.axis.Dot(mBody1->GetLinearVelocity() - mBody2->GetLinearVelocity());
				float next = JPH::Clamp(row.lambda + row.effectiveMass * (speed - row.spring.GetBias(row.lambda)), row.minLambda, row.maxLambda);
				float impulse = next - row.lambda;
				row.lambda = next;
				if(impulse == 0.0f) continue;
				if(mBody1->IsDynamic())
				{
					auto motion = mBody1->GetMotionPropertiesUnchecked();
					if(i < 3) motion->SubLinearVelocityStep(row.axis * (impulse * _inverseMass1));
					motion->SubAngularVelocityStep(row.inertia1 * impulse);
				}
				if(mBody2->IsDynamic())
				{
					auto motion = mBody2->GetMotionPropertiesUnchecked();
					if(i < 3) motion->AddLinearVelocityStep(row.axis * (impulse * _inverseMass2));
					motion->AddAngularVelocityStep(row.inertia2 * impulse);
				}
				changed = true;
			}
			return changed;
		}
		bool SolvePositionConstraint(float delta, float baumgarte) override
		{
			return !_scaled && _motor->SolvePositionConstraint(delta, baumgarte);
		}
		JPH::Vec3 GetMotorTranslationImpulse() const { return _scaled ? JPH::Vec3(_rows[0].lambda, _rows[1].lambda, _rows[2].lambda) : _motor->GetTotalLambdaMotorTranslation(); }
		JPH::Vec3 GetMotorRotationImpulse() const { return _scaled ? JPH::Vec3(_rows[3].lambda, _rows[4].lambda, _rows[5].lambda) : _motor->GetTotalLambdaMotorRotation(); }

	private:
		struct Row
		{
			JPH::Vec3 axis, angular1, angular2, inertia1, inertia2;
			JPH::SpringPart spring;
			float effectiveMass = 0.0f, lambda = 0.0f, minLambda = 0.0f, maxLambda = 0.0f;
		};
		JPH::Ref<JPH::SixDOFConstraint> _motor;
		Row _rows[6];
		float _body2MassScale = 1.0f, _inverseMass1 = 0.0f, _inverseMass2 = 0.0f;
		bool _scaled = false;
	};

	class JoltScaledMotorConstraintSettings : public JPH::TwoBodyConstraintSettings
	{
	public:
		JPH::SixDOFConstraintSettings motorSettings;
		float body2MassScale = 1.0f;
		JPH::TwoBodyConstraint *Create(JPH::Body &body1, JPH::Body &body2) const override;
	};

}
#endif
