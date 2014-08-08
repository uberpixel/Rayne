//
//  RNAudioResource.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE__AUDIORESOURCE__
#define __RAYNE__AUDIORESOURCE__

#include "RNBase.h"
#include "RNObject.h"
#include "RNAsset.h"
#include "RNData.h"

namespace RN
{
	class AudioWorld;
	
	class AudioResource : public Asset
	{
	public:
		friend class AudioWorld;
		
		RNAPI AudioResource();
		RNAPI ~AudioResource();
		
		RNAPI void SetRawAudioData(Data *data, int bitsPerSample, int sampleRate, int channels);
		
		RNAPI Data *GetData() const { return _data; }
		RNAPI uint32 GetBitsPerSample() const { return _bitsPerSample; }
		RNAPI uint32 GetSampleRate() const { return _sampleRate; }
		RNAPI uint32 GetChannels() const { return _channels; }
		
		RNAPI static AudioResource *WithFile(const std::string& path);
		
	protected:
		Data *_data;
		uint32 _bitsPerSample;
		uint32 _sampleRate;
		uint32 _channels;
		
	private:
		
		RNDeclareMeta(AudioResource)
	};
}

#endif /* defined(__RAYNE__AUDIORESOURCE__) */
