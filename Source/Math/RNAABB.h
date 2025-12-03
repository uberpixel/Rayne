
//  RNAABB.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_AABB_H__
#define __RAYNE_AABB_H__

#include "../Base/RNBase.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNVector.h"

namespace RN
{
	class AABB
	{
	public:
		AABB() = default;
		AABB(const Vector3 &min, const Vector3 &max);
		AABB(const Vector3 &pos, const float radius);

		AABB operator+(const AABB &other) const;
		AABB &operator+=(const AABB &other);

		AABB operator*(const Vector3 &other) const;
		AABB &operator*=(const Vector3 &other);

		bool Intersects(const AABB &other) const;
		bool Contains(const Vector3 &position) const;

		void SetRotation(const Quaternion &rotation);

		Vector3 position;
		Vector3 minExtend;
		Vector3 maxExtend;
	};

	RN_INLINE AABB::AABB(const Vector3 &tmin, const Vector3 &tmax)
	{
		minExtend.x = std::min(tmin.x, tmax.x);
		minExtend.y = std::min(tmin.y, tmax.y);
		minExtend.z = std::min(tmin.z, tmax.z);

		maxExtend.x = std::max(tmin.x, tmax.x);
		maxExtend.y = std::max(tmin.y, tmax.y);
		maxExtend.z = std::max(tmin.z, tmax.z);
	}

	RN_INLINE AABB::AABB(const Vector3 &pos, const float radius)
	{
		Vector3 dist(radius);

		minExtend = -dist;
		maxExtend = dist;

		position = pos;
	}

	RN_INLINE AABB AABB::operator+(const AABB &other) const
	{
		Vector3 min;
		Vector3 max;

		min.x = std::min(minExtend.x, other.minExtend.x);
		min.y = std::min(minExtend.y, other.minExtend.y);
		min.z = std::min(minExtend.z, other.minExtend.z);

		max.x = std::max(maxExtend.x, other.maxExtend.x);
		max.y = std::max(maxExtend.y, other.maxExtend.y);
		max.z = std::max(maxExtend.z, other.maxExtend.z);

		return AABB(min, max);
	}

	RN_INLINE AABB &AABB::operator+=(const AABB &other)
	{
		Vector3 min;
		Vector3 max;

		min.x = std::min(minExtend.x, other.minExtend.x);
		min.y = std::min(minExtend.y, other.minExtend.y);
		min.z = std::min(minExtend.z, other.minExtend.z);

		max.x = std::max(maxExtend.x, other.maxExtend.x);
		max.y = std::max(maxExtend.y, other.maxExtend.y);
		max.z = std::max(maxExtend.z, other.maxExtend.z);

		minExtend = min;
		maxExtend = max;

		return *this;
	}

	RN_INLINE AABB AABB::operator*(const Vector3 &other) const
	{
		AABB result = *this;

		result.minExtend *= other;
		result.maxExtend *= other;

		return result;
	}

	RN_INLINE AABB &AABB::operator*=(const Vector3 &other)
	{
		minExtend *= other;
		maxExtend *= other;

		return *this;
	}

	RN_INLINE bool AABB::Intersects(const AABB &other) const
	{
		Vector3 max0 = position + maxExtend;
		Vector3 max1 = other.position + other.maxExtend;

		Vector3 min0 = position + minExtend;
		Vector3 min1 = other.position + other.minExtend;

		if(min0.x > max1.x || min1.x > max0.x)
			return false;
		if(min0.y > max1.y || min1.y > max0.y)
			return false;
		if(min0.z > max1.z || min1.z > max0.z)
			return false;

		return true;
	}

	RN_INLINE bool AABB::Contains(const Vector3 &position) const
	{
		if(position.x - this->position.x > maxExtend.x)
			return false;
		if(position.x - this->position.x < minExtend.x)
			return false;
		if(position.y - this->position.y > maxExtend.y)
			return false;
		if(position.y - this->position.y < minExtend.y)
			return false;
		if(position.z - this->position.z > maxExtend.z)
			return false;
		if(position.z - this->position.z < minExtend.z)
			return false;

		return true;
	}

	RN_INLINE void AABB::SetRotation(const Quaternion &rotation)
	{
		Matrix matrix = rotation.GetRotationMatrix();

		Vector3 corners[8];
		corners[0] = matrix * Vector3(minExtend.x, minExtend.y, minExtend.z);
		corners[1] = matrix * Vector3(minExtend.x, minExtend.y, maxExtend.z);
		corners[2] = matrix * Vector3(minExtend.x, maxExtend.y, minExtend.z);
		corners[3] = matrix * Vector3(minExtend.x, maxExtend.y, maxExtend.z);
		corners[4] = matrix * Vector3(maxExtend.x, minExtend.y, minExtend.z);
		corners[5] = matrix * Vector3(maxExtend.x, minExtend.y, maxExtend.z);
		corners[6] = matrix * Vector3(maxExtend.x, maxExtend.y, minExtend.z);
		corners[7] = matrix * Vector3(maxExtend.x, maxExtend.y, maxExtend.z);

		minExtend = matrix * minExtend;
		maxExtend = matrix * maxExtend;

		for(Vector3 &corner : corners)
		{
			//This used to use std::min, but could trigger an exception in visual studio debug builds
			//if both values are equal, even though the standard explicitly allows it...
			minExtend.x = corner.x < minExtend.x ? corner.x : minExtend.x;
			minExtend.y = corner.y < minExtend.y ? corner.y : minExtend.y;
			minExtend.z = corner.z < minExtend.z ? corner.z : minExtend.z;

			maxExtend.x = corner.x > maxExtend.x ? corner.x : maxExtend.x;
			maxExtend.y = corner.y > maxExtend.y ? corner.y : maxExtend.y;
			maxExtend.z = corner.z > maxExtend.z ? corner.z : maxExtend.z;
		}
	}
} // namespace RN

#endif /* __RAYNE_AABB_H__ */
