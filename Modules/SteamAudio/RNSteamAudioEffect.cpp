//
//  RNSteamAudioEffect.cpp
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSteamAudioEffect.h"

namespace RN
{
	RNDefineMeta(SteamAudioEffect, Object)

	SteamAudioEffect::SteamAudioEffect()
	{
		
	}
		
	SteamAudioEffect::~SteamAudioEffect()
	{
		
	}

	float SteamAudioEffect::ProcessSample(float sample) const
	{
		return sample;
	}
}
