//
//  RNOpenAL.h
//  Rayne-OpenAL
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OPENAL_H_
#define __RAYNE_OPENAL_H_

#include <Rayne.h>

#if defined(RN_BUILD_OPENAL)
	#define OALAPI RN_EXPORT
#else
	#define OALAPI RN_IMPORT
#endif

#endif /* __RAYNE_OPENAL_H_ */
