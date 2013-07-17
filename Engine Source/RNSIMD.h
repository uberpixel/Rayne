//
//  RNSIMD.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SIMD_H__
#define __RAYNE_SIMD_H__

#define RN_SIMD 0

#if __SSE__
	#undef RN_SIMD
	#define RN_SIMD 1

	#include <x86intrin.h>

	#ifndef __SSE4_1__
		#warning "Please enable compiler support for SSE 4.1 intrinsics"
	#endif
	#ifndef __SSE3__
		#warning "Please enable compiler support for SSE 3 intrinsics"
	#endif
#endif

namespace RN
{
	namespace SIMD
	{
#if __SSE__
		typedef __m128  VecFloat;
		typedef __m128i VecInt8;
		typedef __m128i VecInt16;
		typedef __m128i VecInt32;
		typedef __m128i VecUint8;
		typedef __m128i VecUint16;
		typedef __m128i VecUint32;
#endif
		
		
		
		static inline VecFloat Zero()
		{
#if __SSE__
			return _mm_setzero_ps();
#endif
		}
		
		static inline VecFloat NegativeZero()
		{
#if __SSE__
			alignas(16) static const unsigned int negativeZero[4] = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
			return _mm_load_ps(reinterpret_cast<const float *>(negativeZero));
#endif
		}
		
		

		static inline VecFloat SmearX(const VecFloat& vec)
		{
#if __SSE__
			return (_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(0, 0, 0, 0)));
#endif
		}
		
		static inline VecFloat SmearY(const VecFloat& vec)
		{
#if __SSE__
			return (_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(1, 1, 1, 1)));
#endif
		}
		
		static inline VecFloat SmearZ(const VecFloat& vec)
		{
#if __SSE__
			return (_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(2, 2, 2, 2)));
