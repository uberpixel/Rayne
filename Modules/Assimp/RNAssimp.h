//
//  RNAssimp.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ASSIMP_H_
#define __RAYNE_ASSIMP_H_

#include <Rayne.h>

#if defined(RN_BUILD_ASSIMP)
	#define ASAPI RN_EXPORT
#else
	#define ASAPI RN_IMPORT
#endif

#endif /* __RAYNE_ASSIMP_H_ */
