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
		bool Contains(const Vector3 &position) const;
		
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
		minExtendBase.x = std::min(tmin.x, tmax.x);
		minExtendBase.y = std::min(tmin.y, tmax.y);
		minExtendBase.z = std::min(tmin.z, tmax.z);
		
		maxExtendBase.x = std::max(tmin.x, tmax.x);
		maxExtendBase.y = std::max(tmin.y, tmax.y);
		maxExtendBase.z = std::max(tmin.z, tmax.z);
		
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
		
		min.x = std::min(minExtend.x, other.minExtend.x);
		min.y = std::min(minExtend.y, other.minExtend.y);
		min.z = std::min(minExtend.z, other.minExtend.z);
		
		max.x = std::max(maxExtend.x, other.maxExtend.x);
		max.y = std::max(maxExtend.y, other.maxExtend.y);
		max.z = std::max(maxExtend.z, other.maxExtend.z);
		
		return AABB(min, max);
	}
	
	RN_INLINE AABB& AABB::operator+= (const AABB& other)
	{
		Vector3 min;
		Vector3 max;
		
		min.x = std::min(minExtend.x, other.minExtend.x);
		min.y = std::min(minExtend.y, other.minExtend.y);
		min.z = std::min(minExtend.z, other.minExtend.z);
		
		max.x = std::max(maxExtend.x, other.maxExtend.x);
		max.y = std::max(maxExtend.y, other.maxExtend.y);
		max.z = std::max(maxExtend.z, other.maxExtend.z);
		
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
		if(other.position.x - position.x > maxExtend.x + other.minExtend.x)
			return false;
		if(other.position.x - position.x < minExtend.x + other.maxExtend.x)
			return false;
		if(other.position.y - position.y > maxExtend.y + other.minExtend.y)
			return false;
		if(other.position.y - position.y < minExtend.y + other.maxExtend.y)
			return false;
		if(other.position.z - position.z > maxExtend.z + other.minExtend.z)
			return false;
		if(other.position.z - position.z < minExtend.z + other.maxExtend.z)
			return false;
		
		return true;
	}
	
	RN_INLINE bool AABB::Contains(const Vector3& position) const
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
			minExtend.x = std::min(corners[i].x, minExtend.x);
			minExtend.y = std::min(corners[i].y, minExtend.y);
			minExtend.z = std::min(corners[i].z, minExtend.z);
			
			maxExtend.x = std::max(corners[i].x, maxExtend.x);
			maxExtend.y = std::max(corners[i].y, maxExtend.y);
			maxExtend.z = std::max(corners[i].z, maxExtend.z);
		}
	}
}

#endif /* __RAYNE_AABB_H__ */
