//
//  RNSteamAudioInternals.h
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STEAMAUDIOINTERNALS_H_
#define __RAYNE_STEAMAUDIOINTERNALS_H_

#include "RNSteamAudio.h"
#include "phonon.h"

namespace RN
{
	struct SteamAudioWorldInternals
	{
		IPLContext context;
		IPLRenderingSettings settings;

		IPLAudioFormat internalAmbisonicsFormat;
		IPLAudioFormat outputFormat;
	};

	struct SteamAudioSourceInternals
	{
		IPLhandle panningEffect;
		IPLhandle convolutionEffect;

		IPLAudioBuffer inputBuffer;
		IPLAudioBuffer outputBuffer;
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOINTERNALS_H_) */
