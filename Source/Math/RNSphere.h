//
//  RNSphere.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SPHERE_H__
#define __RAYNE_SPHERE_H__

#include "../Base/RNBase.h"
#include "RNAABB.h"

namespace RN
{
	class Sphere
	{
	public:
		Sphere();
		Sphere(const Vector3 &offset, float radius);
		Sphere(const AABB &aabb);

		Sphere operator* (const Vector3 &other) const;
		Sphere &operator*= (const Vector3 &other);

		void SetRotation(const Quaternion &rotation);

		Vector3 position;
		Vector3 offset;
		float radius;
	};

	RN_INLINE Sphere::Sphere() :
		radius(0.0f)
	{}

	RN_INLINE Sphere::Sphere(const Vector3 &toffset, float tradius) :
		offset(toffset),
		radius(tradius)
	{
	}

	RN_INLINE Sphere::Sphere(const AABB &aabb) :
		position(aabb.position),
		offset(aabb.maxExtend * 0.5 + aabb.minExtend * 0.5),
		radius(((aabb.maxExtend - aabb.minExtend) * 0.5).GetLength())
	{
	}

	RN_INLINE Sphere Sphere::operator* (const Vector3 &other) const
	{
		float scale = std::max(std::max(other.x, other.y), other.z);

		Sphere result(*this);
		result.radius *= scale;
		result.offset *= scale;

		return result;
	}

	RN_INLINE Sphere &Sphere::operator*= (const Vector3 &other)
	{
		float scale = std::max(std::max(other.x, other.y), other.z);
		radius *= scale;
		offset *= scale;
		return *this;
	}

	RN_INLINE void Sphere::SetRotation(const Quaternion &rotation)
	{
		offset = rotation.GetRotatedVector(offset);
	}
}

#endif /* __RAYNE_SPHERE_H__ */