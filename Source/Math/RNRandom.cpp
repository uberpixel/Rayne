//
//  RNRandom.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <random>
#include "RNRandom.h"
#include "RNMath.h"

namespace RN
{
	namespace Random
	{
		// ---------------------
		// MARK: -
		// MARK: Color
		// ---------------------

		struct ColorInfo
		{
			ColorInfo(const std::string &tname, const std::pair<float, float> &thueRange, const std::vector<std::pair<float, float>> &tlowerBounds) :
				name(tname),
				hueRange(thueRange),
				lowerBounds(tlowerBounds)
			{
				float sMin = lowerBounds.front().first;
				float sMax = lowerBounds.back().first;

				float bMin = lowerBounds.back().second;
				float bMax = lowerBounds.front().second;

				saturationRange = std::make_pair(sMin, sMax);
				brightnessRange = std::make_pair(bMin, bMax);
			}

			std::string name;
			std::pair<float, float> hueRange;
			std::pair<float, float> saturationRange;
			std::pair<float, float> brightnessRange;
			std::vector<std::pair<float, float>> lowerBounds;
		};

		static std::vector<ColorInfo> __ColorDefinitions;

		void PopulateColorTable()
		{
			static std::once_flag token;
			std::call_once(token, [&] {

				{
					std::vector<std::pair<float, float>> pair = {std::make_pair(20.0f, 100.0f), std::make_pair(30.0f, 92.0f),
																 std::make_pair(40.0f, 89.0f), std::make_pair(50.0f, 85.0f),
																 std::make_pair(60.0f, 78.0f), std::make_pair(70.0f, 70.0f),
																 std::make_pair(80.0f, 60.0f), std::make_pair(90.0f, 55.0f),
																 std::make_pair(100.0f, 50.0f)};
					__ColorDefinitions.emplace_back("red", std::make_pair(-26.0f, 18.0f), pair);
				}
				{
					std::vector<std::pair<float, float>> pair = {std::make_pair(20.0f, 100.0f), std::make_pair(30.0f, 93.0f),
																 std::make_pair(40.0f, 88.0f), std::make_pair(50.0f, 86.0f),
																 std::make_pair(60.0f, 85.0f), std::make_pair(70.0f, 70.0f),
																 std::make_pair(100.0f, 70.0f)};
					__ColorDefinitions.emplace_back("orange", std::make_pair(19.0f, 46.0f), pair);
				}
				{
					std::vector<std::pair<float, float>> pair = {std::make_pair(25.0f, 100.0f), std::make_pair(40.0f, 94.0f),
																 std::make_pair(50.0f, 89.0f), std::make_pair(60.0f, 86.0f),
																 std::make_pair(70.0f, 84.0f), std::make_pair(80.0f, 82.0f),
																 std::make_pair(90.0f, 80.0f), std::make_pair(100.0f, 75.0f)};
					__ColorDefinitions.emplace_back("yellow", std::make_pair(47.0f, 62.0f), pair);
				}
				{
					std::vector<std::pair<float, float>> pair = {std::make_pair(30.0f, 100.0f), std::make_pair(40.0f, 90.0f),
																 std::make_pair(50.0f, 85.0f), std::make_pair(60.0f, 81.0f),
																 std::make_pair(70.0f, 74.0f), std::make_pair(80.0f, 64.0f),
																 std::make_pair(90.0f, 50.0f), std::make_pair(100.0f, 40.0f)};
					__ColorDefinitions.emplace_back("green", std::make_pair(63.0f, 178.0f), pair);
				}
				{
					std::vector<std::pair<float, float>> pair = {std::make_pair(20.0f, 100.0f), std::make_pair(30.0f, 86.0f),
																 std::make_pair(40.0f, 80.0f), std::make_pair(50.0f, 74.0f),
																 std::make_pair(60.0f, 60.0f), std::make_pair(70.0f, 52.0f),
																 std::make_pair(80.0f, 44.0f), std::make_pair(90.0f, 39.0f),
																 std::make_pair(100.0f, 35.0f)};
					__ColorDefinitions.emplace_back("blue", std::make_pair(179.0f, 257.0f), pair);
				}
				{
					std::vector<std::pair<float, float>> pair = {std::make_pair(20.0f, 100.0f), std::make_pair(30.0f, 87.0f),
																 std::make_pair(40.0f, 79.0f), std::make_pair(50.0f, 70.0f),
																 std::make_pair(60.0f, 65.0f), std::make_pair(70.0f, 59.0f),
																 std::make_pair(80.0f, 52.0f), std::make_pair(90.0f, 45.0f),
																 std::make_pair(100.0f, 42.0f)};
					__ColorDefinitions.emplace_back("purple", std::make_pair(258.0f, 282.0f), pair);
				}
				{
					std::vector<std::pair<float, float>> pair = {std::make_pair(20.0f, 100.0f), std::make_pair(30.0f, 90.0f),
																 std::make_pair(40.0f, 86.0f), std::make_pair(60.0f, 84.0f),
																 std::make_pair(80.0f, 80.0f), std::make_pair(90.0f, 75.0f),
																 std::make_pair(100.0f, 73.0f)};
					__ColorDefinitions.emplace_back("pink", std::make_pair(283.0f, 334.0f), pair);
				}
			});
		}

