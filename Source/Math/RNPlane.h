//
//  RNPlane.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_PLANE_H__
#define __RAYNE_PLANE_H__

namespace RN
{
	class Plane
	{
	public:
		Plane();
		static Plane WithPositionNormal(const Vector3 &position, const Vector3 &normal);
		static Plane WithTriangle(const Vector3 &position1, const Vector3 &position2, const Vector3 &position3, float dirfac=1.0f);

		void SetPosition(const Vector3 &position);
		void SetNormal(const Vector3 &normal);

		float GetDistance(const Vector3 &position) const;
		Vector3 GetPosition() const { return _position; }
		Vector3 GetNormal() const { return _normal; }
		float GetD() const { return _d; }

//		Hit CastRay(const Vector3 &position, const Vector3 &direction) const;

	private:
		void CalculateD();
		Vector3 _position;
		Vector3 _normal;
		float _d;
	};

	RN_INLINE Plane::Plane()
	{
		_normal.y = 1.0f;
		_d = 0.0f;
	}

	RN_INLINE Plane Plane::WithPositionNormal(const Vector3 &position, const Vector3 &normal)
	{
		Plane plane;

		plane._position = position;
		plane._normal = normal;
		plane._normal.Normalize();
		plane.CalculateD();

		return plane;
	}

	RN_INLINE Plane Plane::WithTriangle(const Vector3 &position1, const Vector3 &position2, const Vector3 &position3, float dirfac)
	{
		Plane plane;

		Vector3 diff1 = position2 - position1;
		Vector3 diff2 = position3 - position1;

		plane._position = position1;

		plane._normal = diff1.GetCrossProduct(diff2)*dirfac;
		plane._normal.Normalize();

		plane.CalculateD();

		return plane;
	}

	RN_INLINE void Plane::SetPosition(const Vector3 &position)
	{
		_position = position;
		CalculateD();
	}

	RN_INLINE void Plane::SetNormal(const Vector3 &normal)
	{
		_normal = normal;
		CalculateD();
	}

	RN_INLINE float Plane::GetDistance(const Vector3 &position) const
	{
		return _position.GetDotProduct(_normal) - _d;
	}

	RN_INLINE void Plane::CalculateD()
	{
		_d = _normal.GetDotProduct(_position);
	}

/*	RN_INLINE Hit Plane::CastRay(const Vector3 &position, const Vector3 &direction) const
	{
		Hit hit;
		Vector3 normalizedDirection = direction.GetNormalized();
		float angleCos = normalizedDirection.GetDotProduct(_normal);
		if(angleCos >= -k::EpsilonFloat && angleCos <= k::EpsilonFloat)
			return hit;

		if(((GetDistance(position) > 0)?1:-1) == ((angleCos)?1:-1))
			return hit;

		float fac = (_position-position).GetDotProduct(_normal)/angleCos;
		hit.position = position + normalizedDirection * fac;
		hit.distance = fac;

		return hit;
	}*/
}

#endif //__RAYNE_PLANE_H__
