//
//  RNSTL.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STL_H__
#define __RAYNE_STL_H__

#include "RNAlgorithm.h"

namespace std
{
	template<>
	struct hash<const char *>
	{
		size_t operator()(const char *string) const
		{
			size_t length = strlen(string);
			size_t hash = 0;
			
			for(size_t i = 0; i< length; i ++)
			{
				RN::HashCombine(hash, string[i]);
			}
			
			return hash;
		}
	};
	
	template<>
	struct equal_to<const char *>
	{
		bool operator()(const char *string1, const char *string2) const
		{
			return (strcmp(string1, string2) == 0);
		}
	};
}

#endif
