//
//  RNOpenXR.h
//  Rayne-OpenXR
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENXR_H_
#define __RAYNE_OPENXR_H_

#include <Rayne.h>

#if defined(RN_BUILD_OPENXR)
	#define OXRAPI RN_EXPORT
#else
	#define OXRAPI RN_IMPORT
#endif

#endif /* __RAYNE_OPENXR_H_ */
