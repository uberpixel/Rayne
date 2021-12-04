//
//  RNglTF.h
//  Rayne
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_GLTF_H_
#define __RAYNE_GLTF_H_

#include <Rayne.h>

#if defined(RN_BUILD_GLTF)
	#define TFAPI RN_EXPORT
#else
	#define TFAPI RN_IMPORT
#endif

#endif /* __RAYNE_GLTF_H_ */