		const ColorInfo &GetColorInfo(float hue)
		{
			PopulateColorTable();

			if(hue >= 334.0f && hue <= 360.0f)
				hue -= 360.0f;

			for(const ColorInfo &info : __ColorDefinitions)
			{
				if(hue >= info.hueRange.first && hue <= info.hueRange.second)
					return info;
			}

			throw "Couldn't find info for color"; // Should never ever happen
		}

		float GetMinimumBrightness(float hue, float saturation)
		{
			const ColorInfo &info = GetColorInfo(hue);

			size_t count = info.lowerBounds.size() - 1;

			for(size_t i = 0; i < count; i++)
			{
				float s1 = info.lowerBounds[i].first;
				float v1 = info.lowerBounds[i].second;

				float s2 = info.lowerBounds[i + 1].first;
				float v2 = info.lowerBounds[i + 1].second;

				if(saturation >= s1 && saturation <= s2)
				{
					float m = (v2 - v1) / (s2 - s1);
					float b = v1 - m * s1;

					return m * saturation + b;
				}
			}

			return 0.0f;
		}

		float PickSaturationForGenerator(Generator *generator, float hue)
		{
			const ColorInfo &info = GetColorInfo(hue);

			float min = info.saturationRange.first;
			float max = info.saturationRange.second;

			return generator->GetRandomFloatRange(min, max);
		}

		float PickBrightnessForGenerator(Generator *generator, float hue, float saturation)
		{
			float min = GetMinimumBrightness(hue, saturation);
			float max = 100.0f;

			return generator->GetRandomFloatRange(min, max);
		}

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
			uint32 seed = static_cast<uint32>(std::random_device{}());
			return seed;
		}

		int32 Generator::GetRandomInt32Range(int32 min, int32 max)
		{
			RN_ASSERT(min >= GetMin(), "");
			RN_ASSERT(max <= GetMax(), "");

			while(1)
			{
				int32 base = GetRandomInt32();

				if(base == GetMax())
					continue;

				int32 range = max - min;
				int32 remainder = GetMax() % range;
				int32 bucket = GetMax() / range;

				if(base < GetMax() - remainder)
					return min + base / bucket;
			}
		}

		float Generator::GetRandomFloat()
		{
			return GetUniformDeviate(GetRandomInt32());
		}

		float Generator::GetRandomFloatRange(float min, float max)
		{
			float range = max - min;
			return (GetUniformDeviate(GetRandomInt32()) * range) + min;
		}

		double Generator::GetUniformDeviate(int32 seed)
		{
			return seed * (1.0 / (GetMax() + 1.0));
		}

		Color Generator::GetRandomColor()
		{
			float hue = roundf(GetRandomFloatRange(0.0f, 360.0f));
			float saturation = roundf(PickSaturationForGenerator(this, hue));
			float brightness = roundf(PickBrightnessForGenerator(this, hue, saturation));

			hue = hue / 360.0f;
			hue = (hue * (k::Pi * 2)) - k::Pi;

			return Color::WithHSV(hue, (saturation / 100.0f), (brightness / 100.0f));
		}

		Vector2 Generator::GetRandomVector2Range(const Vector2 &min, const Vector2 &max)
		{
			Vector2 result;

			result.x = GetRandomFloatRange(std::min(min.x, max.x), std::max(min.x, max.x));
			result.y = GetRandomFloatRange(std::min(min.y, max.y), std::max(min.y, max.y));

			return result;
		}

