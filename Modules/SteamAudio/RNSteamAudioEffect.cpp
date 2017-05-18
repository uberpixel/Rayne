//
//  RNSteamAudioEffect.cpp
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSteamAudioEffect.h"
#include "RNSteamAudioWorld.h"
#include "phonon.h"

namespace RN
{
	RNDefineMeta(SteamAudioEffect, Object)

	SteamAudioEffect::SteamAudioEffect(SteamAudioWorld *audioWorld) : _audioWorld(audioWorld)
	{
		
	}
		
	SteamAudioEffect::~SteamAudioEffect()
	{
		
	}

	void SteamAudioEffect::ProcessFrame(float *frameIn, float *frameOut, uint32 size) const
	{
		
	}


	RNDefineMeta(SteamAudioEffectBinaural, SteamAudioEffect)

	SteamAudioEffectBinaural::SteamAudioEffectBinaural(SteamAudioWorld *audioWorld) : SteamAudioEffect(audioWorld), _direction(0.0f, 0.0f, -1.0f)
	{

	}

	SteamAudioEffectBinaural::~SteamAudioEffectBinaural()
	{

	}

	void SteamAudioEffectBinaural::SetDirectionToSource(Vector3 direction)
	{
		_direction = direction;
	}

	void SteamAudioEffectBinaural::ProcessFrame(float *frameIn, float *frameOut, uint32 size) const
	{
		
	}
}
