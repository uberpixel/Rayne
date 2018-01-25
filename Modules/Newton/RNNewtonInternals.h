//
//  RNNewtonInternals.h
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NEWTONINTERNALS_H_
#define __RAYNE_NEWTONINTERNALS_H_

#include "Newton.h"
#include <System/RNFile.h>

namespace RN
{
	class NewtonSerialization
	{
	public:
		static void SerializeCallback(void *serializeHandle, const void *buffer, int size);
		static void DeserializeCallback(void *serializeHandle, void *buffer, int size);
	};
}

#endif /* defined(__RAYNE_NEWTONINTERNALS_H_) */
