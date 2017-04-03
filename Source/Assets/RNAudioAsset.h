//
//  RNAudioAsset.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE__AUDIOASSET__
#define __RAYNE__AUDIOASSET__

#include "RNAsset.h"
#include "../Objects/RNData.h"

namespace RN
{
	class AudioWorld;
	
	class AudioAsset : public Asset
	{
	public:
		RNAPI AudioAsset();
		RNAPI ~AudioAsset();
		
		RNAPI void SetRawAudioData(Data *data, int bitsPerSample, int sampleRate, int channels);
		
		RNAPI Data *GetData() const { return _data; }
		RNAPI uint32 GetBitsPerSample() const { return _bitsPerSample; }
		RNAPI uint32 GetSampleRate() const { return _sampleRate; }
		RNAPI uint32 GetChannels() const { return _channels; }
		
		RNAPI static AudioAsset *WithName(const String *name, const Dictionary *settings = nullptr);
		
	protected:
		Data *_data;
		uint32 _bitsPerSample;
		uint32 _sampleRate;
		uint32 _channels;
		
	private:
		
		__RNDeclareMetaInternal(AudioAsset)
	};
}

#endif /* defined(__RAYNE__AUDIOASSET__) */
