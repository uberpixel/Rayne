//
//  RNOpenALSource.cpp
//  Rayne-OpenAL
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOpenALSource.h"
#include "RNOpenALWorld.h"
#include "RNOpenALResourceAttachment.h"

#include "AL/al.h"
#include "AL/alc.h"

namespace RN
{
	RNDefineMeta(OpenALSource, SceneNode)

		OpenALSource::OpenALSource(AudioAsset *asset) :
		_asset(asset),
		_isPlaying(false),
		_isRepeating(false),
		_isSelfdestructing(false),
		_hasEnded(false),
		_currentBuffer(0),
		_ringBufferTemp(nullptr)
	{
		SafeRetain(_asset);
		_oldPosition = GetWorldPosition();
			
		alGenSources(1, &_source);
		alSourcef(_source, AL_PITCH, 1);
		alSourcef(_source, AL_GAIN, 1);
		alSourcei(_source, AL_LOOPING, AL_FALSE);

		SetAudioAsset(asset);
	}
		
	OpenALSource::~OpenALSource()
	{
		alDeleteSources(1, &_source);
		
		if(_asset && _asset->GetType() == AudioAsset::Type::Ringbuffer)
		{
			alDeleteBuffers(3, _ringBuffersID);
		}
		SafeRelease(_asset);
		
		delete[] _ringBufferTemp;
	}

	void OpenALSource::SetAudioAsset(AudioAsset *asset)
	{
		alSourcei(_source, AL_BUFFER, 0);
		
		if(_asset && _asset->GetType() == AudioAsset::Type::Ringbuffer)
		{
			alDeleteBuffers(3, _ringBuffersID);
		}

		SafeRelease(_asset);
		_asset = asset;
		SafeRetain(_asset);

		if(!_asset) return;
		
		if(_asset->GetType() == AudioAsset::Type::Static)
		{
			OpenALResourceAttachment *attachment = OpenALResourceAttachment::GetAttachmentForResource(asset);
			alSourcei(_source, AL_BUFFER, attachment->GetBufferID());
		}
		else if(_asset->GetType() == AudioAsset::Type::Ringbuffer)
		{
			alGenBuffers(3, _ringBuffersID);
			
			_ringBufferTemp = new int16[3840];
			std::fill(_ringBufferTemp, _ringBufferTemp + sizeof(_ringBufferTemp), 0);
			
			//TODO: make the format more flexible
			alBufferData(_ringBuffersID[0], AL_FORMAT_MONO16, _ringBufferTemp, 3840*sizeof(int16), _asset->GetSampleRate());
			alBufferData(_ringBuffersID[1], AL_FORMAT_MONO16, _ringBufferTemp, 3840*sizeof(int16), _asset->GetSampleRate());
			alSourceQueueBuffers(_source, 2, _ringBuffersID);
			
			_currentBuffer = 1;
		}
	}
		
	void OpenALSource::SetRepeat(bool repeat)
	{
		_isRepeating = repeat;
		alSourcei(_source, AL_LOOPING, repeat?AL_TRUE: AL_FALSE);
	}
		
	void OpenALSource::SetPitch(float pitch)
	{
		alSourcef(_source, AL_PITCH, pitch);
	}
		
	void OpenALSource::SetGain(float gain)
	{
		alSourcef(_source, AL_GAIN, gain);
	}
		
	void OpenALSource::SetRange(float min, float max)
	{
		alSourcei(_source, AL_REFERENCE_DISTANCE, min);
		alSourcei(_source, AL_MAX_DISTANCE, max);
	}
		
	void OpenALSource::SetSelfdestruct(bool selfdestruct)
	{
		_isSelfdestructing = selfdestruct;
	}
		
	void OpenALSource::Play()
	{
		Update(0.0f);

		alSourcePlay(_source);
		_isPlaying = true;
		_hasEnded = false;
	}

	void OpenALSource::Stop()
	{
		alSourceStop(_source);
		alSourcePause(_source);
        _isPlaying = false;
	}
	
	void OpenALSource::Pause()
	{
		alSourcePause(_source);
		_isPlaying = false;
	}

    void OpenALSource::Seek(float time)
    {
		alSourcef(_source, AL_SEC_OFFSET, time);
    }
	
	void OpenALSource::Update(float delta)
	{
		if(_asset && _asset->GetType() == AudioAsset::Type::Ringbuffer)
		{
			ALint numberOfProcessedBuffers = 0;
			alGetSourcei(_source, AL_BUFFERS_PROCESSED, &numberOfProcessedBuffers);
			if(numberOfProcessedBuffers >= 1)
			{
				uint32 bufferedSamples = _asset->GetBufferedSize() / _asset->GetBytesPerSample();
				if(bufferedSamples >= 3840)
				{
					if(bufferedSamples > 3840*5)
					{
						_asset->PopData(nullptr, _asset->GetBufferedSize() - 2 * 3840 * _asset->GetBytesPerSample());
						
						RNDebug("too much buffered audio: skipping");
					}
					
					//TODO: Make better and don't hardcode sample type and buffer format
					float samplesBuffer[3840];
					_asset->PopData(samplesBuffer, _asset->GetBytesPerSample()*3840);
					for(size_t i = 0; i < 3840; i++)
					{
						_ringBufferTemp[i] = samplesBuffer[i] * 32000.0f;
					}
				}
				else
				{
					std::fill(_ringBufferTemp, _ringBufferTemp + sizeof(_ringBufferTemp), 0);
					RNDebug("not enough buffered audio: adding silence");
				}
				
				RN::int32 bufferIndex = _currentBuffer + 2;
				if(bufferIndex > 2) bufferIndex = bufferIndex - 3;
				alBufferData(_ringBuffersID[bufferIndex], AL_FORMAT_MONO16, _ringBufferTemp, 3840*sizeof(int16), _asset->GetSampleRate());
				
				
				bufferIndex = _currentBuffer - 1;
				if(bufferIndex < 0) bufferIndex = 2;
				ALuint oldBuffer = _ringBuffersID[bufferIndex];
				alSourceUnqueueBuffers(_source, 1, &oldBuffer);
				
				_currentBuffer += 1;
				if(_currentBuffer > 2) _currentBuffer = 0;
				ALuint newBuffer = _ringBuffersID[bufferIndex];
				alSourceQueueBuffers(_source, 1, &newBuffer);
			}
		}
		
		ALenum sourceState = AL_STOPPED;
		alGetSourcei(_source, AL_SOURCE_STATE, &sourceState);
		if(sourceState == AL_STOPPED)
		{
			RNDebug("no more audio to play");
			_isPlaying = false;
			_hasEnded = true;
			if(_isSelfdestructing)
			{
				if(GetSceneInfo())
					GetSceneInfo()->GetScene()->RemoveNode(this);
			}
		}
		
		Vector3 position = GetWorldPosition();
		alSourcefv(_source, AL_POSITION, &position.x);

		Vector3 velocity = position - _oldPosition;
		_oldPosition = position;

		if(delta == 0.0f)
			return;

		velocity /= delta;
		alSourcefv(_source, AL_VELOCITY, &velocity.x);
	}
}
