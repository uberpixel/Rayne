//
//  RNNewton.h
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NEWTON_H_
#define __RAYNE_NEWTON_H_

#include <Rayne.h>

#if defined(RN_BUILD_NEWTON)
	#define NDAPI RN_EXPORT
#else
	#define NDAPI RN_IMPORT
#endif

#endif /* __RAYNE_NEWTON_H_ */
