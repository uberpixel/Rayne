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
#include "RNMatrix.h"
#include "RNQuaternion.h"

namespace RN
{
	class AABB
	{
	public:
		AABB();
		AABB(const Vector3& min, const Vector3& max);
		AABB(const Vector3& pos, const float radius);
		AABB(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
		
		AABB operator+ (const AABB& other) const;
		AABB& operator+= (const AABB& other);
		
		AABB operator* (const Vector3& other) const;
		AABB& operator*= (const Vector3& other);
		
		bool Intersects(const AABB& other) const;
		
		void Rotate(const Quaternion& rotation);
		
		Vector3 position;
		Vector3 minExtend;
		Vector3 maxExtend;
		
		Vector3 minExtendBase;
		Vector3 maxExtendBase;
	};
	
	RN_INLINE AABB::AABB()
	{
	}
	
	RN_INLINE AABB::AABB(const Vector3& tmin, const Vector3& tmax)
	{		
		minExtendBase.x = MIN(tmin.x, tmax.x);
		minExtendBase.y = MIN(tmin.y, tmax.y);
		minExtendBase.z = MIN(tmin.z, tmax.z);
		
		maxExtendBase.x = MAX(tmin.x, tmax.x);
		maxExtendBase.y = MAX(tmin.y, tmax.y);
		maxExtendBase.z = MAX(tmin.z, tmax.z);
		
		minExtend = minExtendBase;
		maxExtend = maxExtendBase;
	}
	
	RN_INLINE AABB::AABB(const Vector3& pos, const float radius)
	{
		Vector3 dist = radius;
		
		minExtendBase = pos-dist;
		maxExtendBase = pos+dist;
		minExtend = minExtendBase;
		maxExtend = maxExtendBase;
	}
	
	RN_INLINE AABB::AABB(float minX, float minY, float minZ, float maxX, float maxY, float maxZ) :
		AABB(Vector3(minX, minY, minZ), Vector3(maxX, maxY, maxZ))
	{}
	
	RN_INLINE AABB AABB::operator+ (const AABB& other) const
	{
		Vector3 min;
		Vector3 max;
		
		min.x = MIN(minExtend.x, other.minExtend.x);
		min.y = MIN(minExtend.y, other.minExtend.y);
		min.z = MIN(minExtend.z, other.minExtend.z);
		
		max.x = MAX(maxExtend.x, other.maxExtend.x);
		max.y = MAX(maxExtend.y, other.maxExtend.y);
		max.z = MAX(maxExtend.z, other.maxExtend.z);
		
		return AABB(min, max);
	}
	
	RN_INLINE AABB& AABB::operator+= (const AABB& other)
	{
		Vector3 min;
		Vector3 max;
		
		min.x = MIN(minExtend.x, other.minExtend.x);
		min.y = MIN(minExtend.y, other.minExtend.y);
		min.z = MIN(minExtend.z, other.minExtend.z);
		
		max.x = MAX(maxExtend.x, other.maxExtend.x);
		max.y = MAX(maxExtend.y, other.maxExtend.y);
		max.z = MAX(maxExtend.z, other.maxExtend.z);
		
		minExtendBase = minExtend = min;
		maxExtendBase = maxExtend = max;
		
		return *this;
	}
	
	RN_INLINE AABB AABB::operator* (const Vector3& other) const
	{
		AABB result = *this;
		result.minExtend *= other;
		result.maxExtend *= other;
		result.minExtendBase *= other;
		result.maxExtendBase *= other;
		
		return result;
	}
	
	RN_INLINE AABB& AABB::operator*= (const Vector3& other)
	{
		minExtend *= other;
		maxExtend *= other;
		minExtendBase *= other;
		maxExtendBase *= other;
		
		return *this;
	}
	
	RN_INLINE bool AABB::Intersects(const AABB& other) const
	{
		if(Math::FastAbs(position.x - other.position.x) > (minExtend.x + other.maxExtend.x))
			return false;
		if(Math::FastAbs(position.y - other.position.y) > (minExtend.y + other.maxExtend.y))
			return false;
		if(Math::FastAbs(position.z - other.position.z) > (minExtend.z + other.maxExtend.z))
			return false;
		
		return true;
	}
	
	RN_INLINE void AABB::Rotate(const Quaternion& rotation)
	{
		Matrix matrix = rotation.GetRotationMatrix();
		
		Vector3 corners[4];
		corners[0] = matrix.Transform(Vector3(minExtendBase.x, minExtendBase.y, maxExtendBase.z));
		corners[1] = matrix.Transform(Vector3(minExtendBase.x, maxExtendBase.y, maxExtendBase.z));
		corners[2] = matrix.Transform(Vector3(maxExtendBase.x, maxExtendBase.y, minExtendBase.z));
		corners[3] = matrix.Transform(Vector3(maxExtendBase.x, minExtendBase.y, minExtendBase.z));
		
		minExtend = matrix.Transform(minExtendBase);
		maxExtend = matrix.Transform(maxExtendBase);
		
		for(size_t i=0; i<4; i++)
		{
			minExtend.x = MIN(corners[i].x, minExtend.x);
			minExtend.y = MIN(corners[i].y, minExtend.y);
			minExtend.z = MIN(corners[i].z, minExtend.z);
			
			maxExtend.x = MAX(corners[i].x, maxExtend.x);
			maxExtend.y = MAX(corners[i].y, maxExtend.y);
			maxExtend.z = MAX(corners[i].z, maxExtend.z);
		}
	}
}

#endif /* __RAYNE_AABB_H__ */
