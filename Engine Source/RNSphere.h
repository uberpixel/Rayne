//
//  RNSphere.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SPHERE_H__
#define __RAYNE_SPHERE_H__

#include "RNBase.h"
#include "RNAABB.h"

namespace RN
{
	class Sphere
	{
	public:
		Sphere();
		Sphere(const Vector3& offset, float radius);
		Sphere(const AABB& aabb);
		
		Sphere operator+ (const Vector3& other) const;
		Sphere& operator+= (const Vector3& other);
		Sphere operator* (const Vector3& other) const;
		Sphere& operator*= (const Vector3& other);
		
		RN_INLINE Vector3 Position() const { return position + offset; }
		
		void Rotate(const Quaternion& rotation);
		
		Vector3 position;
		Vector3 offset;
		Vector3 offsetBase;
		
		float radius;
	};
	
	RN_INLINE Sphere::Sphere()
	{
		radius = 0.0f;
	}
	
	RN_INLINE Sphere::Sphere(const Vector3& toffset, float tradius) :
		offset(toffset),
		offsetBase(toffset),
		radius(tradius)
	{
	}
	
	RN_INLINE Sphere::Sphere(const AABB& aabb) :
		offset(aabb.maxExtend*0.5+aabb.minExtend*0.5),
		offsetBase(aabb.maxExtend*0.5+aabb.minExtend*0.5),
		radius(((aabb.maxExtend-aabb.minExtend)*0.5).Length())
	{
	}
	
	
	RN_INLINE Sphere Sphere::operator+ (const Vector3& other) const
	{
		Sphere result(*this);
		result.offset += other;
		result.offsetBase += other;
		return result;
	}
	
	RN_INLINE Sphere& Sphere::operator+= (const Vector3& other)
	{
		offset += other;
		offsetBase += other;
		return *this;
	}
	
	RN_INLINE Sphere Sphere::operator* (const Vector3& other) const
	{
		Sphere result(*this);
		float scale = MAX(MAX(other.x, other.y), other.z);
		result.radius *= scale;
		result.offset *= scale;
		result.offsetBase *= scale;
		
		return result;
	}
	
	RN_INLINE Sphere& Sphere::operator*= (const Vector3& other)
	{
		float scale = MAX(MAX(other.x, other.y), other.z);
		radius *= scale;
		offset *= scale;
		offsetBase *= scale;
		return *this;
	}
	
	RN_INLINE void Sphere::Rotate(const Quaternion& rotation)
	{
		offset = rotation.RotateVector(offsetBase);
	}
}

#endif /* __RAYNE_SPHERE_H__ */
