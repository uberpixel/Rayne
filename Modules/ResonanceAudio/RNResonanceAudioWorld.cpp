//
//  RNResonanceAudioWorld.cpp
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResonanceAudioWorld.h"

#include <api/resonance_audio_api.h>
#include <platforms/common/room_effects_utils.h>

#if RN_PLATFORM_MAC_OS
	#include <AVFoundation/AVFoundation.h>
#endif

namespace RN
{
	RNDefineMeta(ResonanceAudioWorld, SceneAttachment)

	RecursiveLockable ResonanceAudioWorld::_sourceLifetimeLock;
	ResonanceAudioWorld *ResonanceAudioWorld::_instance = nullptr;

	ResonanceAudioWorld *ResonanceAudioWorld::GetInstance()
	{
		LockGuard<RecursiveLockable> lock(_sourceLifetimeLock);
		return _instance;
	}

	//TODO: Allow to initialize with preferred device names and fall back to defaults
	ResonanceAudioWorld::ResonanceAudioWorld(ResonanceAudioSystem *audioSystem) :
		_audioSystem(audioSystem),
		_worldMasterVolume(1.0f)
	{
		LockGuard<RecursiveLockable> lock(_sourceLifetimeLock);
		RN_ASSERT(!_instance, "There already is a ResonanceAudioWorld!");
		RN_ASSERT(_audioSystem, "Audio system needs to be provided when creating an audio world!");

		for(auto &snapshot : _audioSourcesSnapshots) snapshot.sources = new Array();

		_audioSystem->CreateListenerContext(this);
		_instance = this;
		_audioSystem->Retain();
	}

	ResonanceAudioWorld::~ResonanceAudioWorld()
	{
		{
			LockGuard<RecursiveLockable> lock(_sourceLifetimeLock);
			if(_instance == this) _instance = nullptr;
		}

		// Stop callbacks before releasing sources or their audio API.
		_audioSystem->SetOutputDevice(nullptr);
		_audioSystem->SetInputDevice(nullptr);

		StopRecordingListener();
		LockGuard<RecursiveLockable> lock(_sourceLifetimeLock);

		for(auto &snapshot : _audioSourcesSnapshots) SafeRelease(snapshot.sources);

		// Sources can outlive this attachment, including sources never added to a scene.
		vraudio::ResonanceAudioApi *audioAPI = GetAudioAPI();
		for(ResonanceAudioSource *source : _allAudioSources)
		{
			source->ReleaseRetiredAudioAssets();
			if(audioAPI && source->_isPositional) audioAPI->DestroySource(source->_sourceID);
			source->_sourceID = vraudio::ResonanceAudioApi::kInvalidSourceId;
			source->_world = nullptr;
			source->_isRegisteredInWorld = false;
			source->_isPlaying = false;
			source->_cachedIsPlaying.store(false, std::memory_order_relaxed);
			source->_selfdestructRequest.store(0, std::memory_order_relaxed);
		}

		_audioSystem->RemoveAllListenerContexts();
		SafeRelease(_audioSystem);
	}

	void ResonanceAudioWorld::AddAudioSource(ResonanceAudioSource *source)
	{
		LockGuard<RecursiveLockable> lock(_sourceLifetimeLock);
		_audioSources.push_back(source);
		++_audioSourcesRevision;
	}

	void ResonanceAudioWorld::RemoveAudioSource(ResonanceAudioSource *source)
	{
		LockGuard<RecursiveLockable> lock(_sourceLifetimeLock);
		auto iterator = std::find(_audioSources.begin(), _audioSources.end(), source);
		if(iterator != _audioSources.end())
		{
			_audioSources.erase(iterator);
			++_audioSourcesRevision;
		}
	}

