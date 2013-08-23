//
//  RNPlane.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PLANE_H__
#define __RAYNE_PLANE_H__

namespace RN
{
	class Plane
	{
	public:
		RNAPI Plane();

		RNAPI void SetPlane(const Vector3 &position, const Vector3 &normal);
		RNAPI void SetPlane(const Vector3 &position1, const Vector3 &position2, const Vector3 &position3, float dirfac=1.0f);
		
		RNAPI float GetDistance(const Vector3 &position) const;
		RNAPI void CalculateD();
		
		Vector3 position;
		Vector3 normal;
		float d;
	};
	
	RN_INLINE Plane::Plane()
	{
		normal.y = 1.0f;
	}
	
	RN_INLINE void Plane::SetPlane(const Vector3 &tposition, const Vector3 &tnormal)
	{
		position = tposition;
		normal = tnormal;
		normal.Normalize();
		
		CalculateD();
	}
	
	RN_INLINE void Plane::SetPlane(const Vector3 &position1, const Vector3 &position2, const Vector3 &position3, float dirfac)
	{
		Vector3 diff1 = position2 - position1;
		Vector3 diff2 = position3 - position1;
		
		position = position1;
		
		normal = diff1.Cross(diff2)*dirfac;
		normal.Normalize();
		
		CalculateD();
	}
	
	RN_INLINE float Plane::GetDistance(const Vector3 &position) const
	{
		return position.Dot(normal) - d;
	}
	
	RN_INLINE void Plane::CalculateD()
	{
		d = normal.Dot(position);
	}
}

#endif //__RAYNE_PLANE_H__
