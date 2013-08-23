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
		
		uint32 Generator::GetSeedValue()
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
			RN_ASSERT(min >= GetMin(), "");
			RN_ASSERT(max <= GetMax(), "");
			
			while(1)
			{
				int32 base = RandomInt32();
				
				if(base == GetMax())
					continue;
				
				int32 range = max - min;
				int32 remainder = GetMax() % range;
				int32 bucket    = GetMax() / range;
				
				if(base < GetMax() - remainder)
					return min + base / bucket;
			}
		}
		
		float Generator::RandomFloat()
		{
			return UniformDeviate(RandomInt32());
		}
		
		float Generator::RandomFloatRange(float min, float max)
		{
			float range = max - min;
			return (UniformDeviate(RandomInt32()) * range) + min;
		}
		
		double Generator::UniformDeviate(int32 seed)
		{
			return seed * (1.0 / (GetMax() + 1.0));
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
			
			Seed(GetSeedValue());
		}
		
		
		int32 LCG::GetMin() const
		{
			return 0;
		}
		
		int32 LCG::GetMax() const
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
			
			Seed(GetSeedValue());
		}
		
		int32 DualPhaseLCG::GetMin() const
		{
			return 0;
		}
		
		int32 DualPhaseLCG::GetMax() const
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
			Seed(GetSeedValue());
		}
		
		MersenneTwister::~MersenneTwister()
		{
			delete [] _bytes;
		}
		
		int32 MersenneTwister::GetMin() const
		{
			return 0;
		}
		
		int32 MersenneTwister::GetMax() const
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
	}
	
	RNDeclareMeta(RandomNumberGenerator)
	
	RandomNumberGenerator::RandomNumberGenerator(Type type)
	{
		switch(type)
		{
			case Type::LCG:
				_generator = new Random::LCG();
				break;
				
			case Type::DualPhaseLCG:
				_generator = new Random::DualPhaseLCG();
				break;
				
			case Type::MersenneTwister:
				_generator = new Random::MersenneTwister();
				break;
		}
	}
	
	RandomNumberGenerator::~RandomNumberGenerator()
	{
		delete _generator;
	}
	
	
	int32 RandomNumberGenerator::GetMin() const
	{
		return _generator->GetMin();
	}
	
	int32 RandomNumberGenerator::GetMax() const
	{
		return _generator->GetMax();
	}
	
	void RandomNumberGenerator::Seed(uint32 seed)
	{
		_generator->Seed(seed);
	}
	
	int32 RandomNumberGenerator::RandomInt32()
	{
		return _generator->RandomInt32();
	}
	
	int32 RandomNumberGenerator::RandomInt32Range(int32 min, int32 max)
	{
		return _generator->RandomInt32Range(min, max);
	}
	
	float RandomNumberGenerator::RandomFloat()
	{
		return _generator->RandomFloat();
	}
	
	float RandomNumberGenerator::RandomFloatRange(float min, float max)
	{
		return _generator->RandomFloatRange(min, max);
	}
	
	double RandomNumberGenerator::UniformDeviate(int32 seed)
	{
		return _generator->UniformDeviate(seed);
	}
	
	Color RandomNumberGenerator::RandomColor()
	{
		Color result;
		
		result.r = _generator->UniformDeviate(_generator->RandomInt32());
		result.g = _generator->UniformDeviate(_generator->RandomInt32());
		result.b = _generator->UniformDeviate(_generator->RandomInt32());
		result.a = _generator->UniformDeviate(_generator->RandomInt32());
		
		return result;
	}
	
	Vector2 RandomNumberGenerator::RandomVector2Range(const Vector2& min, const Vector2& max)
	{
		Vector2 result;
		
		result.x = _generator->RandomFloatRange(MIN(min.x, max.x), MAX(min.x, max.x));
		result.y = _generator->RandomFloatRange(MIN(min.y, max.y), MAX(min.y, max.y));
		
		return result;
	}
	
	Vector3 RandomNumberGenerator::RandomVector3Range(const Vector3& min, const Vector3& max)
	{
		Vector3 result;
		
		result.x = _generator->RandomFloatRange(MIN(min.x, max.x), MAX(min.x, max.x));
		result.y = _generator->RandomFloatRange(MIN(min.y, max.y), MAX(min.y, max.y));
		result.z = _generator->RandomFloatRange(MIN(min.z, max.z), MAX(min.z, max.z));
		
		return result;
	}
	
	Vector4 RandomNumberGenerator::RandomVector4Range(const Vector4& min, const Vector4& max)
	{
		Vector4 result;
		
		result.x = _generator->RandomFloatRange(MIN(min.x, max.x), MAX(min.x, max.x));
		result.y = _generator->RandomFloatRange(MIN(min.y, max.y), MAX(min.y, max.y));
		result.z = _generator->RandomFloatRange(MIN(min.z, max.z), MAX(min.z, max.z));
		result.w = _generator->RandomFloatRange(MIN(min.w, max.w), MAX(min.w, max.w));
		
		return result;
	}
}
