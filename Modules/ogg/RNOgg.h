//
//  RNOgg.h
//  Rayne-Ogg
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OGG_H_
#define __RAYNE_OGG_H_

#include <Rayne.h>

#if defined(RN_BUILD_OGG)
	#define OGGAPI RN_EXPORT
#else
	#define OGGAPI RN_IMPORT
#endif

#endif /* __RAYNE_OGG_H_ */
