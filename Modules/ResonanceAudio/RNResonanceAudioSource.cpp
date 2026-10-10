//
//  RNResonanceAudioSource.cpp
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResonanceAudioSource.h"
#include "RNResonanceAudioSampler.h"
#include "RNResonanceAudioWorld.h"

#include <api/resonance_audio_api.h>

namespace RN
{
	static constexpr uint32 kFadeSamples = 32;

	static vraudio::DistanceRolloffModel MapRolloffModel(ResonanceAudioSource::DistanceRolloffModel model)
	{
		switch(model)
		{
			case ResonanceAudioSource::DistanceRolloffModel::Logarithmic: return vraudio::DistanceRolloffModel::kLogarithmic;
			case ResonanceAudioSource::DistanceRolloffModel::Linear: return vraudio::DistanceRolloffModel::kLinear;
			case ResonanceAudioSource::DistanceRolloffModel::Inverse: return vraudio::DistanceRolloffModel::kInverse;
			case ResonanceAudioSource::DistanceRolloffModel::None: return vraudio::DistanceRolloffModel::kNone;
		}
		return vraudio::DistanceRolloffModel::kLinear;
	}

	RNDefineMeta(ResonanceAudioSource, SceneNode)

	void ResonanceAudioSource::Update(float delta)
	{
		SceneNode::Update(delta);

		auto ResetDoppler = [&] {
			_dopplerPitchMultiplier.store(1.0f, std::memory_order_relaxed);
			_dopplerInitialized = false;
		};

		if(!_isPositional)
			return;

		ResonanceAudioWorld *world = ResonanceAudioWorld::GetInstance();
		if(!world || delta <= 0.0f || !IsPlaying())
		{
			ResetDoppler();
			return;
		}

		ResonanceAudioListenerContext *listenerContext = world->GetListenerContext();
		const ResonanceAudioListenerState listenerState = listenerContext ? listenerContext->GetListenerState() : ResonanceAudioListenerState();
		if(!listenerContext || !listenerState.isValid)
		{
			ResetDoppler();
			return;
		}

		const float dopplerFactor = listenerContext->GetDopplerFactor();
		const float speedOfSound = listenerContext->GetDopplerSpeedOfSound();
		if(dopplerFactor <= 0.0f || speedOfSound <= 0.0f)
		{
			ResetDoppler();
			return;
		}

		const PositionType listenerPosition = listenerState.position;
		const Vector3 listenerVelocity = listenerState.velocity;
		const PositionType sourcePosition = GetWorldPosition();

		if(!_dopplerInitialized)
		{
			_dopplerOldPosition = sourcePosition;
			_dopplerVelocity = Vector3(0.0f, 0.0f, 0.0f);
			_dopplerInitialized = true;
			_dopplerPitchMultiplier.store(1.0f, std::memory_order_relaxed);
			return;
		}

		const float smoothingOld = std::clamp(listenerContext->GetDopplerVelocitySmoothing(), 0.0f, 0.999f);
		Vector3 rawVelocity = Vector3(sourcePosition - _dopplerOldPosition) / delta;
		_dopplerOldPosition = sourcePosition;
		_dopplerVelocity = _dopplerVelocity * smoothingOld + rawVelocity * (1.0f - smoothingOld);

		Vector3 dir(listenerPosition - sourcePosition); // source -> listener
		const float dist = dir.GetLength();
		if(dist <= k::EpsilonFloat)
		{
			_dopplerPitchMultiplier.store(1.0f, std::memory_order_relaxed);
			return;
		}
		dir = dir / dist;

		// Project velocities onto the source->listener direction.
		const float vls = listenerVelocity.GetDotProduct(dir);
		const float vss = _dopplerVelocity.GetDotProduct(dir);

		// OpenAL 1.1-style ratio: (c - factor*v_l) / (c - factor*v_s)
		// Keep strictly positive to avoid negative/NaN pitch factors.
		const float kMinDenom = 0.01f;
		float num = std::max(kMinDenom, speedOfSound - dopplerFactor * vls);
		float den = std::max(kMinDenom, speedOfSound - dopplerFactor * vss);
		float shift = num / den;

		// Prevent extreme pitch explosions.
		shift = std::clamp(shift, 0.125f, 8.0f);
		_dopplerPitchMultiplier.store(shift, std::memory_order_relaxed);
	}

