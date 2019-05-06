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
#include "OVR_Audio.h"

#if RN_PLATFORM_WINDOWS || RN_PLATFORM_MAC_OS || RN_PLATFORM_LINUX
	#include "RtAudio.h"
#elif RN_PLATFORM_ANDROID

#endif

namespace RN
{
	struct OculusAudioWorldInternals
	{
		ovrAudioContext oculusAudioContext;
	};

#if RN_PLATFORM_WINDOWS || RN_PLATFORM_MAC_OS || RN_PLATFORM_LINUX
	struct OculusAudioSystemRtAudioInternals
	{
		RtAudio rtAudioContext;
	};
#elif RN_PLATFORM_ANDROID
	
#endif
}

#endif /* defined(__RAYNE_OculusAudioINTERNALS_H_) */
