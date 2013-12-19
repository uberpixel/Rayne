//
//  RNRandom.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RANDOM_H__
#define __RAYNE_RANDOM_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNColor.h"
#include "RNVector.h"

namespace RN
{
	namespace Random
	{
		class Generator
		{
		public:
			Generator();
			virtual ~Generator();
			
			virtual uint32 GetSeedValue();
			
			virtual int32 GetMin() const = 0;
			virtual int32 GetMax() const = 0;
			
			virtual void Seed(uint32 seed) = 0;
			
			virtual int32 RandomInt32() = 0;
			virtual int32 RandomInt32Range(int32 min, int32 max);
			
			virtual float RandomFloat();
			virtual float RandomFloatRange(float min, float max);
			
			virtual double UniformDeviate(int32 seed);
		};
		
		class LCG : public Generator
		{
		public:
			LCG();
			
			virtual int32 GetMin() const;
			virtual int32 GetMax() const;
			
			virtual void Seed(uint32 seed);
			virtual int32 RandomInt32();
			
		private:
			int32 _M;
			int32 _A;
			int32 _Q;
			int32 _R;
			int32 _seed;
		};
		
		class DualPhaseLCG : public Generator
		{
		public:
			DualPhaseLCG();
			
			virtual int32 GetMin() const;
			virtual int32 GetMax() const;
			
			virtual void Seed(uint32 seed);
			virtual int32 RandomInt32();
			
		private:
			int32 _M1;
			int32 _M2;
			int32 _A1;
			int32 _A2;
			int32 _Q1;
			int32 _Q2;
			int32 _R1;
			int32 _R2;
			
			int32 _seed1;
			int32 _seed2;
		};
		
		class MersenneTwister : public Generator
		{
		public:
			MersenneTwister();
			~MersenneTwister();
			
			virtual int32 GetMin() const;
			virtual int32 GetMax() const;
			
			virtual void Seed(uint32 seed);
			virtual int32 RandomInt32();
			
		private:
			int32 _N;
			int32 _M;
			int32 _A;
			int32 _U;
			int32 _L;
			
			uint32 *_bytes;
			uint32 _offset;
		};
	}
	
	class RandomNumberGenerator : public Object
	{
	public:
		enum class Type
		{
			LCG,
			DualPhaseLCG,
			MersenneTwister
		};
		
		RandomNumberGenerator(Type type);
		~RandomNumberGenerator() override;
		
		int32 GetMin() const;
		int32 GetMax() const;
		
		void Seed(uint32 seed);
		int32 RandomInt32();
		int32 RandomInt32Range(int32 min, int32 max);
		
		float RandomFloat();
		float RandomFloatRange(float min, float max);
		
		double UniformDeviate(int32 seed);
		Color RandomColor();
		
		Vector2 RandomVector2Range(const Vector2& min, const Vector2& max);
		Vector3 RandomVector3Range(const Vector3& min, const Vector3& max);
		Vector4 RandomVector4Range(const Vector4& min, const Vector4& max);
		
	private:
		Random::Generator *_generator;
		
		RNDefineMeta(RandomNumberGenerator, Object)
	};
}

#endif /* __RAYNE_RANDOM_H__ */
