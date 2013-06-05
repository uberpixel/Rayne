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
		Sphere(const Vector3& origin, float radius);
		Sphere(const AABB& aabb);
		
		Sphere operator+ (const Vector3& other) const;
		Sphere& operator+= (const Vector3& other);
		Sphere operator* (const Vector3& other) const;
		Sphere& operator*= (const Vector3& other);
		
		RN_INLINE Vector3 Position() const { return origin + offset; }
		
		Vector3 offset;
		Vector3 origin;
		
		float radius;
	};
	
	RN_INLINE Sphere::Sphere()
	{
		radius = 0.0f;
	}
	
	RN_INLINE Sphere::Sphere(const Vector3& torigin, float tradius) :
		origin(torigin),
		radius(tradius)
	{
	}
	
	RN_INLINE Sphere::Sphere(const AABB& aabb) :
		origin(aabb.maxExtend*0.5+aabb.minExtend*0.5),
		radius(((aabb.maxExtend-aabb.minExtend)*0.5).Length())
	{
	}
	
	
	RN_INLINE Sphere Sphere::operator+ (const Vector3& other) const
	{
		Sphere result(*this);
		result.origin += other;
		
		return result;
	}
	
	RN_INLINE Sphere& Sphere::operator+= (const Vector3& other)
	{
		origin += other;
		return *this;
	}
	
	RN_INLINE Sphere Sphere::operator* (const Vector3& other) const
	{
		Sphere result(*this);
		float scale = MAX(MAX(other.x, other.y), other.z);
		result.radius *= scale;
		result.origin *= scale;
		
		return result;
	}
	
	RN_INLINE Sphere& Sphere::operator*= (const Vector3& other)
	{
		float scale = MAX(MAX(other.x, other.y), other.z);
		radius *= scale;
		origin *= scale;
		return *this;
	}
}

#endif /* __RAYNE_SPHERE_H__ */
