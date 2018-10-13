//
//  RNOpenVR.h
//  Rayne-OpenVR
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENVR_H_
#define __RAYNE_OPENVR_H_

#include <Rayne.h>

#if defined(RN_BUILD_OPENVR)
	#define OVRAPI RN_EXPORT
#else
	#define OVRAPI RN_IMPORT
#endif

#endif /* __RAYNE_OPENVR_H_ */
