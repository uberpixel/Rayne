//
//  RNAABB.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_AABB_H__
#define __RAYNE_AABB_H__

#include "RNBase.h"
#include "RNVector.h"

namespace RN
{
	class AABB
	{
	public:
		AABB();
		AABB(const Vector3& min, const Vector3& max);
		AABB(const Vector3& origin, float width);
		AABB(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
		
		AABB operator+ (const Vector3& other) const;
		AABB operator+ (const AABB& other) const;
		
		AABB& operator+= (const Vector3& other);
		AABB& operator+= (const AABB& other);
		
		bool Intersects(const AABB& other);
		bool Contains(const AABB& other);
		
		float Width() const { return halfWidth.x * 2.0f; }
		float Height() const { return halfWidth.y * 2.0f; }
		
		Vector3 origin;
		Vector3 halfWidth;
	};
	
	RN_INLINE AABB::AABB()
	{
	}
	
	RN_INLINE AABB::AABB(const Vector3& tmin, const Vector3& tmax)
	{
		Vector3 min;
		Vector3 max;
		
		min.x = MIN(tmin.x, tmax.x);
		min.y = MIN(tmin.x, tmax.y);
		min.z = MIN(tmin.x, tmax.z);
		
		max.x = MAX(tmin.x, tmax.x);
		max.y = MAX(tmin.x, tmax.y);
		max.z = MAX(tmin.x, tmax.z);
		
		halfWidth = (max - min) * 0.5f;
		origin    = min + halfWidth;		
	}
	
	RN_INLINE AABB::AABB(const Vector3& torigin, float width) :
		origin(torigin),
		halfWidth(Vector3(width))
	{
	}
	
	RN_INLINE AABB::AABB(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) :
		AABB(Vector3(minX, minY, minZ), Vector3(maxX, maxY, maxZ))
	{}
	
	
	RN_INLINE AABB AABB::operator+ (const Vector3& other) const
	{
		AABB result(*this);
		result.origin += other;
		
		return result;
	}
	
	RN_INLINE AABB AABB::operator+ (const AABB& other) const
	{
		Vector3 min;
		Vector3 max;
		
		Vector3 min1(origin - halfWidth);
		Vector3 min2(other.origin - other.halfWidth);
		
		Vector3 max1(origin + halfWidth);
		Vector3 max2(other.origin + other.halfWidth);
		
		min.x = MIN(min1.x, min2.x);
		min.y = MIN(min1.x, min2.y);
		min.z = MIN(min1.x, min2.z);
		
		max.x = MAX(max1.x, max2.x);
		max.y = MAX(max1.x, max2.y);
		max.z = MAX(max1.x, max2.z);
		
		return AABB(min, max);
	}
	
	RN_INLINE AABB& AABB::operator+= (const Vector3& other)
	{
		Vector3 min;
		Vector3 max;
		
		Vector3 tmin(origin - halfWidth);
		Vector3 tmax(origin + halfWidth);
		
		min.x = MIN(tmin.x, other.x);
		min.y = MIN(tmin.x, other.y);
		min.z = MIN(tmin.x, other.z);
		
		max.x = MAX(tmax.x, other.x);
		max.y = MAX(tmax.x, other.y);
		max.z = MAX(tmax.x, other.z);
		
		halfWidth = (max - min) * 0.5f;
		origin    = min + halfWidth;
		
		return *this;
	}
	
	RN_INLINE AABB& AABB::operator+= (const AABB& other)
	{
		Vector3 min;
		Vector3 max;
		
		Vector3 min1(origin - halfWidth);
		Vector3 min2(other.origin - other.halfWidth);
		
		Vector3 max1(origin + halfWidth);
		Vector3 max2(other.origin + other.halfWidth);
		
		min.x = MIN(min1.x, min2.x);
		min.y = MIN(min1.x, min2.y);
		min.z = MIN(min1.x, min2.z);
		
		max.x = MAX(max1.x, max2.x);
		max.y = MAX(max1.x, max2.y);
		max.z = MAX(max1.x, max2.z);
		
		halfWidth = (max - min) * 0.5f;
		origin    = min + halfWidth;
		
		return *this;
	}
	
	
	
	RN_INLINE bool AABB::Intersects(const AABB& other)
	{
		if(fabsf(origin.x - other.origin.x) > (halfWidth.x + other.halfWidth.x))
			return false;
		if(fabsf(origin.y - other.origin.y) > (halfWidth.y + other.halfWidth.y))
			return false;
		if(fabsf(origin.z - other.origin.z) > (halfWidth.z + other.halfWidth.z))
			return false;
		
		return true;
	}
	
	RN_INLINE bool AABB::Contains(const AABB& other)
	{
		return  origin.x - halfWidth.x <= other.origin.x - other.halfWidth.x &&
				origin.x + halfWidth.x >= other.origin.x + halfWidth.x &&
		
				origin.y - halfWidth.y <= other.origin.y - other.halfWidth.y &&
				origin.y + halfWidth.y >= other.origin.y + halfWidth.y &&
		
				origin.z - halfWidth.z <= other.origin.z - other.halfWidth.z &&
				origin.z + halfWidth.z >= other.origin.z + halfWidth.z;
	}
}

#endif /* __RAYNE_AABB_H__ */