		Vector3 Generator::GetRandomVector3Range(const Vector3 &min, const Vector3 &max)
		{
			Vector3 result;

			result.x = GetRandomFloatRange(std::min(min.x, max.x), std::max(min.x, max.x));
			result.y = GetRandomFloatRange(std::min(min.y, max.y), std::max(min.y, max.y));
			result.z = GetRandomFloatRange(std::min(min.z, max.z), std::max(min.z, max.z));

			return result;
		}

		Vector4 Generator::GetRandomVector4Range(const Vector4 &min, const Vector4 &max)
		{
			Vector4 result;

			result.x = GetRandomFloatRange(std::min(min.x, max.x), std::max(min.x, max.x));
			result.y = GetRandomFloatRange(std::min(min.y, max.y), std::max(min.y, max.y));
			result.z = GetRandomFloatRange(std::min(min.z, max.z), std::max(min.z, max.z));
			result.w = GetRandomFloatRange(std::min(min.w, max.w), std::max(min.w, max.w));

			return result;
		}

		Color Generator::GetRandomColorRange(const Color &min, const Color &max)
		{
			Color result;

			result.r = GetRandomFloatRange(std::min(min.r, max.r), std::max(min.r, max.r));
			result.g = GetRandomFloatRange(std::min(min.g, max.g), std::max(min.g, max.g));
			result.b = GetRandomFloatRange(std::min(min.b, max.b), std::max(min.b, max.b));
			result.a = GetRandomFloatRange(std::min(min.a, max.a), std::max(min.a, max.a));

			return result;
		}

		// ---------------------
		// MARK: -
		// MARK: Mersenne Twister
		// ---------------------

		MersenneTwister::MersenneTwister() :
			___N(624),
			__M(397),
			__A(0x9908b0dfUL),
			__U(0x80000000UL),
			__L(0x7fffffffUL)
		{
			_bytes = new uint32[___N];
			Seed(GetSeedValue());
		}

		MersenneTwister::~MersenneTwister()
		{
			delete[] _bytes;
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
			_offset = 0;
			_bytes[0] = seed & 0xffffffffUL;

			for(uint32 i = 1; i < ___N; i++)
			{
				_bytes[i] = 1812433253UL * (_bytes[i - 1] ^ (_bytes[i - 1] >> 30)) + i;
				_bytes[i] &= 0xffffffffUL;
			}
		}

