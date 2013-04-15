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

namespace RN
{
	class RandomNumberGenerator
	{
	public:
		RandomNumberGenerator();
		virtual ~RandomNumberGenerator();
		
		virtual int32 Min() const = 0;
		virtual int32 Max() const = 0;
		
		virtual int32 RandomInt32() = 0;
		virtual int32 RandomInt32(int32 min, int32 max);
		
		virtual float RandomFloat();
		virtual float RandomFloat(int32 min, int32 max);
		
		virtual double UniformDeviate(int32 seed);
	};
	
	class SecureRNG : public RandomNumberGenerator
	{
	public:
		SecureRNG();
		virtual ~SecureRNG();
		
		virtual int32 Min() const;
		virtual int32 Max() const;
		
		virtual int32 RandomInt32();
		
		static bool UsesHardware();
		
	private:
		void FillBuffer();
		int ReadRDRAND();
		
		uint32 *bytes;
		int32 offset;
		int32 size;
	};
}

#endif /* __RAYNE_RANDOM_H__ */
