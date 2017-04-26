//
//  RNVR.h
//  Rayne-VR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VR_H_
#define __RAYNE_VR_H_

#include <Rayne.h>

#if defined(RN_BUILD_VR)
#define RNVRAPI RN_EXPORT
#else
#define RNVRAPI RN_IMPORT
#endif

#endif /* __RAYNE_VR_H_ */
