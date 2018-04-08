//
//  RNSplash.h
//  Rayne-Splash
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SPLASH_H_
#define __RAYNE_SPLASH_H_

#include <Rayne.h>

#if defined(RN_BUILD_SPLASH)
	#define SPAPI RN_EXPORT
#else
	#define SPAPI RN_IMPORT
#endif

#endif /* __RAYNE_SPLASH_H_ */
