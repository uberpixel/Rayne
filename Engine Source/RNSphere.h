//
//  RNSphere.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SPHERE_H__
#define __RAYNE_SPHERE_H__

#include "RNBase.h"
#include "RNAABB.h"
#include "RNHit.h"

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
		
		Hit CastRay(const Vector3 &position, const Vector3 &direction) const;
		bool IntersectsRay(const Vector3 &position, const Vector3 &direction) const;
		
		Vector3 position;
		Vector3 offset;
		float radius;
	};
	
	RN_INLINE Sphere::Sphere()
	{
		radius = 0.0f;
	}
	
	RN_INLINE Sphere::Sphere(const Vector3 &toffset, float tradius) :
		offset(toffset),
		radius(tradius)
	{
	}
	
	RN_INLINE Sphere::Sphere(const AABB &aabb) :
		position(aabb.position),
		offset(aabb.maxExtend*0.5+aabb.minExtend*0.5),
		radius(((aabb.maxExtend-aabb.minExtend)*0.5).GetLength())
	{
	}
	
	RN_INLINE Sphere Sphere::operator* (const Vector3 &other) const
	{
		Sphere result(*this);
		float scale = std::max(std::max(other.x, other.y), other.z);
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
	
	/*based on http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection*/
	RN_INLINE Hit Sphere::CastRay(const Vector3 &position, const Vector3 &direction) const
	{
		Hit hit;
		
		Vector3 pos = position - this->position - this->offset;
		
		//Compute A, B and C coefficients
		float a = direction.GetDotProduct(direction);
		float b = 2.0f * direction.GetDotProduct(pos);
		float c = pos.GetDotProduct(pos) - (radius * radius);
		
		//Find discriminant
		float disc = b * b - 4.0f * a * c;
		
		// if discriminant is negative there are no real roots, so return
		// false as ray misses sphere
		if (disc < 0.0f)
			return hit;
		
		// compute q as described above
		float distSqrt = sqrtf(disc);
		float q;
		if (b < 0.0f)
			q = (-b - distSqrt)/2.0f;
		else
			q = (-b + distSqrt)/2.0f;
		
		// compute t0 and t1
		float t0 = q / a;
		float t1 = c / q;
		
		// make sure t0 is smaller than t1
		if (t0 > t1)
		{
			// if t0 is bigger than t1 swap them around
			float temp = t0;
			t0 = t1;
			t1 = temp;
		}
		
		// if t1 is less than zero, the object is in the ray's negative direction
		// and consequently the ray misses the sphere
		if (t1 < 0.0f)
			return hit;
		
		// if t0 is less than zero, the intersection point is at t1
		if (t0 < 0.0f)
		{
			hit.position = this->position + this->offset + direction * t1;
			hit.distance = t1;
			return hit;
		}
		else
		{
			hit.position = this->position + this->offset + direction * t0;
			hit.distance = t0;
			return hit;
		}
	}
	
	RN_INLINE bool Sphere::IntersectsRay(const Vector3 &tposition, const Vector3 &direction) const
	{
		float dist = direction.GetCrossProduct(tposition - position - offset).GetLength()/direction.GetLength();
		return (dist <= radius);
	}
}

#endif /* __RAYNE_SPHERE_H__ */
