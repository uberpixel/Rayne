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
		IPLAudioFormat mono;
		mono.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		mono.channelLayout = IPL_CHANNELLAYOUT_MONO;
		mono.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		IPLAudioFormat stereo;
		stereo.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		stereo.channelLayout = IPL_CHANNELLAYOUT_STEREO;
		stereo.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		IPLAudioBuffer inbuffer{ mono, size, frameIn };
		IPLAudioBuffer outbuffer{ stereo, size, frameOut };

//		iplApplyBinauralEffect(_audioWorld->GetBinauralRenderer(), inbuffer, IPLVector3{ _direction.x, _direction.y, _direction.z }, IPL_HRTFINTERPOLATION_BILINEAR, outbuffer); //TODO: Allow user to specify hrtf interpolation type
	}
}
