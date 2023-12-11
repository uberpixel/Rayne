//
//  RNResonanceAudioSource.cpp
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResonanceAudioSource.h"
#include "RNResonanceAudioWorld.h"
#include "RNResonanceAudioSampler.h"

#include <api/resonance_audio_api.h>

namespace RN
{
	RNDefineMeta(ResonanceAudioSource, SceneNode)

	ResonanceAudioSource::ResonanceAudioSource(AudioAsset *asset, bool wantsIndirectSound) :
		_channel(0),
		_sampler(new ResonanceAudioSampler(asset)),
		_sourceID(vraudio::ResonanceAudioApi::kInvalidSourceId),
		_wantsIndirectSound(wantsIndirectSound),
		_isPlaying(false),
		_isRepeating(false),
//		_isSelfdestructing(false),
		_hasTimeOfFlight(true),
		_hasReverb(true),
		_gain(1.0f),
		_pitch(1.0f),
		_minMaxRange(RN::Vector2(0.2f, 200.0f)),
		_currentTime(0.0f)
	{
		RN_ASSERT(ResonanceAudioWorld::_instance, "You need to create a ResonanceAudioWorld before creating audio sources!");

		ResonanceAudioWorld::_instance->AddAudioSource(this);

		//TODO: Make quality adjustable
		_sourceID = ResonanceAudioWorld::_instance->_audioAPI->CreateSoundObjectSource(vraudio::RenderingMode::kBinauralHighQuality);
		ResonanceAudioWorld::_instance->_audioAPI->SetSourceDistanceModel(_sourceID, vraudio::DistanceRolloffModel::kLinear, 1.0f, 20.0f);
	}
	
	ResonanceAudioSource::~ResonanceAudioSource()
	{
		ResonanceAudioWorld::_instance->RemoveAudioSource(this);
		ResonanceAudioWorld::_instance->_audioAPI->DestroySource(_sourceID);
		_sampler->Release();
	}

	void ResonanceAudioSource::SetAudioAsset(AudioAsset *asset)
	{
		_sampler->SetAudioAsset(asset);
	}
		
	void ResonanceAudioSource::SetRepeat(bool repeat)
	{
		_sampler->SetRepeat(repeat);
		_isRepeating = repeat;
	}

	void ResonanceAudioSource::SetDistanceAttenuation(float attentuation)
	{
		ResonanceAudioWorld::_instance->_audioAPI->SetSourceDistanceAttenuation(_sourceID, attentuation);
	}
	
	void ResonanceAudioSource::SetPitch(float pitch)
	{
		_pitch = pitch;
	}
		
	void ResonanceAudioSource::SetVolume(float volume)
	{
		ResonanceAudioWorld::_instance->_audioAPI->SetSourceVolume(_sourceID, volume);
	}
		
/*	void ResonanceAudioSource::SetSelfdestruct(bool selfdestruct)
	{
		_isSelfdestructing = selfdestruct;
	}*/

	void ResonanceAudioSource::SetTimeOfFlight(bool tof)
	{
		_hasTimeOfFlight = tof;
	}

		
	void ResonanceAudioSource::Play()
	{
		_isPlaying = true;
	}

	void ResonanceAudioSource::Stop()
	{
		_isPlaying = false;
	}
	
	void ResonanceAudioSource::Seek(double time)
	{
		_currentTime = time;
	}
	
	bool ResonanceAudioSource::HasEnded() const
	{
		return (_currentTime >= _sampler->GetTotalTime());
	}

	void ResonanceAudioSource::Update(double frameLength, uint32 sampleCount, float **outputBuffer)
	{
		AudioAsset *asset = _sampler->GetAsset();
		if(!asset)
		{
			*outputBuffer = nullptr;
			return;
		}

		if(_sampler->GetAsset()->GetType() == AudioAsset::Type::Ringbuffer)
		{
			//Buffer for audio data to play
			uint32 assetFrameSamples = std::round(frameLength * _sampler->GetAsset()->GetSampleRate() * _sampler->GetAsset()->GetBytesPerSample());
			if(_sampler->GetAsset()->GetBufferedSize() < assetFrameSamples)
			{
				*outputBuffer = nullptr;
				return;
			}
			else
			{
				//Skip samples if data is written faster than played
				uint32 maxBufferedLength = assetFrameSamples * 20;
				if(_sampler->GetAsset()->GetBufferedSize() > maxBufferedLength)
				{
					uint32 skipBytes = _sampler->GetAsset()->GetBufferedSize() - assetFrameSamples;
					double skipTime = skipBytes / _sampler->GetAsset()->GetBytesPerSample() / static_cast<double>(_sampler->GetAsset()->GetSampleRate());
					_currentTime += skipTime;
					_sampler->GetAsset()->PopData(nullptr, skipBytes);
				}

				_sampler->GetAsset()->PopData(nullptr, assetFrameSamples);
			}
		}

		double sampleLength = frameLength / static_cast<double>(sampleCount);
		double localTime = _currentTime;
		for(int i = 0; i < sampleCount; i++)
		{
			ResonanceAudioWorld::_instance->_sharedFrameData[i] = _sampler->GetSample(localTime, _channel);
			localTime += sampleLength * _pitch;
		}

		_currentTime = localTime;
		*outputBuffer = ResonanceAudioWorld::_instance->_sharedFrameData;
	}

	void ResonanceAudioSource::Update()
	{
		float *newBuffer;
		Update(960.0f/48000.0f, 960, &newBuffer);
		ResonanceAudioWorld::_instance->_audioAPI->SetInterleavedBuffer(_sourceID, newBuffer, 1, 960);
	}

	void ResonanceAudioSource::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		SceneNode::DidUpdate(changeSet);

		if(changeSet & SceneNode::ChangeSet::Position || changeSet & SceneNode::ChangeSet::Attachments)
		{
			RN::Vector3 position = GetWorldPosition();
			RN::Quaternion rotation = GetWorldRotation();
			ResonanceAudioWorld::_instance->_audioAPI->SetSourcePosition(_sourceID, position.x, position.y, position.z);
			ResonanceAudioWorld::_instance->_audioAPI->SetSourceRotation(_sourceID, rotation.x, rotation.y, rotation.z, rotation.w);
		}
	}

}
