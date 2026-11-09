//
//  RNResonanceAudioSampler.h
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ResonanceAudioSAMPLER_H_
#define __RAYNE_ResonanceAudioSAMPLER_H_

#include "RNResonanceAudio.h"

namespace RN
{
	class ResonanceAudioSource;
	class ResonanceAudioSampler : public Object
	{
	public:
		RAAPI ResonanceAudioSampler(AudioAsset *asset = nullptr);
		RAAPI ~ResonanceAudioSampler() override;

		RAAPI void SetAudioAsset(AudioAsset *asset);
		RAAPI float GetSample(double time, uint8 channel, bool isRepeating);
		RAAPI double GetTotalTime() const;

		AudioAsset *GetAsset() const { return _asset; }

		//TODO: Implement an optional effects pipeline

	private:
		friend class ResonanceAudioSource;

		// Transfer the new retained reference in and the old one out.
		AudioAsset *ExchangeRetainedAsset(AudioAsset *asset);

		AudioAsset *_asset;
		double _totalTime;

		RNDeclareMetaAPI(ResonanceAudioSampler, RAAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_ResonanceAudioSAMPLER_H_) */
