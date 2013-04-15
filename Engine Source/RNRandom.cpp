//
//  RNRandom.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRandom.h"
#include "RNCPU.h"

#define kRNSecureRNGSize 128

namespace RN
{
	namespace Random
	{
		// ---------------------
		// MARK: -
		// MARK: Generator
		// ---------------------
		
		Generator::Generator()
		{
		}
		Generator::~Generator()
		{
		}
		
		uint32 Generator::SeedValue()
		{
			time_t now = time(0);
			uint8 *p = reinterpret_cast<uint8 *>(&now);
			uint32 seed = 0;
			
			for(int i=0; i<sizeof(time_t); i++)
			{
				seed = seed * (UINT8_MAX + 2U) + p[i];
			}
			
			return seed;
		}
		
		int32 Generator::RandomInt32Range(int32 min, int32 max)
		{
			RN_ASSERT0(min >= Min());
			RN_ASSERT0(max <= Max());
			
			while(1)
			{
				int32 base = RandomInt32();
				
				if(base == Max())
					continue;
				
				int32 range = max - min;
				int32 remainder = Max() % range;
				int32 bucket    = Max() / range;
				
				if(base < Max() - remainder)
					return min + base / bucket;
			}
		}
		
		float Generator::RandomFloat()
		{
			return UniformDeviate(RandomInt32());
		}
		
		float Generator::RandomFloatRange(int32 min, int32 max)
		{
			float M = (float)MIN(min, max);
			float N = (float)MAX(min, max);
			
			return M + UniformDeviate(RandomInt32()) * (N - M);
		}
		
		double Generator::UniformDeviate(int32 seed)
		{
			return seed * (1.0 / (Max() + 1.0));
		}
		
		// ---------------------
		// MARK: -
		// MARK: LCG
		// ---------------------
		
		LCG::LCG()
		{
			_M = 2147483647;
			_A = 16807;
			
			_Q = (_M / _A);
			_R = (_M % _A);
			
			Seed(SeedValue());
		}
		
		
		int32 LCG::Min() const
		{
			return 0;
		}
		
		int32 LCG::Max() const
		{
			return INT32_MAX;
		}
		
		
		void LCG::Seed(uint32 seed)
		{
			_seed = static_cast<int32>(seed);
		}
		