	void ResonanceAudioWorld::PublishAudioSourcesSnapshot(float delta)
	{
		const uint32 writeIndex = _audioSourcesSnapshotBack;
		AudioSourcesSnapshot &snapshot = _audioSourcesSnapshots[writeIndex];
		if(snapshot.sourcesRevision != _audioSourcesRevision)
		{
			snapshot.sources->RemoveAllObjects();
			for(ResonanceAudioSource *source : _audioSources) snapshot.sources->AddObject(source);
			snapshot.sourcesRevision = _audioSourcesRevision;
		}

		ResonanceAudioListenerContext *contexts[] = {GetListenerContext(), _recordingContext};
		const size_t listenerCount = _recordingContext ? 2 : 1;
		ResonanceAudioListenerState listeners[2];
		for(size_t i = 0; i < listenerCount; i++)
		{
			listeners[i] = contexts[i]->_listenerState;
			snapshot.rotation[i] = listeners[i].rotation;
			snapshot.sourceState[i].resize(_audioSources.size());
		}

		for(size_t index = 0; index < _audioSources.size(); index++)
		{
			auto *source = _audioSources[index];
			if(!source->IsPositional()) continue;
			const PositionType sourcePosition = source->GetWorldPosition();
			source->UpdateDopplerVelocity(sourcePosition, delta, contexts[0]->GetDopplerVelocitySmoothing());
			for(size_t i = 0; i < listenerCount; i++)
			{
				ResonanceAudioSource::SpatialState &state = snapshot.sourceState[i][index];
				auto &context = *contexts[i];
				auto &listener = listeners[i];
				state = {};
				if(!listener.isValid) continue;
				state.valid = true;
				state.position = Vector3(sourcePosition - listener.position);
				state.doppler = source->GetDopplerPitch(context, state.position, listener.velocity);
				state.roomEnabled = context._roomEnabled;
				if(!state.roomEnabled) continue;

				const Vector3 roomPosition(context._roomPosition - listener.position);
				state.roomGain = vraudio::ComputeRoomEffectsGain(
				vraudio::WorldPosition(state.position.x, state.position.y, state.position.z),
				vraudio::WorldPosition(roomPosition.x, roomPosition.y, roomPosition.z),
				vraudio::WorldRotation(1.0f, 0.0f, 0.0f, 0.0f),
				vraudio::WorldPosition(context._roomDimensions.x, context._roomDimensions.y, context._roomDimensions.z));
				if(_raycastCallback)
				{
					float distance = -1.0f;
					_raycastCallback(sourcePosition, -state.position, distance);
					state.occlusion = (distance > -0.5f) ? 10.0f : 0.0f;
				}
			}
		}

		snapshot.spatialRevision = ++_spatialRevision;
		_audioSourcesSnapshotBack = _audioSourcesSnapshotPending.exchange(writeIndex | 4U, std::memory_order_acq_rel) & 3U;
	}

	const ResonanceAudioWorld::AudioSourcesSnapshot &ResonanceAudioWorld::AcquireAudioSourcesSnapshot()
	{
		if(_audioSourcesSnapshotPending.load(std::memory_order_acquire) & 4U)
		{
			_audioSourcesSnapshotFront = _audioSourcesSnapshotPending.exchange(_audioSourcesSnapshotFront, std::memory_order_acq_rel) & 3U;
		}
		return _audioSourcesSnapshots[_audioSourcesSnapshotFront];
	}

	void ResonanceAudioWorld::RenderOutput(ResonanceAudioListenerContext &primary, float *output, uint32 frameCount, uint32 sampleRate, uint32 channelCount, ResonanceAudioListenerContext *recording)
	{
		RN_ASSERT(frameCount == primary._frameSize && channelCount == primary._channelCount, "Unexpected output callback format");

		const AudioSourcesSnapshot &snapshot = AcquireAudioSourcesSnapshot();
		const Array *sources = snapshot.sources;
		const bool primarySpatialChanged = snapshot.spatialRevision != primary._renderedSpatialRevision;
		const bool recordingSpatialChanged = recording && snapshot.spatialRevision != recording->_renderedSpatialRevision;
		if(primarySpatialChanged)
		{
			primary.ApplyHeadRotation(snapshot.rotation[0]);
			primary._renderedSpatialRevision = snapshot.spatialRevision;
		}
		if(recordingSpatialChanged)
		{
			recording->ApplyHeadRotation(snapshot.rotation[1]);
			recording->_renderedSpatialRevision = snapshot.spatialRevision;
		}
		const double frameLength = static_cast<double>(frameCount) / sampleRate;
		const size_t sourceCount = sources->GetCount();
		for(size_t i = 0; i < sourceCount; i++)
		{
			auto *source = sources->GetObjectAtIndex<ResonanceAudioSource>(i);
			if(source->IsPositional())
			{
				source->RenderPositional(primary, recording, snapshot.sourceState[0][i], recording ? &snapshot.sourceState[1][i] : nullptr, primarySpatialChanged, recordingSpatialChanged, frameLength, frameCount);
			}
		}

		primary.MixSpatialOutput(output, frameCount);
		float *recorded = recording ? recording->_recordingOutput : nullptr;
		if(recording) recording->MixSpatialOutput(recorded, frameCount);

		const float primaryDry = primary.GetDryVolume();
		const float recordingDry = recording ? recording->GetDryVolume() : 0.0f;
		const uint32 dryChannels = recording ? std::max(channelCount, 2U) : channelCount;
		float *dry = dryChannels > channelCount ? recording->_sharedFrameData : primary._sharedFrameData;
		for(size_t i = 0; i < sourceCount; i++)
		{
			auto *source = sources->GetObjectAtIndex<ResonanceAudioSource>(i);
			if(source->IsPositional()) continue;
			const uint64 playbackRevision = source->_playbackRevision.load(std::memory_order_acquire);
			if(source->RenderSamples(frameLength, frameCount, dry, dryChannels).primary)
			{
				if(dryChannels == channelCount)
				{
					for(uint32 j = 0; j < frameCount * channelCount; j++) output[j] += dry[j] * primaryDry;
				}
				else
				{
					for(uint32 j = 0; j < frameCount; j++) output[j] += dry[j * dryChannels] * primaryDry;
				}
				if(recorded)
				{
					for(uint32 j = 0; j < frameCount; j++)
					{
						recorded[j * 2] += dry[j * dryChannels] * recordingDry;
						recorded[j * 2 + 1] += dry[j * dryChannels + 1] * recordingDry;
					}
				}
			}
			source->FinishRender(playbackRevision, false);
		}

		const float worldMasterVolume = _worldMasterVolume.load(std::memory_order_relaxed);
		primary.FinishOutput(output, frameCount, worldMasterVolume);
		if(recording) recording->FinishOutput(recorded, frameCount, worldMasterVolume);
	}

