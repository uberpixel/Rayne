//
//  RNAudioAsset.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNAudioAsset.h"
#include "RNAssetManager.h"
#include "../Debug/RNLogger.h"

namespace RN
{
	RNDefineMeta(AudioDecoder, Object)
	RNDefineMeta(AudioAsset, Asset)
	
	AudioAsset::AudioAsset() : _type(Type::Static), _readPosition(0), _writePosition(0)
	{

	}

	AudioAsset::AudioAsset(Type type, size_t size, int bytesPerSample, int sampleRate, int channels) : _type(type), _bytesPerSample(bytesPerSample), _sampleRate(sampleRate), _channels(channels), _readPosition(0), _writePosition(0), _bufferedSize(0)
	{
		_data = Data::WithBytes(nullptr, size)->Retain();
	}

	AudioAsset::AudioAsset(AudioDecoder *decoder, size_t size, int bytesPerSample, int sampleRate, int channels) : _type(Type::Decoder), _bytesPerSample(bytesPerSample), _sampleRate(sampleRate), _channels(channels), _readPosition(0), _writePosition(0), _bufferedSize(0), _decoder(decoder)
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
		RN_ASSERT(_type == Type::Ringbuffer || _type == Type::Decoder, "PushData can only be called on an AudioAsset initialized as Ringbuffer or decoder.");

		size_t remainingLength = size;
		size_t offset = 0;
		while(remainingLength)
		{
			//TODO: Something seems wrong about always assuming it can write to end of data cause read position can be in front if a lot has already been buffered!? Probably only works cause the buffer is big enough and it's only buffering n frames in the places I use this
			size_t fittingLength = std::min(remainingLength, _data->GetLength() - _writePosition);
			_data->ReplaceBytes(static_cast<const uint8 *>(bytes) + offset, Range(_writePosition, fittingLength));
			offset += fittingLength;
			_writePosition.fetch_add(fittingLength);
			
			//Do the modulo atomically by trying it until the values match
			uint32 oldValue = _writePosition;
			uint32 newValue = _writePosition % _data->GetLength();
			while(!_writePosition.compare_exchange_weak(oldValue, newValue, std::memory_order_relaxed))
			{
				newValue = oldValue % _data->GetLength();
			}

			_bufferedSize.fetch_add(fittingLength);
			remainingLength -= fittingLength;
		}
	}

	void AudioAsset::PopData(void *bytes, size_t size)
	{
		RN_ASSERT(_type == Type::Ringbuffer || _type == Type::Decoder, "PopData can only be called on an AudioAsset initialized as Ringbuffer or Decoder.");

		if(!bytes)
		{
			_readPosition.fetch_add(size);
			_bufferedSize.fetch_sub(size);
			
			//Do the modulo atomically by trying it until the values match
			uint32 oldValue = _readPosition;
			uint32 newValue = _readPosition % _data->GetLength();
			while(!_readPosition.compare_exchange_weak(oldValue, newValue, std::memory_order_relaxed))
			{
				newValue = oldValue % _data->GetLength();
			}
			return;
		}

		uint8 *data = static_cast<uint8 *>(bytes);
		size_t remainingLength = size;
		while(remainingLength)
		{
			size_t fittingLength = remainingLength;
			fittingLength = std::min(fittingLength, _data->GetLength() - _readPosition);
			_data->GetBytesInRange(data, Range(_readPosition, fittingLength));
			_readPosition.fetch_add(fittingLength);
			data += fittingLength;
			
			//Do the modulo atomically by trying it until the values match
			uint32 oldValue = _readPosition;
			uint32 newValue = _readPosition % _data->GetLength();
			while(!_readPosition.compare_exchange_weak(oldValue, newValue, std::memory_order_relaxed))
			{
				newValue = oldValue % _data->GetLength();
			}

			_bufferedSize.fetch_sub(fittingLength);
			remainingLength -= fittingLength;
		}
	}

	bool AudioAsset::Decode()
	{
		RN_ASSERT(_type == Type::Decoder, "Decode can only be called on an AudioAsset initialized as Decoder.");
		
		int32 remainingLength = _data->GetLength() - GetBufferedSize();
		while(remainingLength > _decoder->_frameSize)
		{
			uint32 encodedBytesCount = _decoder->DecodeFrameToAudioAsset(this);
			if(encodedBytesCount == 0)
			{
				return false;
			}
			
			remainingLength = _data->GetLength() - GetBufferedSize();
		}
		
		return true;
	}

	void AudioAsset::Seek(float time)
	{
		RN_ASSERT(_type == Type::Decoder, "Seek can only be called on an AudioAsset initialized as Decoder.");
		
		_decoder->Seek(time);
	}

	AudioAsset *AudioAsset::WithName(const String *name, const Dictionary *settings)
	{
		AssetManager *coordinator = AssetManager::GetSharedInstance();
		return coordinator->GetAssetWithName<AudioAsset>(name, settings);
	}

	AudioAsset* AudioAsset::WithRingbuffer(size_t size, int bytesPerSample, int sampleRate, int channels)
	{
		AudioAsset *asset = new AudioAsset(Type::Ringbuffer, size, bytesPerSample, sampleRate, channels);
		return asset->Autorelease();
	}

	AudioAsset* AudioAsset::WithDecoder(AudioDecoder *decoder, size_t bufferSize, int bytesPerSample, int sampleRate, int channels)
	{
		AudioAsset *asset = new AudioAsset(decoder, bufferSize, bytesPerSample, sampleRate, channels);
		return asset->Autorelease();
	}
}