		int32 LCG::RandomInt32()
		{
			_seed = _A * (_seed % _Q) - _R * (_seed / _Q);
			
			if(_seed <= 0)
				_seed += _M;
			
			return _seed;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Dual Phase LCG
		// ---------------------
		
		DualPhaseLCG::DualPhaseLCG()
		{
			_M1 = 2147483647;
			_M2 = 2147483399;
			_A1 = 40015;
			_A2 = 40692;
			
			_Q1 = (_M1 / _A1);
			_Q2 = (_M2 / _A2);
			_R1 = (_M1 % _A1);
			_R2 = (_M2 % _A2);
			
			Seed(SeedValue());
		}
		
		int32 DualPhaseLCG::Min() const
		{
			return 0;
		}
		
		int32 DualPhaseLCG::Max() const
		{
			return INT32_MAX;
		}
		
		void DualPhaseLCG::Seed(uint32 seed)
		{
			_seed1 = _seed2 = static_cast<int32>(seed);
		}
		
		int32 DualPhaseLCG::RandomInt32()
		{
			_seed1 = _A1 * (_seed1 % _Q1) - _R1 * (_seed1 / _Q1);
			_seed2 = _A2 * (_seed2 % _Q2) - _R2 * (_seed2 / _Q2);
			
			if(_seed1 <= 0)
				_seed1 += _M1;
			
			if(_seed2 <= 0)
				_seed2 += _M2;
			
			int32 result = _seed1 - _seed2;
			
			if(result <= 0)
				result += _M1 - 1;
			
			return result;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Mersenne Twister
		// ---------------------
		
		MersenneTwister::MersenneTwister()
		{
			_N = 624;
			_M = 397;
			_A = 0x9908b0dfUL;
			_U = 0x80000000UL;
			_L = 0x7fffffffUL;
			
			_bytes  = new uint32[_N];
			Seed(SeedValue());
		}
		
		MersenneTwister::~MersenneTwister()
		{
			delete [] _bytes;
		}
		
		int32 MersenneTwister::Min() const
		{
			return 0;
		}
		
		int32 MersenneTwister::Max() const
		{
			return INT32_MAX;
		}
		
		
		void MersenneTwister::Seed(uint32 seed)
		{
			_bytes[0] = seed & 0xffffffffUL;
			
			for(int i=1; i<_N; i++)
			{
				_bytes[i] = 1812433253UL * (_bytes[i - 1] ^ (_bytes[ - 1] >> 30)) + i;
				_bytes[i] &= 0xffffffffUL;
			}
		}
		
		int32 MersenneTwister::RandomInt32()
		{
			uint32 y, a;
			
			if(_offset >= _N)
			{
				_offset = 0;
				
				for(int i=0; i<_N - 1; i++)
				{
					y = (_bytes[i] & _U) | (_bytes[i + 1] & _L);
					a = (y & 0x1) ? _A : 0x0;
					
					_bytes[i] = _bytes[(i + _M) % _N] ^ (y >> 1) ^ a;
				}
				
				y = (_bytes[_N - 1] & _U) | (_bytes[0] & _L);
				a = (y & 0x1) ? _A : 0x0;
				
				_bytes[_N - 1] = _bytes[_M - 1] ^ (y >> 1) ^ a;
			}
			
			y = _bytes[_offset ++];
			y ^= (y >> 11);
			y ^= (y << 7) & 0x9d2c5680UL;
			y ^= (y << 15) & 0xefc60000UL;
			y ^= (y >> 18);
			
			return static_cast<int32>(y & (1 << 31) - 1);
		}
		
		// ---------------------
		// MARK: -
		// MARK: Secure
		// ---------------------
		
		Secure::Secure()
		{
			_offset = 0;
			_size   = 0;
			_bytes  = new uint32[kRNSecureRNGSize];
		}
		
		Secure::~Secure()
		{
			delete [] _bytes;
		}
		
		int Secure::ReadRDRAND()
		{
			int status;
			int32 result;
			
			__asm__ volatile("rdrand %%eax \n"
							 "movl $1, %%edx \n"
							 "cmovae %%eax, %%edx \n"
							 "movl %%edx, %1 \n"
							 "movl %%eax, %0" : "=r" (result), "=r" (status) :: "%eax", "%edx");
			if(status)
			{
				*(_bytes + _size) = result;
				_size ++;
			}
			
			return status;
		}
		
		void Secure::FillBuffer()
		{
			_offset = 0;
			_size   = 0;
			
			if((X86_64::Caps() & X86_64::CAP_RDRAND))
			{
				while(_size < kRNSecureRNGSize)
				{
					int result = ReadRDRAND();
					if(!result)
					{
						if(_size > 0)
							break;
						
						std::this_thread::sleep_for(std::chrono::nanoseconds(10));
					}
				}
			}
			else
			{
				FILE *file = fopen("/dev/random", "rb");
				fread(_bytes, kRNSecureRNGSize, 4, file);
				fclose(file);
				
				_size = kRNSecureRNGSize;
			}
		}
		
		
		int32 Secure::Min() const
		{
			return 0;
		}
		
		int32 Secure::Max() const
		{
			return INT32_MAX;
		}
		
		
		void Secure::Seed(uint32 seed)
		{
			throw ErrorException(0);
		}
		
		int32 Secure::RandomInt32()
		{
			if(_offset >= _size)
			{
				FillBuffer();
			}
			
			int32 result;
			std::copy(_bytes + _offset, _bytes + _offset + 1, &result);
			_offset ++;
			
			return result & (1 << 31) - 1;
		}
		
		bool Secure::UsesHardware()
		{
			return (X86_64::Caps() & X86_64::CAP_RDRAND);
		}
	}
}
