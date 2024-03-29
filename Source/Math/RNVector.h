//
//  RNVector.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VECTOR_H__
#define __RAYNE_VECTOR_H__

#include "../Base/RNBase.h"

namespace RN
{
	class Vector3;
	class Vector4;
	
	class Vector2
	{
	public:
		Vector2();
		Vector2(const float n);
		Vector2(const float x, const float y);
		explicit Vector2(const Vector3 &other);
		explicit Vector2(const Vector4 &other);

		bool operator== (const Vector2 &other) const;
		bool operator!= (const Vector2 &other) const;

		Vector2 operator- () const;

		Vector2 operator+ (const Vector2 &other) const;
		Vector2 operator- (const Vector2 &other) const;
		Vector2 operator* (const Vector2 &other) const;
		Vector2 operator/ (const Vector2 &other) const;
		Vector2 operator* (const float n) const;
		Vector2 operator/ (const float n) const;

		Vector2 &operator+= (const Vector2 &other);
		Vector2 &operator-= (const Vector2 &other);
		Vector2 &operator*= (const Vector2 &other);
		Vector2 &operator/= (const Vector2 &other);

		float GetLength() const;
		float GetMax() const;
		float GetMin() const;
		float GetDotProduct(const Vector2 &other) const;
		Vector2 GetCrossProduct() const;
		float GetDistance(const Vector2 &other) const;
		float GetSquaredDistance(const Vector2 &other) const;
		Vector2 GetLerp(const Vector2 &other, float factor) const;
		bool IsEqual(const Vector2 &other, float epsilon) const;
		
		bool IsValid() const;

		Vector2 &Normalize(const float n=1.0f);
		Vector2 GetNormalized(const float n=1.0f) const;

		struct
		{
			float x;
			float y;
		};
	};

	class Vector3
	{
	public:
		Vector3();
		Vector3(const float n);
		Vector3(const float x, const float y, const float z);
		explicit Vector3(const Vector2 &other, float z=0.0f);
		explicit Vector3(const Vector4 &other);
		
		bool operator== (const Vector3 &other) const;
		bool operator!= (const Vector3 &other) const;
		
		Vector3 operator- () const;
		
		Vector3 operator+ (const Vector3 &other) const;
		Vector3 operator- (const Vector3 &other) const;
		Vector3 operator* (const Vector3 &other) const;
		Vector3 operator/ (const Vector3 &other) const;
		Vector3 operator* (const float n) const;
		Vector3 operator/ (const float n) const;
		
		Vector3 &operator+= (const Vector3 &other);
		Vector3 &operator-= (const Vector3 &other);
		Vector3 &operator*= (const Vector3 &other);
		Vector3 &operator/= (const Vector3 &other);
		
		float GetLength() const;
		float GetMax() const;
		float GetMin() const;
		float GetDotProduct(const Vector3 &other) const;
		Vector3 GetCrossProduct(const Vector3 &other) const;
		bool IsEqual(const Vector3 &other, float epsilon) const;
		float GetDistance(const Vector3 &other) const;
		float GetSquaredDistance(const Vector3 &other) const;
		float GetDistanceToSegment(const Vector3 &a, const Vector3 &b) const;
		Vector3 GetLerp(const Vector3 &other, float factor) const;
		
		bool IsValid() const;
		
		Vector3 &Normalize(const float n=1.0f);
		Vector3 GetNormalized(const float n=1.0f) const;
		
		struct
		{
			float x;
			float y;
			float z;
		};
	};
	
	class RN_ALIGNAS(16) Vector4
	{
	public:
		Vector4();
		Vector4(const float n);
		Vector4(const float x, const float y, const float z, const float w);
		explicit Vector4(const Vector2 &other, float z=0.0f, float w=0.0f);
		explicit Vector4(const Vector3 &other, float w=0.0f);
		
		bool operator== (const Vector4 &other) const;
		bool operator!= (const Vector4 &other) const;
		
		Vector4 operator- () const;
		
		Vector4 operator+ (const Vector4 &other) const;
		Vector4 operator- (const Vector4 &other) const;
		Vector4 operator* (const Vector4 &other) const;
		Vector4 operator/ (const Vector4 &other) const;
		Vector4 operator* (const float n) const;
		Vector4 operator/ (const float n) const;
		
