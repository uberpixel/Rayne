//
//  RNSteamAudioEffect.h
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_STEAMAUDIOEFFECT_H_
#define __RAYNE_STEAMAUDIOEFFECT_H_

#include "RNSteamAudio.h"

namespace RN
{
	class SteamAudioWorld;
	class SteamAudioEffect : public Object
	{
	public:
		SAAPI SteamAudioEffect(SteamAudioWorld *audioWorld);
		SAAPI ~SteamAudioEffect() override;

		SAAPI virtual void ProcessFrame(float *frameIn, float *frameOut, uint32 size) const;
	
	protected:
		SteamAudioWorld *_audioWorld;

	private:
			
		RNDeclareMetaAPI(SteamAudioEffect, SAAPI)
	};

	class SteamAudioEffectBinaural : public SteamAudioEffect
	{
	public:
		SAAPI SteamAudioEffectBinaural(SteamAudioWorld *audioWorld);
		SAAPI ~SteamAudioEffectBinaural() override;

		SAAPI void SetDirectionToSource(Vector3 direction);

		SAAPI void ProcessFrame(float *frameIn, float *frameOut, uint32 size) const final;

	private:
		Vector3 _direction;

		RNDeclareMetaAPI(SteamAudioEffectBinaural, SAAPI)
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOEFFECT_H_) */
