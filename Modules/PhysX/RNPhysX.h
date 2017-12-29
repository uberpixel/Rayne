//
//  RNPhysX.h
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSX_H_
#define __RAYNE_PHYSX_H_

#include <Rayne.h>

#if defined(RN_BUILD_PHYSX)
	#define PXAPI RN_EXPORT
#else
	#define PXAPI RN_IMPORT
#endif

#endif /* __RAYNE_PHYSX_H_ */