		Vector4 &operator+= (const Vector4 &other);
		Vector4 &operator-= (const Vector4 &other);
		Vector4 &operator*= (const Vector4 &other);
		Vector4 &operator/= (const Vector4 &other);
		
		float GetLength() const;
		float GetMax() const;
		float GetMin() const;
		float GetDotProduct(const Vector4 &other) const;
		float GetDistance(const Vector4 &other) const;
		float GetSquaredDistance(const Vector4 &other) const;
		Vector4 GetLerp(const Vector4 &other, float factor) const;
		bool IsEqual(const Vector4 &other, float epsilon) const;
		
		bool IsValid() const;
		
		Vector4 &Normalize(const float n=1.0f);
		Vector4 GetNormalized(const float n=1.0f) const;
		
		struct
		{
			float x;
			float y;
			float z;
			float w;
		};
	};
	
	
	
	RN_INLINE Vector2::Vector2()
	{
		x = y = 0.0f;
	}

	RN_INLINE Vector2::Vector2(const float n)
	{
		x = y = n;
	}

	RN_INLINE Vector2::Vector2(const float _x, const float _y)
	{
		x = _x;
		y = _y;
	}
	
	RN_INLINE Vector2::Vector2(const Vector3 &other)
	{
		x = other.x;
		y = other.y;
	}
	
	RN_INLINE Vector2::Vector2(const Vector4 &other)
	{
		x = other.x;
		y = other.y;
	}

	RN_INLINE bool Vector2::operator== (const Vector2 &other) const
	{
		if(Math::FastAbs(x - other.x) > k::EpsilonFloat)
			return false;

		if(Math::FastAbs(y - other.y) > k::EpsilonFloat)
			return false;

		return true;
	}

	RN_INLINE bool Vector2::operator!= (const Vector2 &other) const
	{
		if(Math::FastAbs(x - other.x) <= k::EpsilonFloat && Math::FastAbs(y - other.y) <= k::EpsilonFloat)
			return false;

		return true;
	}

	RN_INLINE Vector2 Vector2::operator- () const
	{
		return Vector2(-x, -y);
	}

	RN_INLINE Vector2 Vector2::operator+ (const Vector2 &other) const
	{
		return Vector2(x + other.x, y + other.y);
	}
	RN_INLINE Vector2 Vector2::operator- (const Vector2 &other) const
	{
		return Vector2(x - other.x, y - other.y);
	}
	RN_INLINE Vector2 Vector2::operator* (const Vector2 &other) const
	{
		return Vector2(x * other.x, y * other.y);
	}
	RN_INLINE Vector2 Vector2::operator/ (const Vector2 &other) const
	{
		return Vector2(x / other.x, y / other.y);
	}
	RN_INLINE Vector2 Vector2::operator* (const float n) const
	{
		return Vector2(x * n, y * n);
	}
	RN_INLINE Vector2 Vector2::operator/ (const float n) const
	{
		return Vector2(x / n, y / n);
	}

	RN_INLINE Vector2 &Vector2::operator+= (const Vector2 &other)
	{
		x += other.x;
		y += other.y;

		return *this;
	}
	RN_INLINE Vector2 &Vector2::operator-= (const Vector2 &other)
	{
		x -= other.x;
		y -= other.y;

		return *this;
	}
	RN_INLINE Vector2 &Vector2::operator*= (const Vector2 &other)
	{
		x *= other.x;
		y *= other.y;

		return *this;
	}
	RN_INLINE Vector2 &Vector2::operator/= (const Vector2 &other)
	{
		x /= other.x;
		y /= other.y;

		return *this;
	}

	RN_INLINE float Vector2::GetLength() const
	{
		return Math::Sqrt(x * x + y * y);
	}
	
	RN_INLINE float Vector2::GetMax() const
	{
		return std::max(x, y);
	}
	
	RN_INLINE float Vector2::GetMin() const
	{
		return std::min(x, y);
	}

	RN_INLINE float Vector2::GetDotProduct(const Vector2 &other) const
	{
		return (x * other.x + y * other.y);
	}
	
	RN_INLINE Vector2 Vector2::GetCrossProduct() const
	{
		return Vector2(y, -x);
	}