	ResonanceAudioSource::ResonanceAudioSource(AudioAsset *asset, bool wantsIndirectSound, bool isPositional) :
		_isRegisteredInWorld(false),
		_channel(0),
		_sampler(new ResonanceAudioSampler(asset)),
		_sourceID(vraudio::ResonanceAudioApi::kInvalidSourceId),
		_wantsIndirectSound(wantsIndirectSound),
		_isPositional(isPositional),
		_isPlaying(false),
		_isRepeating(false),
		_isSelfdestructing(false),
		_volume(1.0f),
		_pitch(1.0f),
		_dopplerPitchMultiplier(1.0f),
		_dopplerOldPosition(PositionType()),
		_dopplerVelocity(Vector3()),
		_dopplerInitialized(false),
		_minMaxRange(RN::Vector2(0.2f, 200.0f)),
		_rolloffModel(DistanceRolloffModel::Logarithmic),
		_currentTime(0.0f),
		_currentPitch(1.0f),
		_currentVolume(-1.0f),
		_currentSpatialSequence(0),
		_fadeSamples(0),
		_controlBits(0),
		_finalAction(PendingAction::None),
		_nextSeekTime(0.0),
		_nextAsset(nullptr),
		_spatialSequence(0),
		_spatialPositionX(0.0f),
		_spatialPositionY(0.0f),
		_spatialPositionZ(0.0f),
		_pendingSeekTime(-1.0),
		_pendingAsset(nullptr),
		_pendingFinalAction(PendingAction::None),
		_cachedHasAsset(asset != nullptr),
		_cachedTotalTime(asset ? _sampler->GetTotalTime() : 0.0),
		_cachedIsPlaying(false),
		_cachedCurrentTime(0.0)
	{
		// Ring buffers should always be repeating (streaming sources that never "end")
		if(asset && asset->GetType() == AudioAsset::Type::Ringbuffer)
		{
			_isRepeating.store(true, std::memory_order_relaxed);
		}

		RN_ASSERT(ResonanceAudioWorld::_instance, "You need to create a ResonanceAudioWorld before creating audio sources!");

		if(_isPositional)
		{
			_sourceID = ResonanceAudioWorld::_instance->GetAudioAPI()->CreateSoundObjectSource(vraudio::RenderingMode::kBinauralHighQuality);
			ResonanceAudioWorld::_instance->GetAudioAPI()->SetSourceDistanceModel(_sourceID, MapRolloffModel(_rolloffModel), _minMaxRange.x, _minMaxRange.y);
		}
	}

	ResonanceAudioSource::~ResonanceAudioSource()
	{
		AudioAsset *pendingAsset = _pendingAsset.exchange(nullptr, std::memory_order_acq_rel);
		if(pendingAsset != GetNullAssetSentinel()) SafeRelease(pendingAsset);
		SafeRelease(_nextAsset);
		if(_isRegisteredInWorld)
		{
			ResonanceAudioWorld::_instance->RemoveAudioSource(this);
			_isRegisteredInWorld = false;
		}
		if(_isPositional) ResonanceAudioWorld::_instance->GetAudioAPI()->DestroySource(_sourceID);
		_sampler->Release();
	}

	void ResonanceAudioSource::SetAudioAsset(AudioAsset *asset)
	{
		AudioAsset* retained = asset ? SafeRetain(asset) : GetNullAssetSentinel();
		AudioAsset* old = _pendingAsset.exchange(retained, std::memory_order_acq_rel);
		if(old != GetNullAssetSentinel()) SafeRelease(old);
	}

	void ResonanceAudioSource::SetRepeat(bool repeat)
	{
		_isRepeating.store(repeat, std::memory_order_relaxed);
	}

	void ResonanceAudioSource::SetChannel(uint8 channel)
	{
		_channel.store(channel, std::memory_order_relaxed);
	}

