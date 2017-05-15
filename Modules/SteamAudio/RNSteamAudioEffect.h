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
	class SteamAudioEffect : public Object
	{
	public:
		SAAPI SteamAudioEffect();
		SAAPI ~SteamAudioEffect() override;

		SAAPI virtual float ProcessSample(float sample) const;
			
	private:
			
		RNDeclareMetaAPI(SteamAudioEffect, SAAPI)
	};
}

#endif /* defined(__RAYNE_STEAMAUDIOEFFECT_H_) */
