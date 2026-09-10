#include "RNJoltScaledMotorConstraint.h"

namespace RN
{
	JPH::TwoBodyConstraint *JoltScaledMotorConstraintSettings::Create(JPH::Body &body1, JPH::Body &body2) const
	{
		auto constraint = new JoltScaledMotorConstraint(body1, body2, motorSettings);
		constraint->SetBody2MassScale(body2MassScale);
		return constraint;
	}

	JPH::Ref<JPH::ConstraintSettings> JoltScaledMotorConstraint::GetConstraintSettings() const
	{
		auto settings = new JoltScaledMotorConstraintSettings;
		auto motorSettings = _motor->GetConstraintSettings();
		settings->motorSettings = *static_cast<JPH::SixDOFConstraintSettings *>(motorSettings.GetPtr());
		ToConstraintSettings(settings->motorSettings);
		ToConstraintSettings(*settings);
		settings->body2MassScale = _body2MassScale;
		return settings;
	}
}