	void ResonanceAudioSource::SetCurrentDistanceAttenuationValue(float attentuation)
	{
		if(!_isPositional) return;
		RN_DEBUG_ASSERT(_rolloffModel == DistanceRolloffModel::None, "Distance attenuation value is only supported for none rolloff model");

		ResonanceAudioWorld::_instance->GetAudioAPI()->SetSourceDistanceAttenuation(_sourceID, attentuation);
	}

	void ResonanceAudioSource::SetPitch(float pitch)
	{
		_pitch.store(pitch, std::memory_order_relaxed);
	}

	void ResonanceAudioSource::SetVolume(float volume)
	{
		_volume.store(volume, std::memory_order_relaxed);
	}

	void ResonanceAudioSource::UpdateDistanceModel()
	{
		const float minimumDistance = std::max(0.001f, _minMaxRange.x);
		const float maximumDistanceOrRolloff = _rolloffModel == DistanceRolloffModel::Inverse ?
			std::max(0.0f, _minMaxRange.y) : std::max(minimumDistance, _minMaxRange.y);

		ResonanceAudioWorld::_instance->GetAudioAPI()->SetSourceDistanceModel(
			_sourceID, MapRolloffModel(_rolloffModel), minimumDistance, maximumDistanceOrRolloff);
	}

	void ResonanceAudioSource::SetSpatialPosition(const Vector3 &position)
	{
		_spatialSequence.fetch_add(1, std::memory_order_acq_rel);
		_spatialPositionX.store(position.x, std::memory_order_relaxed);
		_spatialPositionY.store(position.y, std::memory_order_relaxed);
		_spatialPositionZ.store(position.z, std::memory_order_relaxed);
		_spatialSequence.fetch_add(1, std::memory_order_release);
	}

	void ResonanceAudioSource::SetRange(RN::Vector2 minMaxRange)
	{
		if(!_isPositional) return;
		minMaxRange.x = std::max(0.001f, minMaxRange.x);
		minMaxRange.y = std::max(0.0f, minMaxRange.y);
		if(_minMaxRange == minMaxRange) return;

		_minMaxRange = minMaxRange;
		UpdateDistanceModel();
	}

	void ResonanceAudioSource::SetSelfdestruct(bool selfdestruct)
	{
		_isSelfdestructing.store(selfdestruct, std::memory_order_relaxed);
	}

	void ResonanceAudioSource::SetRolloffModel(DistanceRolloffModel rolloffModel)
	{
		if(!_isPositional) return;
		if(_rolloffModel == rolloffModel) return;

		_rolloffModel = rolloffModel;
		UpdateDistanceModel();
	}


	void ResonanceAudioSource::Play()
	{
		_pendingFinalAction.store(PendingAction::Play, std::memory_order_release);
	}

	void ResonanceAudioSource::Stop()
	{
		_pendingSeekTime.store(0.0, std::memory_order_release);
		_pendingFinalAction.store(PendingAction::Stop, std::memory_order_release);
	}

	void ResonanceAudioSource::Pause()
	{
		_pendingFinalAction.store(PendingAction::Pause, std::memory_order_release);
	}

	void ResonanceAudioSource::Seek(double time)
	{
		_pendingSeekTime.store(std::max(time, 0.0), std::memory_order_release);
	}

	bool ResonanceAudioSource::HasEnded() const
	{
		bool cachedHasAsset = _cachedHasAsset.load(std::memory_order_relaxed);
		if(!cachedHasAsset) return true;
		if(_isRepeating.load(std::memory_order_relaxed)) return false;
		double cachedTotalTime = _cachedTotalTime.load(std::memory_order_relaxed);
		double cachedCurrentTime = _cachedCurrentTime.load(std::memory_order_relaxed);
		return (cachedCurrentTime >= cachedTotalTime);
	}

