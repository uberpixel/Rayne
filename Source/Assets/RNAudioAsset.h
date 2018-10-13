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
		enum Type
		{
			Static,
			Ringbuffer
		};

		RNAPI AudioAsset();
		RNAPI AudioAsset(Type type, size_t size, int bytesPerSample, int sampleRate, int channels);

		RNAPI ~AudioAsset();
		
		RNAPI void SetRawAudioData(Data *data, int bytesPerSample, int sampleRate, int channels);

		RNAPI void PushData(const void *bytes, size_t size);
		RNAPI void PopData(void *bytes, size_t size);
		
		Data *GetData() const { return _data; }
		uint32 GetBytesPerSample() const { return _bytesPerSample; }
		uint32 GetSampleRate() const { return _sampleRate; }
		uint32 GetChannels() const { return _channels; }
		uint32 GetBufferedSize() const { return _bufferedSize.load(); }
		Type GetType() const { return _type; }
		
		RNAPI static AudioAsset *WithName(const String *name, const Dictionary *settings = nullptr);
		RNAPI static AudioAsset *WithRingbuffer(size_t size, int bytesPerSample, int sampleRate, int channels);
		
	protected:
		Type _type;

		Data *_data;
		uint32 _bytesPerSample;
		uint32 _sampleRate;
		uint32 _channels;

		uint32 _readPosition;
		uint32 _writePosition;
		std::atomic<uint32> _bufferedSize;
		
	private:
		
		__RNDeclareMetaInternal(AudioAsset)
	};
}

#endif /* defined(__RAYNE__AUDIOASSET__) */
