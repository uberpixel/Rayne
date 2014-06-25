//
//  RNRandom.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
			RNAPI Generator();
			RNAPI virtual ~Generator();
			
			RNAPI virtual uint32 GetSeedValue();
			
			RNAPI virtual int32 GetMin() const = 0;
			RNAPI virtual int32 GetMax() const = 0;
			
			RNAPI virtual void Seed(uint32 seed) = 0;
			
			RNAPI virtual int32 GetRandomInt32() = 0;
			RNAPI virtual int32 GetRandomInt32Range(int32 min, int32 max);
			
			RNAPI virtual float GetRandomFloat();
			RNAPI virtual float GetRandomFloatRange(float min, float max);
			
			RNAPI virtual double GetUniformDeviate(int32 seed);
			
			RNAPI Color GetRandomColor();
			RNAPI Color GetRandomColorRange(const Color &min, const Color &max);
			
			RNAPI Vector2 GetRandomVector2Range(const Vector2 &min, const Vector2 &max);
			RNAPI Vector3 GetRandomVector3Range(const Vector3 &min, const Vector3 &max);
			RNAPI Vector4 GetRandomVector4Range(const Vector4 &min, const Vector4 &max);
		};
		
		class MersenneTwister : public Generator
		{
		public:
			RNAPI MersenneTwister();
			RNAPI ~MersenneTwister();
			
			RNAPI virtual int32 GetMin() const;
			RNAPI virtual int32 GetMax() const;
			
			RNAPI virtual void Seed(uint32 seed);
			RNAPI virtual int32 GetRandomInt32();
			
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
			MersenneTwister
		};
		
		RNAPI RandomNumberGenerator(Type type);
		RNAPI ~RandomNumberGenerator() override;
		
		RNAPI int32 GetMin() const;
		RNAPI int32 GetMax() const;
		
		RNAPI void Seed(uint32 seed);
		
		RNAPI int32 GetRandomInt32();
		RNAPI int32 GetRandomInt32Range(int32 min, int32 max);
		
		RNAPI float GetRandomFloat();
		RNAPI float GetRandomFloatRange(float min, float max);
		
		RNAPI double GetUniformDeviate(int32 seed);
		RNAPI Color GetRandomColor();
		RNAPI Color GetRandomColorRange(const Color &min, const Color &max);
		
		RNAPI Vector2 GetRandomVector2Range(const Vector2 &min, const Vector2 &max);
		RNAPI Vector3 GetRandomVector3Range(const Vector3 &min, const Vector3 &max);
		RNAPI Vector4 GetRandomVector4Range(const Vector4 &min, const Vector4 &max);
		
	private:
		Random::Generator *_generator;
		
		RNDeclareMeta(RandomNumberGenerator)
	};
	
	RNObjectClass(RandomNumberGenerator)
}

#endif /* __RAYNE_RANDOM_H__ */