	RN_INLINE bool Vector2::IsEqual(const Vector2 &other, float epsilon) const
	{
		if(Math::FastAbs(x - other.x) > epsilon)
			return false;

		if(Math::FastAbs(y - other.y) > epsilon)
			return false;

		return true;
	}

	RN_INLINE bool Vector2::IsValid() const
	{
		if(!std::isfinite(x))
			return false;

		if(!std::isfinite(y))
			return false;

		return true;
	}

	RN_INLINE Vector2 &Vector2::Normalize(const float n)
	{
		if(x*x+y*y > k::EpsilonFloat)
		{
			float invlength = n*Math::InverseSqrt(x*x+y*y);
			x *= invlength;
			y *= invlength;
		}

		return *this;
	}
	
	RN_INLINE Vector2 Vector2::GetNormalized(const float n) const
	{
		return Vector2(*this).Normalize(n);
	}
	
	RN_INLINE float Vector2::GetDistance(const Vector2 &other) const
	{
		Vector2 difference = *this - other;
		return difference.GetLength();
	}
	
	RN_INLINE float Vector2::GetSquaredDistance(const Vector2 &other) const
	{
		Vector2 difference = *this - other;
		return difference.GetDotProduct(difference);
	}
	
	RN_INLINE Vector2 Vector2::GetLerp(const Vector2 &other, float factor) const
	{
		return *this*(1.0f-factor)+other*factor;
	}

	
	

	RN_INLINE Vector3::Vector3()
	{
		x = y = z = 0.0f;
	}

	RN_INLINE Vector3::Vector3(const float n)
	{
		x = y = z = n;
	}

	RN_INLINE Vector3::Vector3(const float _x, const float _y, const float _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}
	
	RN_INLINE Vector3::Vector3(const Vector2 &other, float _z)
	{
		x = other.x;
		y = other.y;
		z = _z;
	}
	
	RN_INLINE Vector3::Vector3(const Vector4 &other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
	}

	RN_INLINE bool Vector3::operator== (const Vector3 &other) const
	{
		if(Math::FastAbs(x - other.x) > k::EpsilonFloat)
			return false;

		if(Math::FastAbs(y - other.y) > k::EpsilonFloat)
			return false;

		if(Math::FastAbs(z - other.z) > k::EpsilonFloat)
			return false;

		return true;
	}

	RN_INLINE bool Vector3::operator!= (const Vector3 &other) const
	{
		if(Math::FastAbs(x - other.x) <= k::EpsilonFloat && Math::FastAbs(y - other.y) <= k::EpsilonFloat && Math::FastAbs(z - other.z) <= k::EpsilonFloat)
			return false;

		return true;
	}

	RN_INLINE Vector3 Vector3::operator- () const
	{
		return Vector3(-x, -y, -z);
	}

	RN_INLINE Vector3 Vector3::operator+ (const Vector3 &other) const
	{
		return Vector3(x + other.x, y + other.y, z + other.z);
	}
	RN_INLINE Vector3 Vector3::operator- (const Vector3 &other) const
	{
		return Vector3(x - other.x, y - other.y, z - other.z);
	}
	RN_INLINE Vector3 Vector3::operator* (const Vector3 &other) const
	{
		return Vector3(x * other.x, y * other.y, z * other.z);
	}
	RN_INLINE Vector3 Vector3::operator/ (const Vector3 &other) const
	{
		return Vector3(x / other.x, y / other.y, z / other.z);
	}
	RN_INLINE Vector3 Vector3::operator* (const float n) const
	{
		return Vector3(x * n, y * n, z * n);
	}
	RN_INLINE Vector3 Vector3::operator/ (const float n) const
	{
		return Vector3(x / n, y / n, z / n);
	}

	RN_INLINE Vector3 &Vector3::operator+= (const Vector3 &other)
	{
		x += other.x;
		y += other.y;
		z += other.z;

		return *this;
	}
	RN_INLINE Vector3 &Vector3::operator-= (const Vector3 &other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;

		return *this;
	}
	RN_INLINE Vector3 &Vector3::operator*= (const Vector3 &other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;

		return *this;
	}
	RN_INLINE Vector3 &Vector3::operator/= (const Vector3 &other)
	{
		x /= other.x;
		y /= other.y;
		z /= other.z;

		return *this;
	}

	RN_INLINE float Vector3::GetLength() const
	{
		return Math::Sqrt(x * x + y * y + z * z);
	}
	
