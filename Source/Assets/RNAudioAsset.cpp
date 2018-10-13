//
//  RNAudioAsset.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAudioAsset.h"
#include "RNAssetManager.h"

namespace RN
{
	RNDefineMeta(AudioAsset, Asset)
	
	AudioAsset::AudioAsset() : _type(Type::Static), _readPosition(0), _writePosition(0)
	{

	}

	AudioAsset::AudioAsset(Type type, size_t size, int bytesPerSample, int sampleRate, int channels) : _type(type), _bytesPerSample(bytesPerSample), _sampleRate(sampleRate), _channels(channels), _readPosition(0), _writePosition(0), _bufferedSize(0)
	{
		_data = Data::WithBytes(nullptr, size)->Retain();
	}
	
	AudioAsset::~AudioAsset()
	{
		SafeRelease(_data);
	}
	
	void AudioAsset::SetRawAudioData(Data *data, int bytesPerSample, int sampleRate, int channels)
	{
		_data = data->Retain();
		_bytesPerSample = bytesPerSample;
		_sampleRate = sampleRate;
		_channels = channels;
	}

	void AudioAsset::PushData(const void *bytes, size_t size)
	{
		RN_ASSERT(_type == Type::Ringbuffer, "PushData can only be called on an AudioAsset initialized as Ringbuffer.");

		size_t remainingLength = size;
		size_t offset = 0;
		while(remainingLength)
		{
			size_t fittingLength = remainingLength;
			fittingLength = std::min(fittingLength, _data->GetLength() - _writePosition);
			_data->ReplaceBytes(static_cast<const uint8 *>(bytes) + offset, Range(_writePosition, fittingLength));
			offset += fittingLength;
			_writePosition += fittingLength;
			_writePosition %= _data->GetLength();

			_bufferedSize.fetch_add(fittingLength);
			remainingLength -= fittingLength;
		}
	}

	void AudioAsset::PopData(void *bytes, size_t size)
	{
		RN_ASSERT(_type == Type::Ringbuffer, "PopData can only be called on an AudioAsset initialized as Ringbuffer.");

		if(!bytes)
		{
			_readPosition += size;
			_bufferedSize.fetch_sub(size);
			_readPosition %= _data->GetLength();
			return;
		}

		uint8 *data = static_cast<uint8 *>(bytes);
		size_t remainingLength = size;
		while(remainingLength)
		{
			size_t fittingLength = remainingLength;
			fittingLength = std::min(fittingLength, _data->GetLength() - _readPosition);
			_data->GetBytesInRange(data, Range(_readPosition, fittingLength));
			_readPosition += fittingLength;
			data += fittingLength;
			_readPosition %= _data->GetLength();

			_bufferedSize.fetch_sub(fittingLength);
			remainingLength -= fittingLength;
		}
	}

	AudioAsset *AudioAsset::WithName(const String *name, const Dictionary *settings)
	{
		AssetManager *coordinator = AssetManager::GetSharedInstance();
		return coordinator->GetAssetWithName<AudioAsset>(name, settings);
	}

	AudioAsset* AudioAsset::WithRingbuffer(size_t size, int bitsPerSample, int sampleRate, int channels)
	{
		AudioAsset *asset = new AudioAsset(Type::Ringbuffer, size, bitsPerSample, sampleRate, channels);
		return asset->Autorelease();
	}

}
