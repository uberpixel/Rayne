//
//  RNJoltConversions.h
//  Rayne-Jolt
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTCONVERSIONS_H_
#define __RAYNE_JOLTCONVERSIONS_H_

#include "RNJoltWorld.h"

namespace RN
{
	class JoltConversions
	{
	public:
		// All conversions require a non-null world; velocity positions are in universe space.
		// Absolute point velocities include frame motion; deltas, forces and directions use ConvertVector.
		static DVector3 ConvertLinearVelocityFromPhysicsWorld(const Vector3 &velocity, const PositionType &position, const JoltWorld *world)
		{
			return world->_worldLinearVelocity + DVector3(world->_worldAngularVelocity).GetCrossProduct(DVector3(position) - DVector3(world->_worldPosition)) + world->_worldRotation.GetRotatedVector(DVector3(velocity));
		}

		static Vector3 ConvertLinearVelocityToPhysicsWorld(const DVector3 &velocity, const PositionType &position, const JoltWorld *world)
		{
			const DVector3 relative = velocity - world->_worldLinearVelocity - DVector3(world->_worldAngularVelocity).GetCrossProduct(DVector3(position) - DVector3(world->_worldPosition));
			return Vector3(world->_inverseWorldRotation.GetRotatedVector(relative));
		}

		static Vector3 ConvertAngularVelocityFromPhysicsWorld(const Vector3 &velocity, const JoltWorld *world)
		{
			return world->_worldAngularVelocity + ConvertVectorFromPhysicsWorld(velocity, world);
		}

		static Vector3 ConvertAngularVelocityToPhysicsWorld(const Vector3 &velocity, const JoltWorld *world)
		{
			return ConvertVectorToPhysicsWorld(velocity - world->_worldAngularVelocity, world);
		}

		static JoltPosition ConvertPositionToPhysicsWorld(const PositionType &position, const JoltWorld *world)
		{
			DVector3 offset(position);
			offset -= DVector3(world->_worldPosition);
			const DVector3 physicsPosition = world->_inverseWorldPositionRotation.GetRotatedVector(offset);
			return JoltPosition(physicsPosition);
		}

		static PositionType ConvertPositionFromPhysicsWorld(const JoltPosition &position, const JoltWorld *world)
		{
			const DVector3 worldPosition = DVector3(world->_worldPosition) + world->_worldPositionRotation.GetRotatedVector(DVector3(position));
			return PositionType(worldPosition);
		}

		static Vector3 ConvertVectorToPhysicsWorld(const Vector3 &vector, const JoltWorld *world)
		{
			return world->_inverseWorldRotation.GetRotatedVector(vector);
		}

		static Vector3 ConvertVectorFromPhysicsWorld(const Vector3 &vector, const JoltWorld *world)
		{
			return world->_worldRotation.GetRotatedVector(vector);
		}

		static Quaternion ConvertRotationToPhysicsWorld(const Quaternion &rotation, const JoltWorld *world)
		{
			Quaternion result = world->_inverseWorldRotation * rotation;
			result.Normalize();
			return result;
		}

		static Quaternion ConvertRotationFromPhysicsWorld(const Quaternion &rotation, const JoltWorld *world)
		{
			Quaternion result = world->_worldRotation * rotation;
			result.Normalize();
			return result;
		}
	};
} // namespace RN

#endif /* __RAYNE_JOLTCONVERSIONS_H_ */