	bool ResonanceAudioSource::Update(double frameLength, uint32 sampleCount, float **outputBuffer, uint8 channelCount)
	{
		ConsumePendingState(); //Only needs to happen once per frame
		ProcessPendingActions(); //Needs to happen once per sample, but also here to actually be able to change assets while playing

		AudioAsset *asset = _sampler->GetAsset();
		if(!asset || !_isPlaying)
		{
			*outputBuffer = nullptr;
			return false;
		}
		
		if(!outputBuffer || !*outputBuffer)
		{
			return false;
		}

		const bool isRingbuffer = asset->GetType() == AudioAsset::Type::Ringbuffer;
		uint32 ringBytes = 0;
		if(isRingbuffer)
		{
			const uint32 bytesPerSecond = asset->GetSampleRate() * asset->GetBytesPerSample();
			ringBytes = std::round(frameLength * bytesPerSecond);
			const uint32 bufferedSize = asset->GetBufferedSize();
			if(bufferedSize < ringBytes)
			{
				*outputBuffer = nullptr;
				return false;
			}
			// Catch up to the newest frame when the producer runs too far ahead.
			if(bufferedSize > ringBytes * 20) asset->PopData(nullptr, bufferedSize - ringBytes);
			// A new source may attach to an already consumed stream. The unread
			// ring position, rather than source lifetime, defines its next sample.
			_currentTime = asset->GetReadPosition() / static_cast<double>(bytesPerSecond);
		}

		double sampleLength = frameLength / static_cast<double>(sampleCount);
		double localTime = _currentTime;
		bool isRepeating = _isRepeating.load(std::memory_order_relaxed);
		uint8 channel = _channel.load(std::memory_order_relaxed);
		float targetPitch = _pitch.load(std::memory_order_relaxed);
		// Do not apply Doppler to ringbuffer-backed sources (typically streaming/voice).
		if(_isPositional && asset && asset->GetType() != AudioAsset::Type::Ringbuffer)
		{
			targetPitch *= _dopplerPitchMultiplier.load(std::memory_order_relaxed);
		}
		float pitch = _currentPitch;
		float pitchStep = sampleCount > 0 ? (targetPitch - pitch) / static_cast<float>(sampleCount) : 0.0f;
		float volume = _isPositional ? 1.0f : _volume.load(std::memory_order_relaxed);
		bool isMonoAsset = _sampler->GetAsset()->GetChannels() == 1;
		
		const bool ringFadeOut = isRingbuffer && _fadeSamples < 0;
		float *targetBuffer = *outputBuffer;

		for(int i = 0; i < sampleCount; i++)
		{
			float gain = ringFadeOut && _fadeSamples == 0 ? 0.0f : volume;

			if(_fadeSamples > 0)
			{
				gain *= 1.0f - (static_cast<float>(_fadeSamples - 1) / static_cast<float>(kFadeSamples));
				_fadeSamples -= 1;
			}
			else if(_fadeSamples < 0)
			{
				gain *= static_cast<float>(-_fadeSamples) / static_cast<float>(kFadeSamples);
				_fadeSamples += 1;
			}

			for(int j = 0; j < channelCount; j++)
			{
				int sampleChannel = isMonoAsset ? 0 : j;
				float value = _isPlaying ? _sampler->GetSample(localTime, sampleChannel + channel, isRepeating) : 0.0f;
				targetBuffer[i * channelCount + j] = value * gain;
			}
			if(_isPlaying) localTime += sampleLength * pitch;
			pitch += pitchStep;

			// Keep the ring asset and its unread bytes owned until sampling completes.
			// Pending stream changes take effect at the next frame boundary.
			if(_fadeSamples == 0 && !isRingbuffer)
			{
				if(ProcessPendingActions())
				{
					localTime = _currentTime;
				}
			}
		}

		if(isRepeating)
		{
			const double totalTime = _sampler->GetTotalTime();
			if(totalTime > 0.0 && localTime >= totalTime)
			{
				localTime = fmod(localTime, totalTime);
			}
		}
		if(isRingbuffer) asset->PopData(nullptr, ringBytes);
		_currentPitch = targetPitch;
		_currentTime = localTime;
		_cachedCurrentTime.store(_currentTime, std::memory_order_relaxed);

		*outputBuffer = targetBuffer;
		return true;
	}