	RN_INLINE float Vector3::GetMax() const
	{
		return std::max(std::max(x, y), z);
	}
	
	RN_INLINE float Vector3::GetMin() const
	{
		return std::min(std::min(x, y), z);
	}

	RN_INLINE float Vector3::GetDotProduct(const Vector3 &other) const
	{
		return (x * other.x + y * other.y + z * other.z);
	}

	RN_INLINE Vector3 Vector3::GetCrossProduct(const Vector3 &other) const
	{
		Vector3 result;

		result.x = y * other.z - z * other.y;
		result.y = z * other.x - x * other.z;
		result.z = x * other.y - y * other.x;

		return result;
	}

	RN_INLINE bool Vector3::IsEqual(const Vector3 &other, float epsilon) const
	{
		if(Math::FastAbs(x - other.x) > epsilon)
			return false;

		if(Math::FastAbs(y - other.y) > epsilon)
			return false;

		if(Math::FastAbs(z - other.z) > epsilon)
			return false;

		return true;
	}

	RN_INLINE Vector3 &Vector3::Normalize(const float n)
	{
		if(x*x+y*y+z*z > k::EpsilonFloat)
		{
			float invlength = n*Math::InverseSqrt(x*x+y*y+z*z);
			x *= invlength;
			y *= invlength;
			z *= invlength;
		}

		return *this;
	}
	
	RN_INLINE Vector3 Vector3::GetNormalized(const float n) const
	{
		return Vector3(*this).Normalize(n);
	}

	RN_INLINE float Vector3::GetDistance(const Vector3 &other) const
	{
		Vector3 difference = *this - other;
		return difference.GetLength();
	}
	
	RN_INLINE float Vector3::GetSquaredDistance(const Vector3 &other) const
	{
		Vector3 difference = *this - other;
		return difference.GetDotProduct(difference);
	}

	RN_INLINE float Vector3::GetDistanceToSegment(const Vector3 &a, const Vector3 &b) const
	{
		Vector3 ab  = b - a;
		Vector3 av  = *this - a;

		if(av.GetDotProduct(ab) <= 0.0f) return av.GetLength();

		Vector3 bv = *this - b;
		if(bv.GetDotProduct(ab) >= 0.0) return bv.GetLength();

		return (ab.GetCrossProduct(av)).GetLength() / ab.GetLength();
	}

	RN_INLINE Vector3 Vector3::GetLerp(const Vector3 &other, float factor) const
	{
		return *this*(1.0f-factor)+other*factor;
	}

	RN_INLINE bool Vector3::IsValid() const
	{
		if(!std::isfinite(x))
			return false;

		if(!std::isfinite(y))
			return false;
		
		if(!std::isfinite(z))
			return false;

		return true;
	}




	RN_INLINE Vector4::Vector4()
	{
		x = y = z = w = 0.0f;
	}

	RN_INLINE Vector4::Vector4(const float n)
	{
		x = y = z = w = n;
	}

