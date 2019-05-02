//
//  RNOculusAudio.h
//  Rayne-OculusAudio
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OculusAudio_H_
#define __RAYNE_OculusAudio_H_

#include <Rayne.h>

#if defined(RN_BUILD_OCULUSAUDIO)
	#define OAAPI RN_EXPORT
#else
	#define OAAPI RN_IMPORT
#endif

#endif /* __RAYNE_OculusAudio_H_ */