	void ResonanceAudioSource::Update()
	{
		if(_isPositional)
		{
			const uint32 spatialSequence = _spatialSequence.load(std::memory_order_acquire);
			if(!(spatialSequence & 1U) && spatialSequence != _currentSpatialSequence)
			{
				const float positionX = _spatialPositionX.load(std::memory_order_relaxed);
				const float positionY = _spatialPositionY.load(std::memory_order_relaxed);
				const float positionZ = _spatialPositionZ.load(std::memory_order_relaxed);
				if(_spatialSequence.load(std::memory_order_acquire) == spatialSequence)
				{
					ResonanceAudioWorld::_instance->GetAudioAPI()->SetSourcePosition(_sourceID, positionX, positionY, positionZ);
					_currentSpatialSequence = spatialSequence;
				}
			}

			float volume = _volume.load(std::memory_order_relaxed);
			if(volume != _currentVolume)
			{
				ResonanceAudioWorld::_instance->GetAudioAPI()->SetSourceVolume(_sourceID, volume);
				_currentVolume = volume;
			}
		}

		if(!_isPlaying)
		{
			ConsumePendingState();
			ProcessPendingActions();
		}
		
		if(_isPositional && _isPlaying)
		{
			const uint32 frameSize = ResonanceAudioWorld::_instance->_audioSystem->_frameSize;
			const uint32 sampleRate = ResonanceAudioWorld::_instance->_audioSystem->_sampleRate;

			//For none positional sources this happens in the audio handling callback
			ResonanceAudioListenerContext *listenerContext = ResonanceAudioWorld::_instance->GetListenerContext();
			float *newBuffer = listenerContext ? listenerContext->GetSharedFrameData() : nullptr;
			if(newBuffer)
			{
				Update(static_cast<double>(frameSize) / static_cast<double>(sampleRate), frameSize, &newBuffer, 1);
				ResonanceAudioWorld::_instance->GetAudioAPI()->SetInterleavedBuffer(_sourceID, newBuffer, 1, frameSize);
			}
		}

		if(HasEnded())
		{
			_isPlaying = false;
			_cachedIsPlaying.store(false, std::memory_order_relaxed);

			if(_isSelfdestructing.load(std::memory_order_relaxed) && GetSceneInfo() && GetSceneInfo()->GetScene())
			{
				GetSceneInfo()->GetScene()->RemoveNode(const_cast<ResonanceAudioSource *>(this));
			}
		}
	}

	void ResonanceAudioSource::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		SceneNode::DidUpdate(changeSet);
		