	RN_INLINE Vector4::Vector4(const float _x, const float _y, const float _z, const float _w)
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}
	
	RN_INLINE Vector4::Vector4(const Vector2 &other, float _z, float _w)
	{
		x = other.x;
		y = other.y;
		z = _z;
		w = _w;
	}
	
	RN_INLINE Vector4::Vector4(const Vector3 &other, float _w)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		w = _w;
	}

	RN_INLINE bool Vector4::operator== (const Vector4 &other) const
	{
		if(Math::FastAbs(x - other.x) > k::EpsilonFloat)
			return false;

		if(Math::FastAbs(y - other.y) > k::EpsilonFloat)
			return false;

		if(Math::FastAbs(z - other.z) > k::EpsilonFloat)
			return false;

		if(Math::FastAbs(w - other.w) > k::EpsilonFloat)
			return false;

		return true;
	}

	RN_INLINE bool Vector4::operator!= (const Vector4 &other) const
	{
		if(Math::FastAbs(x - other.x) <= k::EpsilonFloat && Math::FastAbs(y - other.y) <= k::EpsilonFloat && Math::FastAbs(z - other.z) <= k::EpsilonFloat && Math::FastAbs(w - other.w) <= k::EpsilonFloat)
			return false;

		return true;
	}

	RN_INLINE Vector4 Vector4::operator- () const
	{
		return Vector4(-x, -y, -z, -w);
	}

	RN_INLINE Vector4 Vector4::operator+ (const Vector4 &other) const
	{
		return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
	}
	RN_INLINE Vector4 Vector4::operator- (const Vector4 &other) const
	{
		return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
	}
	RN_INLINE Vector4 Vector4::operator* (const Vector4 &other) const
	{
		return Vector4(x * other.x, y * other.y, z * other.z, w * other.w);
	}
	RN_INLINE Vector4 Vector4::operator/ (const Vector4 &other) const
	{
		return Vector4(x / other.x, y / other.y, z / other.z, w / other.w);
	}
	
	RN_INLINE Vector4 Vector4::operator* (const float n) const
	{
		return Vector4(x * n, y * n, z * n, w * n);
	}
	RN_INLINE Vector4 Vector4::operator/ (const float n) const
	{
		return Vector4(x / n, y / n, z / n, w / n);
	}

	RN_INLINE Vector4 &Vector4::operator+= (const Vector4 &other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
		
		return *this;
	}

	RN_INLINE Vector4 &Vector4::operator-= (const Vector4 &other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		w -= other.w;

		return *this;
	}

	RN_INLINE Vector4 &Vector4::operator*= (const Vector4 &other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		w *= other.w;

		return *this;
	}

	RN_INLINE Vector4 &Vector4::operator/= (const Vector4 &other)
	{
		x /= other.x;
		y /= other.y;
		z /= other.z;
		w /= other.w;

		return *this;
	}

	RN_INLINE float Vector4::GetLength() const
	{
		return Math::Sqrt(x * x + y * y + z * z + w * w);
	}
	
	RN_INLINE float Vector4::GetMax() const
	{
		return std::max(std::max(std::max(x, y), z), w);
	}
	
	RN_INLINE float Vector4::GetMin() const
	{
		return std::min(std::min(std::min(x, y), z), w);
	}

	RN_INLINE float Vector4::GetDotProduct(const Vector4 &other) const
	{
		return (x * other.x + y * other.y + z * other.z + w * other.w);
	}

	RN_INLINE bool Vector4::IsEqual(const Vector4 &other, float epsilon) const
	{
		if(Math::FastAbs(x - other.x) > epsilon)
			return false;

		if(Math::FastAbs(y - other.y) > epsilon)
			return false;

		if(Math::FastAbs(z - other.z) > epsilon)
			return false;

		if(Math::FastAbs(w - other.w) > epsilon)
			return false;

		return true;
	}

	RN_INLINE bool Vector4::IsValid() const
	{
		if(!std::isfinite(x))
			return false;

		if(!std::isfinite(y))
			return false;
		
		if(!std::isfinite(z))
			return false;
		
		if(!std::isfinite(w))
			return false;

		return true;
	}

	RN_INLINE Vector4 &Vector4::Normalize(const float n)
	{
		if(x*x+y*y+z*z+w*w > k::EpsilonFloat)
		{
			float invlength = n*Math::InverseSqrt(x*x+y*y+z*z+w*w);
			x *= invlength;
			y *= invlength;
			z *= invlength;
			w *= invlength;
		}

		return *this;
	}
	
	RN_INLINE Vector4 Vector4::GetNormalized(const float n) const
	{
		return Vector4(*this).Normalize();
	}
	
	RN_INLINE float Vector4::GetDistance(const Vector4 &other) const
	{
		Vector4 difference = *this - other;
		return difference.GetLength();
	}
	
	RN_INLINE float Vector4::GetSquaredDistance(const Vector4 &other) const
	{
		Vector4 difference = *this - other;
		return difference.GetDotProduct(difference);
	}
	
	RN_INLINE Vector4 Vector4::GetLerp(const Vector4 &other, float factor) const
	{
		return *this*(1.0f-factor)+other*factor;
	}

#if RN_SUPPORTS_TRIVIALLY_COPYABLE
	static_assert(std::is_trivially_copyable<Vector2>::value, "Vector2 must be trivially copyable");
	static_assert(std::is_trivially_copyable<Vector3>::value, "Vector3 must be trivially copyable");
	static_assert(std::is_trivially_copyable<Vector4>::value, "Vector4 must be trivially copyable");
#endif
}

#endif /* __RAYNE_VECTOR_H__ */
