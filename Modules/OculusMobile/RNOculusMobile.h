//
//  RNOculus.h
//  Rayne-Oculus
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OCULUS_H_
#define __RAYNE_OCULUS_H_

#include <Rayne.h>

#if defined(RN_BUILD_OCULUS)
	#define OVRAPI RN_EXPORT
#else
	#define OVRAPI RN_IMPORT
#endif

#endif /* __RAYNE_OCULUS_H_ */
