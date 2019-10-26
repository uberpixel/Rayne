//
//  RNOculusAudioInternals.h
//  Rayne-OculusAudio
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OculusAudioINTERNALS_H_
#define __RAYNE_OculusAudioINTERNALS_H_

#include "RNOculusAudio.h"

#if !RN_PLATFORM_ANDROID
	#include "OVR_Audio.h"
#endif

#if RN_PLATFORM_WINDOWS || RN_PLATFORM_MAC_OS || RN_PLATFORM_LINUX
	#include "RtAudio.h"
#elif RN_PLATFORM_ANDROID

#endif

namespace RN
{
#if !RN_PLATFORM_ANDROID
	struct OculusAudioWorldInternals
	{
		ovrAudioContext oculusAudioContext;
	};
#endif

#if RN_PLATFORM_WINDOWS || RN_PLATFORM_MAC_OS || RN_PLATFORM_LINUX
	struct OculusAudioSystemRtAudioInternals
	{
		RtAudio outputContext;
		RtAudio inputContext;
	};
#elif RN_PLATFORM_ANDROID
	
#endif
}

#endif /* defined(__RAYNE_OculusAudioINTERNALS_H_) */
