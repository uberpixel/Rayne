//
//  RNRandom.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RANDOM_H__
#define __RAYNE_RANDOM_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
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
			uint32 ___N;
			uint32 __M;
			uint32 __A;
			uint32 __U;
			uint32 __L;

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

		RNAPI static void InitialWakeUp(MetaClass *meta);
		RNAPI static RandomNumberGenerator *GetSharedGenerator();

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
		Lockable _lock;
		Random::Generator *_generator;

		__RNDeclareMetaInternal(RandomNumberGenerator)
	};

	RNAPI int32 GetRandomInt32();
	RNAPI int32 GetRandomInt32Range(int32 min, int32 max);

	RNAPI float GetRandomFloat();
	RNAPI float GetRandomFloatRange(float min, float max);

	RNAPI Color GetRandomColor();
	RNAPI Color GetRandomColorRange(const Color &min, const Color &max);

	RNAPI Vector2 GetRandomVector2Range(const Vector2 &min, const Vector2 &max);
	RNAPI Vector3 GetRandomVector3Range(const Vector3 &min, const Vector3 &max);
	RNAPI Vector4 GetRandomVector4Range(const Vector4 &min, const Vector4 &max);

	RNObjectClass(RandomNumberGenerator)
}

#endif /* __RAYNE_RANDOM_H__ */