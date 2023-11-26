//
//  RNJolt.h
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLT_H_
#define __RAYNE_JOLT_H_

#include <Rayne.h>

#if defined(RN_BUILD_JOLT)
	#define JTAPI RN_EXPORT
#else
	#define JTAPI RN_IMPORT
#endif

#endif /* __RAYNE_JOLT_H_ */
