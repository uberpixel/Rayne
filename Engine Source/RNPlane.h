//
//  RNPlane.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PLANE_H__
#define __RAYNE_PLANE_H__
namespace RN
{
	class RenderingPipeline;
	class Plane
	{
	friend RenderingPipeline;
	public:
		Plane();

		void SetPlane(const Vector3 &position, const Vector3 &normal);
		void SetPlane(const Vector3 &position1, const Vector3 &position2, const Vector3 &position3);
		
		float Distance(const Vector3 &position);
	
	private:
		void CalcD();
		
		Vector3 _position;
		Vector3 _normal;
		float _d;
	};
	
	RN_INLINE Plane::Plane()
	{
		_normal.y = 1.0f;
	}
	
	RN_INLINE void Plane::SetPlane(const Vector3 &position, const Vector3 &normal)
	{
		_position = position;
		_normal = normal;
		_normal.Normalize();
		CalcD();
	}
	
	RN_INLINE void Plane::SetPlane(const Vector3 &position1, const Vector3 &position2, const Vector3 &position3)
	{
		_position = position1;
		Vector3 diff1 = position2-position1;
		Vector3 diff2 = position3-position1;
		_normal = diff1.Cross(diff2);
		_normal.Normalize();
		CalcD();
	}
	
	RN_INLINE float Plane::Distance(const Vector3 &position)
	{
		return position.Dot(_normal)-_d;
	}
	
	RN_INLINE void Plane::CalcD()
	{
		_d = _normal.Dot(_position);
	}
}

#endif //__RAYNE_PLANE_H__
