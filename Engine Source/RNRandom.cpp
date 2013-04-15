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
	RandomNumberGenerator::RandomNumberGenerator()
	{
	}
	RandomNumberGenerator::~RandomNumberGenerator()
	{
	}
	
	int32 RandomNumberGenerator::RandomInt32(int32 min, int32 max)
	{
		int32 base = RandomInt32();
		if(base == Max())
			return RandomInt32(min, max);
		
		int32 range = max - min;
		int32 remainder = Max() % range;
		int32 bucket    = Max() / range;
		
		if(base < Max() - remainder)
			return min + base / bucket;
		
		return RandomInt32(min, max);
	}
	
	float RandomNumberGenerator::RandomFloat()
	{
		return UniformDeviate(RandomInt32());
	}
	
	float RandomNumberGenerator::RandomFloat(int32 min, int32 max)
	{
		float M = (float)MIN(min, max);
		float N = (float)MAX(min, max);
		
		return M + UniformDeviate(RandomInt32()) * (N - M);
	}
	
	double RandomNumberGenerator::UniformDeviate(int32 seed)
	{
		return seed * (1.0 / (Max() + 1.0));
	}
	
	
	
	SecureRNG::SecureRNG()
	{
		offset = 0;
		size   = 0;
		bytes  = new uint32[kRNSecureRNGSize];
	}
	
	SecureRNG::~SecureRNG()
	{
		delete [] bytes;
	}
	
	int SecureRNG::ReadRDRAND()
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
			*(bytes + size) = result;
			size ++;
		}
		
		return status;
	}
	
	void SecureRNG::FillBuffer()
	{
		offset = 0;
		size   = 0;
		
		if((X86_64::Caps() & X86_64::CAP_RDRAND))
		{
			while(size < kRNSecureRNGSize)
			{
				int result = ReadRDRAND();
				if(!result)
				{
					if(size > 0)
						break;
					
					std::this_thread::sleep_for(std::chrono::nanoseconds(10));
				}
			}
		}
		else
		{
			FILE *file = fopen("/dev/random", "rb");
			fread(bytes, kRNSecureRNGSize, 1, file);
			fclose(file);
			
			size = kRNSecureRNGSize;
		}
	}
	
	
	int32 SecureRNG::Min() const
	{
		return 0;
	}
	
	int32 SecureRNG::Max() const
	{
		return INT32_MAX;
	}
	
	int32 SecureRNG::RandomInt32()
	{
		if(offset >= size)
		{
			FillBuffer();
		}
		
		int32 result;
		std::copy(bytes + offset, bytes + offset + 1, &result);
		offset ++;
		
		return result & (1 << 31) - 1;
	}
	
	bool SecureRNG::UsesHardware()
	{
		return (X86_64::Caps() & X86_64::CAP_RDRAND);
	}
}
