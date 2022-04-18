//
//  RNBHaptics.h
//  Rayne-BHaptics
//
//  Copyright 2022 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BHAPTICS_H_
#define __RAYNE_BHAPTICS_H_

#include <Rayne.h>

#if defined(RN_BUILD_BHAPTICS)
	#define BHAPI RN_EXPORT
#else
	#define BHAPI RN_IMPORT
#endif

#endif /* __RAYNE_BHAPTICS_H_ */
