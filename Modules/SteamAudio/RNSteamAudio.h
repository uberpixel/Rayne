//
//  RNSteamAudio.h
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STEAMAUDIO_H_
#define __RAYNE_STEAMAUDIO_H_

#include <Rayne.h>

#if defined(RN_BUILD_STEAMAUDIO)
	#define SAAPI RN_EXPORT
#else
	#define SAAPI RN_IMPORT
#endif

#endif /* __RAYNE_STEAMAUDIO_H_ */
