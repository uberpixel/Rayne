//
//  RNAppleXR.h
//  Rayne-AppleXR
//
//  Copyright 2024 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_APPLEXR_H_
#define __RAYNE_APPLEXR_H_

#include <Rayne.h>

#if defined(RN_BUILD_APPLEXR)
	#define AXRAPI RN_EXPORT
#else
	#define AXRAPI RN_IMPORT
#endif

#endif /* __RAYNE_APPLEXR_H_ */