#endif
		}
		
		static inline VecFloat SmearW(const VecFloat& vec)
		{
#if __SSE__
			return (_mm_shuffle_ps(vec, vec, _MM_SHUFFLE(3, 3, 3, 3)));
#endif
		}
		
		
		static inline VecFloat Select(const VecFloat& v1, const VecFloat& v2, const VecFloat& mask)
		{
#if __SSE__
			return _mm_or_ps(_mm_andnot_ps(mask, v1), _mm_and_ps(mask, v2));
#endif
		}
		
		
		static inline VecFloat Cmplt(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_cmplt_ps(v1, v2);
#endif
		}
		
		static inline VecFloat Cmpgt(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_cmpgt_ps(v1, v2);
#endif
		}
		
		static inline VecFloat Cmpeq(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_cmpeq_ps(v1, v2);
#endif
		}
		
		
		static inline bool CmpltScalar(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_comilt_ss(v1, v2);
#endif
		}
		
		static inline bool CmpgtSclar(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_comigt_ss(v1, v2);
#endif
		}
		
		static inline bool CmpeqScalar(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_comieq_ss(v1, v2);
#endif
		}
		
		
		static inline VecFloat And(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_and_ps(v1, v2);
#endif
		}
		
		static inline VecFloat AndNot(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_andnot_ps(v2, v1);
#endif
		}
		
		static inline VecFloat Or(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_or_ps(v1, v2);
#endif
		}
		
		static inline VecFloat Xor(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_xor_ps(v1, v2);
#endif
		}
		
		
		static inline VecFloat Add(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_add_ps(v1, v2);
#endif
		}
		
		static inline VecFloat Hadd(const VecFloat& v1, const VecFloat& v2)
		{
#ifdef __SSE3__
			return _mm_hadd_ps(v1, v2);
#endif
		}
		
		
		static inline VecFloat AddScalar(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_add_ss(v1, v2);
#endif
		}
		
		
		static inline VecFloat Sub(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_sub_ps(v1, v2);
#endif
		}
		
		static inline VecFloat SubScalar(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_sub_ss(v1, v2);
#endif
		}
		
		static inline VecFloat Mul(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_mul_ps(v1, v2);
#endif
		}
		
		static inline VecFloat MulScalar(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_mul_ss(v1, v2);	
#endif
		}
		
		static inline VecFloat Div(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_div_ps(v1, v2);
#endif
		}
		
		static inline VecFloat DivScalar(const VecFloat& v1, const VecFloat& v2)
		{
#if __SSE__
			return _mm_div_ss(v1, v2);
#endif
		}
		
		
		inline VecFloat Madd(const VecFloat& v1, const VecFloat& v2, const VecFloat& v3)
		{
#if __SSE__
			return _mm_add_ps(_mm_mul_ps(v1, v2), v3);
#endif
		}
		
		inline VecFloat MaddScalar(const VecFloat& v1, const VecFloat& v2, const VecFloat& v3)
		{
#if __SSE__
			return _mm_add_ss(_mm_mul_ss(v1, v2), v3);
#endif
		}
		
		inline VecFloat Nmsub(const VecFloat& v1, const VecFloat& v2, const VecFloat& v3)
		{
#if __SSE__
			return _mm_sub_ps(v3, _mm_mul_ps(v1, v2));
#endif
		}
		
		inline VecFloat NmsubScalar(const VecFloat& v1, const VecFloat& v2, const VecFloat& v3)
		{
#if __SSE__
			return _mm_sub_ss(v3, _mm_mul_ss(v1, v2));	
#endif
		}
		
		
		
		template <unsigned int value>
		static inline VecFloat LoadConstant()
		{
#if __SSE__
			alignas(16) static const unsigned int values[4] = {value, value, value, value};
			return _mm_load_ps(reinterpret_cast<const float *>(values));
#endif
		}
		
		static inline VecFloat Load(const float *values)
		{
#if __SSE__
			return _mm_load_ps(values);
#endif
		}
		
		static inline VecFloat LoadUnaligned(const float *values)
		{
#if __SSE__
			return _mm_loadu_ps(values);
#endif
		}
		
		static inline VecFloat LoadScalar(const float *value)
		{
#if __SSE__
			return _mm_load_ss(value);
#endif
		}
		
		static inline VecFloat Set(float value)
		{
#if __SSE__
			return _mm_set1_ps(value);
#endif
		}
		
		
		static inline VecFloat PositiveFloor(const VecFloat& v)
		{
#if __SSE__
			register const VecFloat one = LoadConstant<0x3F800000>();
			register const VecFloat two23 = LoadConstant<0x4B000000>();
			
			VecFloat result = _mm_sub_ps(_mm_add_ps(v, two23), two23);
			return _mm_sub_ps(result, _mm_and_ps(one, _mm_cmplt_ps(v, result)));
#endif
		}
		
		static inline VecFloat PositiveFloorScalar(const VecFloat& v)
		{
#if __SSE__
			register const VecFloat one = LoadConstant<0x3F800000>();
			register const VecFloat two23 = LoadConstant<0x4B000000>();
			
			VecFloat result = _mm_sub_ss(_mm_add_ss(v, two23), two23);
			return _mm_sub_ss(result, _mm_and_ps(one, _mm_cmplt_ps(v, result)));
#endif
		}
		
		static inline int TruncateConvert(const VecFloat& v)
		{
#if __SSE__
			return _mm_cvtt_ss2si(v);
#endif
		}
		
		
		inline VecFloat Negate(const VecFloat& v)
		{
#if __SSE__
			return _mm_sub_ps(_mm_setzero_ps(), v);
#endif
		}
		
		
		static inline void Store(const VecFloat& vec, float *dest)
		{
#if __SSE__
			_mm_store_ps(dest, vec);
#endif
		}
		
		static inline void StoreUnaligned(const VecFloat& vec, float *dest)
		{
#if __SSE__
			_mm_store_ss(&dest[0], vec);
			_mm_store_ss(&dest[1], SmearY(vec));
			_mm_store_ss(&dest[2], SmearZ(vec));
			_mm_store_ss(&dest[3], SmearW(vec));
#endif
		}
		
		static inline void StoreX(const VecFloat& vec, float *dest)
		{
#if __SSE__
			_mm_store_ss(dest, vec);
#endif
		}
		
		static inline void StoreY(const VecFloat& vec, float *dest)
		{
#if __SSE__
			_mm_store_ss(dest, SmearY(vec));
#endif
		}
		
		static inline void StoreZ(const VecFloat& vec, float *dest)
		{
#if __SSE__
			_mm_store_ss(dest, SmearZ(vec));
#endif
		}
		
		static inline void StoreW(const VecFloat& vec, float *dest)
		{
#if __SSE__
			_mm_store_ss(dest, SmearW(vec));
#endif
		}
	}
}

#endif /* __RAYNE_SIMD_H__ */