	bool ResonanceAudioWorld::StartRecordingListener(SceneNode *listener, ResonanceAudioRecordingCallback callback)
	{
		if(!listener || !callback) return false;
		LockGuard<RecursiveLockable> lock(_sourceLifetimeLock);
		if(_recordingContext) return false;

		auto *context = new ResonanceAudioListenerContext(this, 2, _audioSystem->_frameSize, _audioSystem->_sampleRate);
		context->_recordingOutput = new float[_audioSystem->_frameSize * 2];
		context->CopyMixSettings(*GetListenerContext());
		context->SetListener(listener);
		_recordingContext = context;
		for(auto *source : _allAudioSources) source->CreateRecordingSource(context);
		context->_recordingCallback = std::move(callback);
		_recordingPending = true;
		return true;
	}

	void ResonanceAudioWorld::StopRecordingListener()
	{
		LockGuard<RecursiveLockable> lock(_sourceLifetimeLock);
		if(!_recordingContext) return;

		_audioSystem->SetRecordingContext(nullptr);
		_recordingPending = false;
		_recordingContext->_recordingCallback = nullptr;
		// Destroying the graph retires all of its sources together.
		for(auto *source : _allAudioSources) source->RemoveRecordingSource(nullptr);
		SafeRelease(_recordingContext);
	}

	void ResonanceAudioWorld::SetListenerMixSettings(const std::function<void(ResonanceAudioListenerContext *)> &apply)
	{
		LockGuard<RecursiveLockable> lock(_sourceLifetimeLock);
		apply(GetListenerContext());
		if(_recordingContext) apply(_recordingContext);
	}

	void ResonanceAudioWorld::SetSimpleRoomEnabled(bool enabled)
	{
		SetListenerMixSettings([&](auto *context) { context->SetSimpleRoomEnabled(enabled); });
	}

	void ResonanceAudioWorld::SetSimpleRoom(const PositionType &position, Vector3 dimensions, float reflectionConstant, ResonanceAudioMaterial left, ResonanceAudioMaterial right, ResonanceAudioMaterial bottom, ResonanceAudioMaterial top, ResonanceAudioMaterial front, ResonanceAudioMaterial back)
	{
		SetListenerMixSettings([&](auto *context) {
			context->SetSimpleRoom(position, dimensions, reflectionConstant, left, right, bottom, top, front, back);
		});
	}

	void ResonanceAudioWorld::SetRaycastCallback(const std::function<void(PositionType, Vector3, float &distance)> &raycastCallback)
	{
		LockGuard<RecursiveLockable> lock(_sourceLifetimeLock);
		_raycastCallback = raycastCallback;
	}

	ResonanceAudioListenerState ResonanceAudioWorld::GetListenerState() const
	{
		ResonanceAudioListenerContext *ctx = GetListenerContext();
		return ctx ? ctx->GetListenerState() : ResonanceAudioListenerState();
	}

	void ResonanceAudioWorld::SetDopplerEffect(float factor, float speedOfSound)
	{
		SetListenerMixSettings([&](auto *context) { context->SetDopplerEffect(factor, speedOfSound); });
	}

	void ResonanceAudioWorld::SetDopplerVelocitySmoothing(float oldVelocityWeight)
	{
		SetListenerMixSettings([&](auto *context) { context->SetDopplerVelocitySmoothing(oldVelocityWeight); });
	}

