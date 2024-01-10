//
//  RNHalfVector.h
//  Rayne
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_HALFVECTOR_H__
#define __RAYNE_HALFVECTOR_H__

#include "../Base/RNBase.h"

namespace RN
{
	//These are used to convert from/to 16bit float vectors to use on the GPU, but most CPUs do not have native 16bit float support.
	class HalfVector2
	{
	public:
		HalfVector2()
		{
			x = 0;
			y = 0;
		}
		HalfVector2(const float x_, const float y_)
		{
			x = Math::ConvertFloatToHalf(x_);
			y = Math::ConvertFloatToHalf(y_);
		}
		explicit HalfVector2(const Vector2 &other)
		{
			x = Math::ConvertFloatToHalf(other.x);
			y = Math::ConvertFloatToHalf(other.y);
		}
		
		HalfVector2(const HalfVector2 &other) = default;
		
		Vector2 GetVector2()
		{
			Vector2 result;
			result.x = Math::ConvertHalfToFloat(x);
			result.y = Math::ConvertHalfToFloat(y);
			return result;
		}

	private:
		struct
		{
			uint16 x;
			uint16 y;
		};
	};

	class HalfVector3
	{
	public:
		HalfVector3()
		{
			x = 0;
			y = 0;
			z = 0;
		}
		HalfVector3(const float x_, const float y_, const float z_)
		{
			x = Math::ConvertFloatToHalf(x_);
			y = Math::ConvertFloatToHalf(y_);
			z = Math::ConvertFloatToHalf(z_);
		}
		explicit HalfVector3(const Vector3 &other)
		{
			x = Math::ConvertFloatToHalf(other.x);
			y = Math::ConvertFloatToHalf(other.y);
			z = Math::ConvertFloatToHalf(other.z);
		}
		
		HalfVector3(const HalfVector3 &other) = default;
		
		Vector3 GetVector3()
		{
			Vector3 result;
			result.x = Math::ConvertHalfToFloat(x);
			result.y = Math::ConvertHalfToFloat(y);
			result.z = Math::ConvertHalfToFloat(z);
			return result;
		}

	private:
		struct
		{
			uint16 x;
			uint16 y;
			uint16 z;
		};
	};
		
	class HalfVector4
	{
	public:
		HalfVector4()
		{
			x = 0;
			y = 0;
			z = 0;
			w = 0;
		}
		HalfVector4(const float x_, const float y_, const float z_, const float w_)
		{
			x = Math::ConvertFloatToHalf(x_);
			y = Math::ConvertFloatToHalf(y_);
			z = Math::ConvertFloatToHalf(z_);
			w = Math::ConvertFloatToHalf(w_);
		}
		explicit HalfVector4(const Vector4 &other)
		{
			x = Math::ConvertFloatToHalf(other.x);
			y = Math::ConvertFloatToHalf(other.y);
			z = Math::ConvertFloatToHalf(other.z);
			w = Math::ConvertFloatToHalf(other.w);
		}
		
		HalfVector4(const HalfVector4 &other) = default;
		
		Vector4 GetVector4()
		{
			Vector4 result;
			result.x = Math::ConvertHalfToFloat(x);
			result.y = Math::ConvertHalfToFloat(y);
			result.z = Math::ConvertHalfToFloat(z);
			result.w = Math::ConvertHalfToFloat(w);
			return result;
		}

	private:
		struct
		{
			uint16 x;
			uint16 y;
			uint16 z;
			uint16 w;
		};
	};

#if RN_SUPPORTS_TRIVIALLY_COPYABLE
	static_assert(std::is_trivially_copyable<HalfVector2>::value, "HalfVector2 must be trivially copyable");
	static_assert(std::is_trivially_copyable<HalfVector3>::value, "HalfVector3 must be trivially copyable");
	static_assert(std::is_trivially_copyable<HalfVector4>::value, "HalfVector4 must be trivially copyable");
#endif
}

#endif /* __RAYNE_HALFVECTOR_H__ */
