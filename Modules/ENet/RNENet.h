//
//  RNENet.h
//  Rayne-ENet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ENET_H_
#define __RAYNE_ENET_H_

#include <Rayne.h>

#if defined(RN_BUILD_ENET)
	#define ENAPI RN_EXPORT
#else
	#define ENAPI RN_IMPORT
#endif

#endif /* __RAYNE_ENET_H_ */
