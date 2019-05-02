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
#include "RtAudio.h"

namespace RN
{
	struct OculusAudioWorldInternals
	{
		RtAudio rtAudioContext;
		ovrAudioContext oculusAudioContext;
		/*IPLhandle context;
		IPLRenderingSettings settings;

		IPLAudioFormat internalAmbisonicsFormat;
		IPLAudioFormat outputFormat;*/
	};

	struct OculusAudioSourceInternals
	{
		/*IPLhandle panningEffect;
		IPLhandle convolutionEffect;

		IPLAudioBuffer inputBuffer;
		IPLAudioBuffer outputBuffer;*/
	};
}

#endif /* defined(__RAYNE_OculusAudioINTERNALS_H_) */
