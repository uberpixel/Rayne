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

	static inline vraudio::DistanceRolloffModel MapRolloffModel(ResonanceAudioSource::DistanceRolloffModel model)
	{
		switch(model)
		{
			case ResonanceAudioSource::DistanceRolloffModel::Logarithmic:
				return vraudio::DistanceRolloffModel::kLogarithmic;
			case ResonanceAudioSource::DistanceRolloffModel::Linear:
				return vraudio::DistanceRolloffModel::kLinear;
			case ResonanceAudioSource::DistanceRolloffModel::Inverse:
				return vraudio::DistanceRolloffModel::kInverse;
			case ResonanceAudioSource::DistanceRolloffModel::None:
				return vraudio::DistanceRolloffModel::kNone;
		}
		return vraudio::DistanceRolloffModel::kLinear;
	}

	RNDefineMeta(ResonanceAudioSource, SceneNode)

	void ResonanceAudioSource::Update(float delta)
	{
		SceneNode::Update(delta);

		if(std::isfinite(delta) && delta > 0.0f) _ringDiagnosticsElapsed += delta;
		if(_ringDiagnosticsElapsed >= 5.0f)
		{
			const uint32 underruns = _ringUnderruns.exchange(0, std::memory_order_relaxed);
			if(underruns) RNInfo("Resonance audio ring source=" << static_cast<const void *>(this) << " underruns=" << underruns << " over " << _ringDiagnosticsElapsed << " s");
			_ringDiagnosticsElapsed = 0.0f;
		}

		const uint64 request = _selfdestructRequest.exchange(0, std::memory_order_acq_rel);
		if(request && request == _playbackRevision.load(std::memory_order_acquire) &&
		   _isSelfdestructing.load(std::memory_order_relaxed) && HasEnded() && !IsPlaying())
		{
			if(GetSceneInfo() && GetSceneInfo()->GetScene())
			{
				GetSceneInfo()->GetScene()->RemoveNode(this);
				return;
			}
		}
	}

	void ResonanceAudioSource::UpdateDopplerVelocity(const PositionType &position, float delta, float smoothing)
	{
		if(delta <= 0.0f) return;
		if(_dopplerInitialized)
		{
			const Vector3 velocity = Vector3(position - _dopplerOldPosition) / delta;
			_dopplerVelocity = _dopplerVelocity * smoothing + velocity * (1.0f - smoothing);
		}
		_dopplerOldPosition = position;
		_dopplerInitialized = true;
	}

	float ResonanceAudioSource::GetDopplerPitch(const ResonanceAudioListenerContext &context, const Vector3 &offset, const Vector3 &listenerVelocity) const
	{
		const float factor = context.GetDopplerFactor();
		if(factor <= 0.0f || !_dopplerInitialized) return 1.0f;
		const float distance = offset.GetLength();
		if(distance <= k::EpsilonFloat) return 1.0f;
		const Vector3 direction = -offset / distance;
		const float speed = context.GetDopplerSpeedOfSound();
		const float numerator = std::max(0.01f, speed - factor * listenerVelocity.GetDotProduct(direction));
		const float denominator = std::max(0.01f, speed - factor * _dopplerVelocity.GetDotProduct(direction));
		return std::clamp(numerator / denominator, 0.125f, 8.0f);
	}

	ResonanceAudioSource::ResonanceAudioSource(AudioAsset *asset, bool wantsIndirectSound, bool isPositional) :
		_world(nullptr),
		_isRegisteredInWorld(false),
		_channel(0),
		_sampler(new ResonanceAudioSampler(asset)),
		_sourceID(vraudio::ResonanceAudioApi::kInvalidSourceId),
		_recordingSourceID(vraudio::ResonanceAudioApi::kInvalidSourceId),
		_wantsIndirectSound(wantsIndirectSound),
		_isPositional(isPositional),
		_isPlaying(false),
		_isRepeating(false),
		_isSelfdestructing(false),
		_volume(1.0f),
		_pitch(1.0f),
		_dopplerOldPosition(PositionType()),
		_dopplerVelocity(Vector3()),
		_dopplerInitialized(false),
		_minMaxRange(RN::Vector2(0.2f, 200.0f)),
		_rolloffModel(DistanceRolloffModel::Logarithmic),
		_currentTime(0.0f),
		_currentPitch(1.0f),
		_currentVolume(-1.0f),
		_fadeSamples(0),
		_controlBits(0),
		_finalAction(PendingAction::None),
		_nextSeekTime(0.0),
		_nextAsset(nullptr),
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

		LockGuard<RecursiveLockable> lock(ResonanceAudioWorld::_sourceLifetimeLock);
		_world = ResonanceAudioWorld::_instance;
		RN_ASSERT(_world, "You need to create a ResonanceAudioWorld before creating audio sources!");

		if(!_world) return;
		_world->_allAudioSources.push_back(this);
		if(_isPositional)
		{
			_sourceID = _world->GetAudioAPI()->CreateSoundObjectSource(vraudio::RenderingMode::kBinauralHighQuality);
			UpdateDistanceModel();
			if(_world->_recordingContext) CreateRecordingSource(_world->_recordingContext);
		}
	}

	ResonanceAudioSource::~ResonanceAudioSource()
	{
		LockGuard<RecursiveLockable> lock(ResonanceAudioWorld::_sourceLifetimeLock);
		AudioAsset *pendingAsset = _pendingAsset.exchange(nullptr, std::memory_order_acq_rel);
		if(pendingAsset != GetNullAssetSentinel()) SafeRelease(pendingAsset);
		SafeRelease(_nextAsset);
		ResonanceAudioWorld *world = _world;
		if(world)
		{
			if(_isRegisteredInWorld) world->RemoveAudioSource(this);
			auto &sources = world->_allAudioSources;
			auto iterator = std::find(sources.begin(), sources.end(), this);
			if(iterator != sources.end()) sources.erase(iterator);
			if(_isPositional)
			{
				vraudio::ResonanceAudioApi *audioAPI = world->GetAudioAPI();
				if(audioAPI) audioAPI->DestroySource(_sourceID);
				if(world->_recordingContext) RemoveRecordingSource(world->_recordingContext);
			}
		}
		ReleaseRetiredAudioAssets();
		_sampler->Release();
	}

	void ResonanceAudioSource::SetAudioAsset(AudioAsset *asset)
	{
		AudioAsset* retained = asset ? SafeRetain(asset) : GetNullAssetSentinel();
		AudioAsset* old = _pendingAsset.exchange(retained, std::memory_order_acq_rel);
		_playbackRevision.fetch_add(1, std::memory_order_acq_rel);
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

	void ResonanceAudioSource::SetCurrentDistanceAttenuationValue(float attenuation)
	{
		LockGuard<RecursiveLockable> lock(ResonanceAudioWorld::_sourceLifetimeLock);
		if(!_isPositional || !_world || !_world->GetAudioAPI()) return;
		RN_DEBUG_ASSERT(_rolloffModel == DistanceRolloffModel::None, "Distance attenuation value is only supported for none rolloff model");

		_manualDistanceAttenuation = attenuation;
		_world->GetAudioAPI()->SetSourceDistanceAttenuation(_sourceID, attenuation);
		if(_world->_recordingContext)
		{
			_world->_recordingContext->GetAudioAPI()->SetSourceDistanceAttenuation(_recordingSourceID, attenuation);
		}
	}

	void ResonanceAudioSource::SetPitch(float pitch)
	{
		if(!std::isfinite(pitch)) return;
		_pitch.store(pitch, std::memory_order_relaxed);
	}

	void ResonanceAudioSource::SetVolume(float volume)
	{
		if(!std::isfinite(volume)) return;
		_volume.store(volume, std::memory_order_relaxed);
	}

	void ResonanceAudioSource::SetRecordingSpatialVolume(float volume)
	{
		if(!_isPositional || !std::isfinite(volume)) return;
		_recordingSpatialVolume.store(volume, std::memory_order_relaxed);
	}

	float ResonanceAudioSource::GetRecordingSpatialVolume(float sharedVolume) const
	{
		const float volume = _recordingSpatialVolume.load(std::memory_order_relaxed);
		return volume < 0.0f ? sharedVolume : volume;
	}

	void ResonanceAudioSource::UpdateDistanceModel()
	{
		auto *context = _world ? _world->GetListenerContext() : nullptr;
		if(!context) return;

		ApplyDistanceModel(*context, _sourceID);
		if(_recordingSourceID != vraudio::ResonanceAudioApi::kInvalidSourceId)
		{
			ApplyDistanceModel(*_world->_recordingContext, _recordingSourceID);
		}
	}

	void ResonanceAudioSource::ApplyDistanceModel(ResonanceAudioListenerContext &context, int sourceID) const
	{
		const float minimumDistance = std::max(0.001f, _minMaxRange.x);
		const float maximumDistanceOrRolloff = _rolloffModel == DistanceRolloffModel::Inverse ? std::max(0.0f, _minMaxRange.y) : std::max(minimumDistance, _minMaxRange.y);
		context.GetAudioAPI()->SetSourceDistanceModel(sourceID, MapRolloffModel(_rolloffModel), minimumDistance, maximumDistanceOrRolloff);
	}

	void ResonanceAudioSource::CreateRecordingSource(ResonanceAudioListenerContext *context)
	{
		if(!_isPositional) return;
		auto *api = context->GetAudioAPI();
		_recordingSourceID = api->CreateSoundObjectSource(vraudio::RenderingMode::kBinauralHighQuality);
		ApplyDistanceModel(*context, _recordingSourceID);
		if(_rolloffModel == DistanceRolloffModel::None) api->SetSourceDistanceAttenuation(_recordingSourceID, _manualDistanceAttenuation);
		_recordingVolume = GetRecordingSpatialVolume(GetVolume());
		api->SetSourceVolume(_recordingSourceID, _recordingVolume);
		_recordingPlayback = {};
		_recordingRoomWasEnabled = false;
	}

	void ResonanceAudioSource::RemoveRecordingSource(ResonanceAudioListenerContext *context)
	{
		if(_recordingSourceID == vraudio::ResonanceAudioApi::kInvalidSourceId) return;
		if(context) context->GetAudioAPI()->DestroySource(_recordingSourceID);
		_recordingSourceID = vraudio::ResonanceAudioApi::kInvalidSourceId;
		_cachedIsRecordingPlaying.store(false, std::memory_order_relaxed);
	}

	void ResonanceAudioSource::SetRange(RN::Vector2 minMaxRange)
	{
		if(!_isPositional) return;
		LockGuard<RecursiveLockable> lock(ResonanceAudioWorld::_sourceLifetimeLock);
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
		LockGuard<RecursiveLockable> lock(ResonanceAudioWorld::_sourceLifetimeLock);
		if(_rolloffModel == rolloffModel) return;

		_rolloffModel = rolloffModel;
		UpdateDistanceModel();
	}


	void ResonanceAudioSource::Play()
	{
		_pendingFinalAction.store(PendingAction::Play, std::memory_order_release);
		_playbackRevision.fetch_add(1, std::memory_order_acq_rel);
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
		if(!std::isfinite(time)) return;
		_pendingSeekTime.store(std::max(time, 0.0), std::memory_order_release);
		_playbackRevision.fetch_add(1, std::memory_order_acq_rel);
	}

	bool ResonanceAudioSource::HasEnded() const
	{
		bool cachedHasAsset = _cachedHasAsset.load(std::memory_order_relaxed);
		if(!cachedHasAsset) return true;
		if(_isRepeating.load(std::memory_order_relaxed)) return false;
		double cachedTotalTime = _cachedTotalTime.load(std::memory_order_relaxed);
		double cachedCurrentTime = _cachedCurrentTime.load(std::memory_order_acquire);
		return (cachedCurrentTime >= cachedTotalTime);
	}

	bool ResonanceAudioSource::Update(double frameLength, uint32 sampleCount, float **outputBuffer, uint8 channelCount)
	{
		if(!outputBuffer || !*outputBuffer) return false;
		if(RenderSamples(frameLength, sampleCount, *outputBuffer, channelCount).primary) return true;
		*outputBuffer = nullptr;
		return false;
	}

	ResonanceAudioSource::RenderedSamples ResonanceAudioSource::RenderSamples(double frameLength, uint32 sampleCount, float *output, uint8 channelCount, float primaryDoppler, float *recordingOutput, float recordingDoppler)
	{
		RecordingPlayback *recording = recordingOutput ? &_recordingPlayback : nullptr;
		if(recording && !recording->initialized)
		{
			*recording = {_currentTime, _currentPitch, true, _isPlaying};
		}

		ConsumePendingState(recording && recording->playing);
		ProcessPendingActions(recording);

		AudioAsset *asset = _sampler->GetAsset();
		RenderedSamples rendered {_isPlaying, recording && recording->playing};
		if(!asset || !(rendered.primary || rendered.recording) || sampleCount == 0)
		{
			if(recording) _cachedIsRecordingPlaying.store(asset && recording->playing, std::memory_order_relaxed);
			return {};
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
				_ringUnderruns.fetch_add(1, std::memory_order_relaxed);
				if(_fadeSamples < 0)
				{
					// An empty stream has no PCM to fade before stopping or swapping.
					_fadeSamples = 0;
					ProcessPendingActions(recording);
				}
				if(recording) _cachedIsRecordingPlaying.store(recording->playing, std::memory_order_relaxed);
				return {};
			}
			// Catch up to the newest frame when the producer runs too far ahead.
			if(bufferedSize > ringBytes * 20) asset->PopData(nullptr, bufferedSize - ringBytes);
			_currentTime = asset->GetReadPosition() / static_cast<double>(bytesPerSecond);
		}

		const double sampleLength = frameLength / static_cast<double>(sampleCount);
		bool isRepeating = _isRepeating.load(std::memory_order_relaxed);
		const uint8 channel = _channel.load(std::memory_order_relaxed);
		const float sourcePitch = _pitch.load(std::memory_order_relaxed);
		float targetPitch = sourcePitch * (isRingbuffer ? 1.0f : primaryDoppler);
		if(!std::isfinite(targetPitch)) targetPitch = 1.0f;
		float pitch = _currentPitch;
		const float pitchStep = (targetPitch - pitch) / static_cast<float>(sampleCount);
		float recordingPitch = recording ? recording->pitch : 1.0f;
		float recordingTargetPitch = recording ? sourcePitch * recordingDoppler : 1.0f;
		if(!std::isfinite(recordingTargetPitch)) recordingTargetPitch = 1.0f;
		const float recordingPitchStep = (recordingTargetPitch - recordingPitch) / static_cast<float>(sampleCount);
		const float volume = _isPositional ? 1.0f : _volume.load(std::memory_order_relaxed);
		bool isMonoAsset = asset->GetChannels() == 1;

		for(uint32 i = 0; i < sampleCount; i++)
		{
			const bool hasPendingAction = _finalAction != PendingAction::None || _controlBits;
			const bool waitingForRing = (_controlBits & static_cast<uint32_t>(ControlBits::kWantAssetChange)) && (_nextAsset && _nextAsset->GetType() == AudioAsset::Type::Ringbuffer);
			float gain = volume;
			if(_fadeSamples == 0 && ((isRingbuffer && hasPendingAction) || waitingForRing)) gain = 0.0f;
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

			for(uint32 j = 0; j < channelCount; j++)
			{
				const uint8 sampleChannel = (isMonoAsset ? 0 : j) + channel;
				const float sample = (_isPlaying ? _sampler->GetSample(_currentTime, sampleChannel, isRepeating) : 0.0f) * gain;
				// Keep corrupt stream samples out of Resonance's persistent filters.
				output[i * channelCount + j] = std::isfinite(sample) ? sample : 0.0f;
			}
			if(recording)
			{
				if(isRingbuffer)
				{
					recordingOutput[i] = output[i];
				}
				else
				{
					const float sample = (recording->playing ? _sampler->GetSample(recording->time, channel, isRepeating) : 0.0f) * gain;
					recordingOutput[i] = std::isfinite(sample) ? sample : 0.0f;
					if(recording->playing)
					{
						rendered.recording = true;
						recording->time += sampleLength * recordingPitch;
					}
					recordingPitch += recordingPitchStep;
				}
			}
			if(_isPlaying) _currentTime += sampleLength * pitch;
			pitch += pitchStep;

			// The ring asset and unread bytes stay owned until the single pop.
			if(_fadeSamples == 0 && !isRingbuffer && !waitingForRing && hasPendingAction)
			{
				ProcessPendingActions(recording, false);
				asset = _sampler->GetAsset();
				isMonoAsset = !asset || asset->GetChannels() == 1;
				isRepeating = _isRepeating.load(std::memory_order_relaxed);
			}
		}

		const double totalTime = _sampler->GetTotalTime();
		if(isRepeating && totalTime > 0.0 && _currentTime >= totalTime) _currentTime = fmod(_currentTime, totalTime);
		if(isRingbuffer) asset->PopData(nullptr, ringBytes);
		if(recording)
		{
			if(isRingbuffer)
			{
				recording->time = _currentTime;
				recording->pitch = targetPitch;
			}
			else
			{
				recording->pitch = recordingTargetPitch;
				if(isRepeating && totalTime > 0.0 && recording->time >= totalTime) recording->time = fmod(recording->time, totalTime);
				if(!isRepeating && recording->time >= totalTime) recording->playing = false;
			}
			_cachedIsRecordingPlaying.store(recording->playing, std::memory_order_relaxed);
		}

		if(rendered.primary) _currentPitch = targetPitch;
		// Make the recording playback state visible before HasEnded can report the primary cursor's end.
		_cachedCurrentTime.store(_currentTime, std::memory_order_release);
		return rendered;
	}

	void ResonanceAudioSource::ApplySpatialState(ResonanceAudioListenerContext &context, int sourceID, const SpatialState &state, bool &roomWasEnabled)
	{
		if(!state.valid) return;
		auto *api = context.GetAudioAPI();
		api->SetSourcePosition(sourceID, state.position.x, state.position.y, state.position.z);
		if(state.roomEnabled || roomWasEnabled)
		{
			api->SetSourceRoomEffectsGain(sourceID, state.roomGain);
			api->SetSoundObjectOcclusionIntensity(sourceID, state.occlusion);
		}
		roomWasEnabled = state.roomEnabled;
	}

	void ResonanceAudioSource::RenderPositional(ResonanceAudioListenerContext &primary, ResonanceAudioListenerContext *recording, const SpatialState &primaryState, const SpatialState *recordingState, bool primarySpatialChanged, bool recordingSpatialChanged, double frameLength, uint32 sampleCount)
	{
		const uint64 playbackRevision = _playbackRevision.load(std::memory_order_acquire);
		const float volume = _volume.load(std::memory_order_relaxed);
		auto *primaryAPI = primary.GetAudioAPI();
		if(primarySpatialChanged) ApplySpatialState(primary, _sourceID, primaryState, _primaryRoomWasEnabled);
		if(volume != _currentVolume)
		{
			primaryAPI->SetSourceVolume(_sourceID, volume);
			_currentVolume = volume;
		}

		if(recording)
		{
			if(recordingSpatialChanged) ApplySpatialState(*recording, _recordingSourceID, *recordingState, _recordingRoomWasEnabled);
			const float recordingVolume = GetRecordingSpatialVolume(volume);
			if(recordingVolume != _recordingVolume)
			{
				recording->GetAudioAPI()->SetSourceVolume(_recordingSourceID, recordingVolume);
				_recordingVolume = recordingVolume;
			}
		}

		float *primaryPCM = primary.GetSharedFrameData();
		float *recordingPCM = recording ? recording->GetSharedFrameData() : nullptr;
		const RenderedSamples rendered = RenderSamples(frameLength, sampleCount, primaryPCM, 1, primaryState.doppler, recordingPCM, recordingState ? recordingState->doppler : 1.0f);
		if(rendered.primary) primaryAPI->SetInterleavedBuffer(_sourceID, primaryPCM, 1, sampleCount);
		if(rendered.recording) recording->GetAudioAPI()->SetInterleavedBuffer(_recordingSourceID, recordingPCM, 1, sampleCount);
		FinishRender(playbackRevision, recording != nullptr);
	}

	void ResonanceAudioSource::FinishRender(uint64 playbackRevision, bool recording)
	{
		if(!HasEnded()) return;
		_isPlaying = false;
		_cachedIsPlaying.store(false, std::memory_order_relaxed);

		// Public playback state follows the primary cursor. A slower recording
		// cursor can still finish the one-shot before automatic node removal.
		if(recording && _recordingPlayback.playing) return;
		if(_isSelfdestructing.load(std::memory_order_relaxed) && !_controlBits &&
		   !_pendingAsset.load(std::memory_order_acquire) &&
		   _pendingFinalAction.load(std::memory_order_acquire) != PendingAction::Play &&
		   _pendingSeekTime.load(std::memory_order_acquire) < 0.0)
		{
			_selfdestructRequest.store(playbackRevision, std::memory_order_release);
		}
	}

	void ResonanceAudioSource::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		LockGuard<RecursiveLockable> lock(ResonanceAudioWorld::_sourceLifetimeLock);
		SceneNode::DidUpdate(changeSet);
		
		// Register/unregister based on whether the node is actually attached to a scene.
		if(_world && (changeSet & (SceneNode::ChangeSet::Parent | SceneNode::ChangeSet::World)))
		{
			const bool isInScene = (GetSceneInfo() != nullptr);
			if(isInScene && !_isRegisteredInWorld)
			{
				_world->AddAudioSource(this);
				_isRegisteredInWorld = true;
			}
			else if(!isInScene && _isRegisteredInWorld)
			{
				_world->RemoveAudioSource(this);
				_isRegisteredInWorld = false;
			}
		}

	}

	void ResonanceAudioSource::ConsumePendingState(bool recordingPlaying)
	{
		if(_pendingFinalAction.load(std::memory_order_relaxed) == PendingAction::None &&
		   _pendingSeekTime.load(std::memory_order_relaxed) < 0.0 &&
		   !_pendingAsset.load(std::memory_order_relaxed)) return;

		// Keep a slot for the swap through its fade. Replacing a staged asset can use another.
		const uint32 slotsNeeded = _nextAsset ? 2 : 1;
		const uint32 write = _retiredAudioAssetWrite.load(std::memory_order_relaxed);
		const uint32 read = _retiredAudioAssetRead.load(std::memory_order_acquire);
		if(write - read + slotsNeeded > kAudioAssetRetirementCapacity) return;

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
			RetireAudioAsset(std::exchange(_nextAsset, nextAsset));
			_controlBits |= static_cast<uint32_t>(ControlBits::kWantAssetChange);
			if(_isPlaying && _sampler->GetAsset() != nextAsset)
				_controlBits |= static_cast<uint32_t>(ControlBits::kWantFadeOut) | static_cast<uint32_t>(ControlBits::kWantFadeIn);
		}

		switch(finalAction)
		{
			case PendingAction::Stop:
			case PendingAction::Pause:
				if(_isPlaying || recordingPlaying)
				{
					_finalAction = finalAction;
					if(_isPlaying) _controlBits |= static_cast<uint32_t>(ControlBits::kWantFadeOut);
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

	void ResonanceAudioSource::ProcessPendingActions(RecordingPlayback *recording, bool blockBoundary)
	{
		bool isSeeking = false;

		if(_controlBits & static_cast<uint32_t>(ControlBits::kWantFadeOut))
		{
			_controlBits &= ~static_cast<uint32_t>(ControlBits::kWantFadeOut);
			if(_isPlaying)
			{
				_fadeSamples = -static_cast<int32>(kFadeSamples);
				return;
			}
		}

		if(_fadeSamples < 0)
		{
			if(_isPlaying) return;
			_fadeSamples = 0;
		}
		if(_controlBits & static_cast<uint32_t>(ControlBits::kWantAssetChange))
		{
			if(!blockBoundary && _nextAsset && _nextAsset->GetType() == AudioAsset::Type::Ringbuffer) return;
			AudioAsset *pendingAsset = std::exchange(_nextAsset, nullptr);
			AudioAsset *retiredAsset = pendingAsset;
			if(pendingAsset != _sampler->GetAsset())
			{
				retiredAsset = _sampler->ExchangeRetainedAsset(pendingAsset);
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
			RetireAudioAsset(retiredAsset);
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
				if(recording) recording->playing = false;
				break;

			case PendingAction::Play:
				if(_sampler->GetAsset() && _currentTime >= _sampler->GetTotalTime())
				{
					_currentTime = 0.0f;
					isSeeking = true;
					_cachedCurrentTime.store(0.0, std::memory_order_relaxed);
				}
				_isPlaying = true;
				if(recording)
				{
					// Resume at this listener's cursor. A one-shot that already
					// finished here restarts only when the shared transport does.
					recording->playing = !_sampler->GetAsset() || IsRepeating() || recording->time < _sampler->GetTotalTime();
				}
				_cachedIsPlaying.store(true, std::memory_order_relaxed);
				break;

			default:
				break;
		}
		_finalAction = PendingAction::None;
		if(recording && isSeeking)
		{
			recording->time = _currentTime;
			recording->playing = _isPlaying;
		}

		if(_controlBits & static_cast<uint32_t>(ControlBits::kWantFadeIn))
		{
			if(_isPlaying)
			{
				_fadeSamples = static_cast<int32>(kFadeSamples);
			}

			_controlBits &= ~static_cast<uint32_t>(ControlBits::kWantFadeIn);
		}
	}

	void ResonanceAudioSource::RetireAudioAsset(AudioAsset *asset)
	{
		if(!asset) return;
		if(!_world)
		{
			// The output device has stopped before the world detaches this source.
			asset->Release();
			return;
		}

		const uint32 write = _retiredAudioAssetWrite.load(std::memory_order_relaxed);
		_retiredAudioAssets[write % kAudioAssetRetirementCapacity] = asset;
		_retiredAudioAssetWrite.store(write + 1, std::memory_order_release);
		_world->_hasRetiredAudioAssets.store(true, std::memory_order_release);
	}

	void ResonanceAudioSource::ReleaseRetiredAudioAssets()
	{
		const uint32 write = _retiredAudioAssetWrite.load(std::memory_order_acquire);
		uint32 read = _retiredAudioAssetRead.load(std::memory_order_relaxed);
		while(read != write)
		{
			AudioAsset *asset = _retiredAudioAssets[read % kAudioAssetRetirementCapacity];
			_retiredAudioAssetRead.store(++read, std::memory_order_release);
			asset->Release();
		}
	}
} // namespace RN
