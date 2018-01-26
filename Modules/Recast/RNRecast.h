//
//  RNRecast.h
//  Rayne-Recast
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RECAST_H_
#define __RAYNE_RECAST_H_

#include <Rayne.h>

#if defined(RN_BUILD_RECAST)
	#define RCAPI RN_EXPORT
#else
	#define RCAPI RN_IMPORT
#endif

#endif /* __RAYNE_RECAST_H_ */
