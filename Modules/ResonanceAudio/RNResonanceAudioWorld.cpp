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

	ResonanceAudioWorld *ResonanceAudioWorld::_instance = nullptr;

	ResonanceAudioWorld *ResonanceAudioWorld::GetInstance()
	{
		return _instance;
	}

	//TODO: Allow to initialize with preferred device names and fall back to defaults
	ResonanceAudioWorld::ResonanceAudioWorld(ResonanceAudioSystem *audioSystem) :
		_audioSystem(audioSystem),
		_worldMasterVolume(1.0f)
	{
		RN_ASSERT(!_instance, "There already is a ResonanceAudioWorld!");
		RN_ASSERT(_audioSystem, "Audio system needs to be provided when creating an audio world!");

		for(int i = 0; i < 3; i++)
		{
			_audioSourcesSnapshots[i] = new Array();
		}

		_audioSystem->SetOwningWorld(this);
		_audioSystem->CreateListenerContext();
		_instance = this;
		_audioSystem->Retain();
	}

	ResonanceAudioWorld::~ResonanceAudioWorld()
	{
		_audioSystem->RemoveAllListenerContexts();
		_audioSystem->SetOwningWorld(nullptr);
		_audioSystem->Release();

		for(int i = 0; i < 3; i++)
		{
			SafeRelease(_audioSourcesSnapshots[i]);
		}

		_instance = nullptr;
	}

	void ResonanceAudioWorld::AddAudioSource(ResonanceAudioSource *source)
	{
		Lock();
		_audioSources.push_back(source);
		_audioSourcesSnapshotDirty = true;
		Unlock();
	}

	void ResonanceAudioWorld::RemoveAudioSource(ResonanceAudioSource *source)
	{
		Lock();
		auto iterator = std::find(_audioSources.begin(), _audioSources.end(), source);
		if(iterator != _audioSources.end())
		{
			_audioSources.erase(iterator);
			_audioSourcesSnapshotDirty = true;
		}
		Unlock();
	}

	void ResonanceAudioWorld::PublishAudioSourcesSnapshot(const std::vector<ResonanceAudioSource *> &sources)
	{
		const uint32 writeIndex = _audioSourcesSnapshotBack;

		Array *fresh = _audioSourcesSnapshots[writeIndex];
		fresh->RemoveAllObjects();
		for(ResonanceAudioSource *source : sources)
		{
			if(!source) continue;
			fresh->AddObject(source);
		}

		_audioSourcesSnapshotBack = _audioSourcesSnapshotPending.exchange(writeIndex | 4U, std::memory_order_acq_rel) & 3U;
	}

	Array *ResonanceAudioWorld::AcquireAudioSourcesSnapshot()
	{
		if(_audioSourcesSnapshotPending.load(std::memory_order_acquire) & 4U)
		{
			_audioSourcesSnapshotFront = _audioSourcesSnapshotPending.exchange(_audioSourcesSnapshotFront, std::memory_order_acq_rel) & 3U;
		}
		return _audioSourcesSnapshots[_audioSourcesSnapshotFront];
	}

	void ResonanceAudioWorld::SetSimpleRoomEnabled(bool enabled)
	{
		if(!_audioSystem) return;
		for(ResonanceAudioListenerContext *ctx : _audioSystem->_listenerContexts)
		{
			if(ctx) ctx->SetSimpleRoomEnabled(enabled);
		}
	}

	void ResonanceAudioWorld::SetSimpleRoom(const PositionType &position, Vector3 dimensions, float reflectionConstant, ResonanceAudioMaterial left, ResonanceAudioMaterial right, ResonanceAudioMaterial bottom, ResonanceAudioMaterial top, ResonanceAudioMaterial front, ResonanceAudioMaterial back)
	{
		if(!_audioSystem) return;
		for(ResonanceAudioListenerContext *ctx : _audioSystem->_listenerContexts)
		{
			if(ctx) ctx->SetSimpleRoom(position, dimensions, reflectionConstant, left, right, bottom, top, front, back);
		}
	}

	void ResonanceAudioWorld::SetRaycastCallback(const std::function<void(PositionType, Vector3, float &distance)> &raycastCallback)
	{
		_raycastCallback = raycastCallback;
	}

	ResonanceAudioListenerState ResonanceAudioWorld::GetListenerState() const
	{
		ResonanceAudioListenerContext *ctx = GetListenerContext();
		return ctx ? ctx->GetListenerState() : ResonanceAudioListenerState();
	}

	void ResonanceAudioWorld::SetDopplerEffect(float factor, float speedOfSound)
	{
		if(!_audioSystem) return;
		for(ResonanceAudioListenerContext *ctx : _audioSystem->_listenerContexts)
		{
			if(ctx) ctx->SetDopplerEffect(factor, speedOfSound);
		}
	}

	void ResonanceAudioWorld::SetDopplerVelocitySmoothing(float oldVelocityWeight)
	{
		if(!_audioSystem) return;
		for(ResonanceAudioListenerContext *ctx : _audioSystem->_listenerContexts)
		{
			if(ctx) ctx->SetDopplerVelocitySmoothing(oldVelocityWeight);
		}
	}

	void ResonanceAudioWorld::Update(float delta)
	{
		SceneAttachment::Update(delta);

		// Publish audio sources snapshot at most once per frame (instead of on every add/remove).
		Lock();
		if(_audioSourcesSnapshotDirty)
		{
			PublishAudioSourcesSnapshot(_audioSources);
			_audioSourcesSnapshotDirty = false;
		}
		Unlock();
		
		if(_audioSystem)
		{
			for(ResonanceAudioListenerContext *ctx : _audioSystem->_listenerContexts)
			{
				if(ctx) ctx->Update(delta);
			}
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
		if(!_audioSystem) return;
		for(ResonanceAudioListenerContext *ctx : _audioSystem->_listenerContexts)
		{
			if(ctx) ctx->SetWetVolume(volume);
		}
	}

	void ResonanceAudioWorld::SetDryVolume(float volume)
	{
		if(!_audioSystem) return;
		for(ResonanceAudioListenerContext *ctx : _audioSystem->_listenerContexts)
		{
			if(ctx) ctx->SetDryVolume(volume);
		}
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
