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
		Sphere(const Sphere& other);
		Sphere(const Vector3& origin, float radius);
		Sphere(const AABB& aabb);
		
		Vector3 origin;
		float radius;
	};
	
	RN_INLINE Sphere::Sphere()
	{
		radius = 0.0f;
	}
	
	RN_INLINE Sphere::Sphere(const Sphere& other) :
		origin(other.origin),
		radius(other.radius)
	{
	}
	
	RN_INLINE Sphere::Sphere(const Vector3& torigin, float tradius) :
		origin(torigin),
		radius(tradius)
	{
	}
	
	RN_INLINE Sphere::Sphere(const AABB& aabb) :
		origin(aabb.origin),
		radius(aabb.halfWidth.Length())
	{
	}
}

#endif /* __RAYNE_SPHERE_H__ */