	void ResonanceAudioWorld::Update(float delta)
	{
		SceneAttachment::Update(delta);
		LockGuard<RecursiveLockable> lock(_sourceLifetimeLock);
		if(_hasRetiredAudioAssets.exchange(false, std::memory_order_acquire))
		{
			for(ResonanceAudioSource *source : _allAudioSources) source->ReleaseRetiredAudioAssets();
		}

		GetListenerContext()->Update(delta);
		if(_recordingContext) _recordingContext->Update(delta);
		PublishAudioSourcesSnapshot(delta);
		if(_recordingPending)
		{
			// Realize queued SDK source setup off the device thread before
			// publishing this context. No source PCM is submitted here.
			_recordingContext->MixSpatialOutput(_recordingContext->_recordingOutput, _audioSystem->_frameSize);
			_audioSystem->SetRecordingContext(_recordingContext);
			_recordingPending = false;
		}
	}

	void ResonanceAudioWorld::SetInputSamplesCallback(std::function<void(uint32, uint32, uint32, const float *)> inputSamplesCallback)
	{
		ResonanceAudioListenerContext *ctx = GetListenerContext();
		if(ctx) ctx->SetInputSamplesCallback(std::move(inputSamplesCallback));
	}

	void ResonanceAudioWorld::SetListener(SceneNode *listener)
	{
		ResonanceAudioListenerContext *ctx = GetListenerContext();
		if(ctx) ctx->SetListener(listener);
	}

	SceneNode *ResonanceAudioWorld::GetListener() const
	{
		ResonanceAudioListenerContext *ctx = GetListenerContext();
		return ctx ? ctx->GetListener() : nullptr;
	}

	void ResonanceAudioWorld::SetMasterVolume(float volume)
	{
		_worldMasterVolume.store(volume, std::memory_order_relaxed);
	}

	void ResonanceAudioWorld::SetWetVolume(float volume)
	{
		SetListenerMixSettings([&](auto *context) { context->SetWetVolume(volume); });
	}

	void ResonanceAudioWorld::SetDryVolume(float volume)
	{
		SetListenerMixSettings([&](auto *context) { context->SetDryVolume(volume); });
	}

	ResonanceAudioSource *ResonanceAudioWorld::PlaySound(AudioAsset *resource) const
	{
		ResonanceAudioSource *source = new ResonanceAudioSource(resource);
		source->Play();

		GetParent()->AddNode(source);

		return source->Autorelease();
	}

	ResonanceAudioSource *ResonanceAudioWorld::PlaySound(AudioAsset *resource, const PositionType &position) const
	{
		ResonanceAudioSource *source = new ResonanceAudioSource(resource);
		source->SetWorldPosition(position);
		source->Play();

		GetParent()->AddNode(source);

		return source->Autorelease();
	}

	void ResonanceAudioWorld::RequestMicrophonePermission()
	{
		MicrophonePermissionState permissionState = GetMicrophonePermissionState();
		if(permissionState == MicrophonePermissionStateNotDetermined)
		{
#if RN_PLATFORM_MAC_OS
			if(@available(macOS 10.14, *))
			{
				[AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio
										 completionHandler:^(BOOL granted) {
										 /* if(granted)
					 {
						_inputDevice = alcCaptureOpenDevice(inputDeviceName?inputDeviceName->GetUTF8String():nullptr, 48000, AL_FORMAT_MONO16, 480);
					 }*/
										 }];
			}
#elif RN_PLATFORM_ANDROID
			const AndroidState *androidState = Kernel::GetSharedInstance()->GetAndroidState();
			if(androidState)
			{
				androidState->RequestPermission("android.permission.RECORD_AUDIO", 1);
			}
#endif
		}
	}

	ResonanceAudioWorld::MicrophonePermissionState ResonanceAudioWorld::GetMicrophonePermissionState()
	{
#if RN_PLATFORM_MAC_OS
		// Request permission to access the microphone.
		if(@available(macOS 10.14, *))
		{
			switch([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeAudio])
			{
				case AVAuthorizationStatusAuthorized:
				{
					return MicrophonePermissionStateAuthorized;
				}
				case AVAuthorizationStatusNotDetermined:
				{
					return MicrophonePermissionStateNotDetermined;
				}
				case AVAuthorizationStatusDenied:
				case AVAuthorizationStatusRestricted:
					return MicrophonePermissionStateForbidden;
			}
		}
		else
		{
			// Fallback on earlier versions
			return MicrophonePermissionStateAuthorized;
		}
#elif RN_PLATFORM_ANDROID
		const AndroidState *androidState = Kernel::GetSharedInstance()->GetAndroidState();
		int returnValue = androidState? androidState->CheckSelfPermission("android.permission.RECORD_AUDIO") : -1;

		//Permission not granted
		if(returnValue == -1)
		{
			return MicrophonePermissionStateNotDetermined;
		}
		//Permission granted
		else if(returnValue == 0)
		{
			return MicrophonePermissionStateAuthorized;
		}
#else
		return MicrophonePermissionStateAuthorized;
#endif
		return MicrophonePermissionStateForbidden;
	}
} // namespace RN
