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
		
		AudioResource();
		~AudioResource();
		
		void SetRawAudioData(Data *data, int bitsPerSample, int sampleRate, int channels);
		
		Data *GetData() const { return _data; }
		uint32 GetBitsPerSample() const { return _bitsPerSample; }
		uint32 GetSampleRate() const { return _sampleRate; }
		uint32 GetChannels() const { return _channels; }
		
		static AudioResource *WithFile(const std::string& path);
		
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