		int32 MersenneTwister::GetRandomInt32()
		{
			uint32 y, a;

			if(_offset >= ___N)
			{
				_offset = 0;

				for(uint32 i = 0; i < ___N - 1; i++)
				{
					y = (_bytes[i] & __U) | (_bytes[i + 1] & __L);
					a = (y & 0x1) ? __A : 0x0;

					_bytes[i] = _bytes[(i + __M) % ___N] ^ (y >> 1) ^ a;
				}

				y = (_bytes[___N - 1] & __U) | (_bytes[0] & __L);
				a = (y & 0x1) ? __A : 0x0;

				_bytes[___N - 1] = _bytes[__M - 1] ^ (y >> 1) ^ a;
			}

			y = _bytes[_offset++];
			y ^= (y >> 11);
			y ^= (y << 7) & 0x9d2c5680UL;
			y ^= (y << 15) & 0xefc60000UL;
			y ^= (y >> 18);

#if RN_PLATFORM_WINDOWS
#pragma warning(disable: 4307)
#endif

			return static_cast<int32>(y & ((1UL << 31) - 1));
		}
	}

	// ---------------------
	// MARK: -
	// MARK: RandomNumberGenerator
	// ---------------------

	RNDefineMeta(RandomNumberGenerator, Object)

	static RandomNumberGenerator *__sharedGenerator = nullptr;

	void RandomNumberGenerator::InitialWakeUp(MetaClass *meta)
	{
		if(meta == RandomNumberGenerator::GetMetaClass())
		{
			__sharedGenerator = new RandomNumberGenerator(RandomNumberGenerator::Type::MersenneTwister);
		}
	}

	RandomNumberGenerator *RandomNumberGenerator::GetSharedGenerator()
	{
		return __sharedGenerator;
	}


	RandomNumberGenerator::RandomNumberGenerator(Type type)
	{
		switch(type)
		{
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
		LockGuard<Lockable> lock(_lock);
		_generator->Seed(seed);
	}

	int32 RandomNumberGenerator::GetRandomInt32()
	{
		LockGuard<Lockable> lock(_lock);
		return _generator->GetRandomInt32();
	}

	int32 RandomNumberGenerator::GetRandomInt32Range(int32 min, int32 max)
	{
		LockGuard<Lockable> lock(_lock);
		return _generator->GetRandomInt32Range(min, max);
	}

	float RandomNumberGenerator::GetRandomFloat()
	{
		LockGuard<Lockable> lock(_lock);
		return _generator->GetRandomFloat();
	}

	float RandomNumberGenerator::GetRandomFloatRange(float min, float max)
	{
		LockGuard<Lockable> lock(_lock);
		return _generator->GetRandomFloatRange(min, max);
	}

	double RandomNumberGenerator::GetUniformDeviate(int32 seed)
	{
		LockGuard<Lockable> lock(_lock);
		return _generator->GetUniformDeviate(seed);
	}

	Vector2 RandomNumberGenerator::GetRandomVector2Range(const Vector2 &min, const Vector2 &max)
	{
		LockGuard<Lockable> lock(_lock);
		return _generator->GetRandomVector2Range(min, max);
	}

	Vector3 RandomNumberGenerator::GetRandomVector3Range(const Vector3 &min, const Vector3 &max)
	{
		LockGuard<Lockable> lock(_lock);
		return _generator->GetRandomVector3Range(min, max);
	}

	Vector4 RandomNumberGenerator::GetRandomVector4Range(const Vector4 &min, const Vector4 &max)
	{
		LockGuard<Lockable> lock(_lock);
		return _generator->GetRandomVector4Range(min, max);
	}

	Color RandomNumberGenerator::GetRandomColor()
	{
		LockGuard<Lockable> lock(_lock);
		return _generator->GetRandomColor();
	}

	Color RandomNumberGenerator::GetRandomColorRange(const Color &min, const Color &max)
	{
		LockGuard<Lockable> lock(_lock);
		return _generator->GetRandomColorRange(min, max);
	}


	int32 GetRandomInt32()
	{
		RN::RandomNumberGenerator *generator = RN::RandomNumberGenerator::GetSharedGenerator();
		return generator->GetRandomInt32();
	}

	int32 GetRandomInt32Range(int32 min, int32 max)
	{
		RN::RandomNumberGenerator *generator = RN::RandomNumberGenerator::GetSharedGenerator();
		return generator->GetRandomInt32Range(min, max);
	}

	float GetRandomFloat()
	{
		RN::RandomNumberGenerator *generator = RN::RandomNumberGenerator::GetSharedGenerator();
		return generator->GetRandomFloat();
	}

	float GetRandomFloatRange(float min, float max)
	{
		RN::RandomNumberGenerator *generator = RN::RandomNumberGenerator::GetSharedGenerator();
		return generator->GetRandomFloatRange(min, max);
	}

	Color GetRandomColor()
	{
		RN::RandomNumberGenerator *generator = RN::RandomNumberGenerator::GetSharedGenerator();
		return generator->GetRandomColor();
	}

	Color GetRandomColorRange(const Color &min, const Color &max)
	{
		RN::RandomNumberGenerator *generator = RN::RandomNumberGenerator::GetSharedGenerator();
		return generator->GetRandomColorRange(min, max);
	}

	Vector2 GetRandomVector2Range(const Vector2 &min, const Vector2 &max)
	{
		RN::RandomNumberGenerator *generator = RN::RandomNumberGenerator::GetSharedGenerator();
		return generator->GetRandomVector2Range(min, max);
	}

	Vector3 GetRandomVector3Range(const Vector3 &min, const Vector3 &max)
	{
		RN::RandomNumberGenerator *generator = RN::RandomNumberGenerator::GetSharedGenerator();
		return generator->GetRandomVector3Range(min, max);
	}

	Vector4 GetRandomVector4Range(const Vector4 &min, const Vector4 &max)
	{
		RN::RandomNumberGenerator *generator = RN::RandomNumberGenerator::GetSharedGenerator();
		return generator->GetRandomVector4Range(min, max);
	}
}