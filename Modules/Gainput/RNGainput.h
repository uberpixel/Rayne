//
//  RNGainput.h
//  Rayne-Gainput
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_Gainput_H_
#define __RAYNE_Gainput_H_

#include <Rayne.h>

#if defined(RN_BUILD_GAINPUT)
	#define GPAPI RN_EXPORT
#else
	#define GPAPI RN_IMPORT
#endif

#endif /* __RAYNE_Gainput_H_ */
