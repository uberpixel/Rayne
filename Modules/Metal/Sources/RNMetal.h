//
//  RNMetal.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METAL_H_
#define __RAYNE_METAL_H_

#include <Rayne.h>
#import <Metal/Metal.h>

#if defined(RN_BUILD_METAL)
	#define MTLAPI RN_EXPORT
#else
	#define MTLAPI RN_IMPORT
#endif


#endif /* __RAYNE_METAL_H_ */