		// Register/unregister based on whether the node is actually attached to a scene.
		if(changeSet & (SceneNode::ChangeSet::Parent | SceneNode::ChangeSet::World))
		{
			const bool isInScene = (GetSceneInfo() != nullptr);
			if(isInScene && !_isRegisteredInWorld)
			{
				ResonanceAudioWorld::_instance->AddAudioSource(this);
				_isRegisteredInWorld = true;
			}
			else if(!isInScene && _isRegisteredInWorld)
			{
				ResonanceAudioWorld::_instance->RemoveAudioSource(this);
				_isRegisteredInWorld = false;
			}
		}

	}

	void ResonanceAudioSource::ConsumePendingState()
	{
		// Acquire the action first so preceding asset and seek updates from the
		// same producer are consumed with it.
		PendingAction finalAction = _pendingFinalAction.exchange(PendingAction::None, std::memory_order_acq_rel);

		double pendingSeekTime = _pendingSeekTime.exchange(-1.0, std::memory_order_acq_rel);
		if(pendingSeekTime >= 0.0)
		{
			_nextSeekTime = pendingSeekTime;
			_controlBits |= static_cast<uint32_t>(ControlBits::kWantSeek);
			if(_isPlaying) _controlBits |= static_cast<uint32_t>(ControlBits::kWantFadeOut) | static_cast<uint32_t>(ControlBits::kWantFadeIn);
		}

		AudioAsset *pendingAsset = _pendingAsset.exchange(nullptr, std::memory_order_acq_rel);
		if(pendingAsset)
		{
			AudioAsset *nextAsset = pendingAsset == GetNullAssetSentinel() ? nullptr : pendingAsset;
			SafeRelease(_nextAsset);
			_nextAsset = nextAsset;
			_controlBits |= static_cast<uint32_t>(ControlBits::kWantAssetChange);
			if(_isPlaying && _sampler->GetAsset() != nextAsset)
				_controlBits |= static_cast<uint32_t>(ControlBits::kWantFadeOut) | static_cast<uint32_t>(ControlBits::kWantFadeIn);
		}

		switch(finalAction)
		{
			case PendingAction::Stop:
			case PendingAction::Pause:
				if(_isPlaying)
				{
					_finalAction = finalAction;
					_controlBits |= static_cast<uint32_t>(ControlBits::kWantFadeOut);
				}
				_controlBits &= ~static_cast<uint32_t>(ControlBits::kWantFadeIn);
				break;

			case PendingAction::Play:
				_finalAction = finalAction;
				_controlBits |= static_cast<uint32_t>(ControlBits::kWantFadeIn);
				break;

			default:
				break;
		}
	}

	bool ResonanceAudioSource::ProcessPendingActions()
	{
		// Now process the control bits as before
		bool isSeeking = false;

		if(_controlBits & static_cast<uint32_t>(ControlBits::kWantFadeOut) && _isPlaying)
		{
			_fadeSamples = -static_cast<int32>(kFadeSamples);
			_controlBits &= ~static_cast<uint32_t>(ControlBits::kWantFadeOut);
			return isSeeking;
		}

		if(_controlBits & static_cast<uint32_t>(ControlBits::kWantAssetChange))
		{
			AudioAsset *pendingAsset = _nextAsset;
			_nextAsset = nullptr;
			if(pendingAsset != _sampler->GetAsset())
			{
				_sampler->SetAudioAsset(pendingAsset);
				// Update cached values after asset change
				_cachedHasAsset.store(_sampler->GetAsset() != nullptr, std::memory_order_relaxed);
				_cachedTotalTime.store(_sampler->GetTotalTime(), std::memory_order_relaxed);

				// Ring buffers should always be repeating (streaming sources that never "end")
				if(pendingAsset && pendingAsset->GetType() == AudioAsset::Type::Ringbuffer)
				{
					_isRepeating.store(true, std::memory_order_relaxed);
				}

				_currentTime = 0.0;
				_cachedCurrentTime.store(0.0, std::memory_order_relaxed);
				isSeeking = true;
			}
			SafeRelease(pendingAsset);
			_controlBits &= ~static_cast<uint32_t>(ControlBits::kWantAssetChange);
		}

		if(_controlBits & static_cast<uint32_t>(ControlBits::kWantSeek))
		{
			_currentTime = _nextSeekTime;
			_cachedCurrentTime.store(_currentTime, std::memory_order_relaxed);
			isSeeking = true;
			_controlBits &= ~static_cast<uint32_t>(ControlBits::kWantSeek);
		}

		switch(_finalAction)
		{
			case PendingAction::Stop:
			case PendingAction::Pause:
				_isPlaying = false;
				_cachedIsPlaying.store(false, std::memory_order_relaxed);
				break;

			case PendingAction::Play:
				if(_sampler->GetAsset() && _currentTime >= _sampler->GetTotalTime())
				{
					_currentTime = 0.0f;
					_cachedCurrentTime.store(0.0, std::memory_order_relaxed);
				}
				_isPlaying = true;
				_cachedIsPlaying.store(true, std::memory_order_relaxed);
				break;

			default:
				break;
		}
		_finalAction = PendingAction::None;

		if(_controlBits & static_cast<uint32_t>(ControlBits::kWantFadeIn))
		{
			if(_isPlaying)
			{
				_fadeSamples = static_cast<int32>(kFadeSamples);
			}

			_controlBits &= ~static_cast<uint32_t>(ControlBits::kWantFadeIn);
		}

		return isSeeking;
	}
} // namespace RN
