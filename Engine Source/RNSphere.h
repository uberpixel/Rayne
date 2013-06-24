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
#include "RNHit.h"

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
		
		Hit CastRay(const Vector3 &position, const Vector3 &direction) const;
		bool IntersectsRay(const Vector3 &position, const Vector3 &direction) const;
		
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
	
	/*based on http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection*/
	RN_INLINE Hit Sphere::CastRay(const Vector3 &position, const Vector3 &direction) const
	{
		Hit hit;
		
		//Compute A, B and C coefficients
		float a = direction.Dot(direction);
		float b = 2.0f * direction.Dot(position);
		float c = position.Dot(position) - (radius * radius);
		
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
			hit.position = position+direction*t1;
			hit.distance = t1;
			return hit;
		}
		else
		{
			hit.position = position+direction*t0;
			hit.distance = t0;
			return hit;
		}
	}
	
	RN_INLINE bool Sphere::IntersectsRay(const Vector3 &position, const Vector3 &direction) const
	{
				float dist = direction.Cross(position-Position()).Length()/direction.Length();
		 
		 return (dist <= radius);
	}
}

#endif /* __RAYNE_SPHERE_H__ */
