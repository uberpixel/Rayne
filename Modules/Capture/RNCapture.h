//
//  RNCapture.h
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CAPTURE_H_
#define __RAYNE_CAPTURE_H_

#include <Rayne.h>

#if defined(RN_BUILD_CAPTURE)
	#define CAPTUREAPI RN_EXPORT
#else
	#define CAPTUREAPI RN_IMPORT
#endif

#endif
