//
//  RNResonanceAudioListenerContext.cpp
//  Rayne-ResonanceAudio
//
//  Copyright 2026 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResonanceAudioListenerContext.h"
#include "RNResonanceAudioWorld.h"

#include <api/resonance_audio_api.h>
#include <platforms/common/room_effects_utils.h>

namespace RN
{
	RNDefineMeta(ResonanceAudioListenerContext, Object)

	static inline float SoftClipTanhKnee(float x, float T)
	{
		const float ax = std::fabs(x);
		if(ax <= T) return x;

		const float u = (ax - T) / (1.0f - T);
		const float y = T + (1.0f - T) * std::tanhf(u);
		return std::copysign(y, x);
	}

	ResonanceAudioListenerContext::ResonanceAudioListenerContext(ResonanceAudioWorld *world, uint32 channelCount, uint32 frameSize, uint32 sampleRate) :
		_world(world),
		_listener(nullptr),
		_oldPosition(PositionType()),
		_frameSize(frameSize),
		_channelCount(channelCount),
		_sharedFrameData(nullptr),
		_listenerMasterVolume(1.0f),
		_wetVolume(1.0f),
		_dryVolume(1.0f),
		_dopplerFactor(1.0f),
		_dopplerSpeedOfSound(343.3f),
		_dopplerVelocitySmoothing(0.95f),
		_roomEnabled(false),
		_roomDirty(false),
		_roomPosition(PositionType()),
		_roomDimensions(Vector3(1.0f, 1.0f, 1.0f)),
		_roomReflectionConstant(1.0f),
		_audioAPI(nullptr)
	{
		_roomMaterials[0] = ResonanceAudioMaterialBrickBare;
		_roomMaterials[1] = ResonanceAudioMaterialBrickBare;
		_roomMaterials[2] = ResonanceAudioMaterialBrickBare;
		_roomMaterials[3] = ResonanceAudioMaterialBrickBare;
		_roomMaterials[4] = ResonanceAudioMaterialBrickBare;
		_roomMaterials[5] = ResonanceAudioMaterialBrickBare;

		_audioAPI = vraudio::CreateResonanceAudioApi(channelCount, frameSize, sampleRate);
		_audioAPI->SetMasterVolume(_wetVolume);
		_audioAPI->EnableRoomEffects(false);

		_sharedFrameData = new float[_frameSize * _channelCount];
	}

	ResonanceAudioListenerContext::~ResonanceAudioListenerContext()
	{
		SafeRelease(_listener);
		delete[] _recordingOutput;
		delete[] _sharedFrameData;
		_sharedFrameData = nullptr;
		delete _audioAPI;
		_audioAPI = nullptr;
	}

	void ResonanceAudioListenerContext::SetMasterVolume(float volume)
	{
		_listenerMasterVolume.store(volume, std::memory_order_relaxed);
	}

	void ResonanceAudioListenerContext::SetWetVolume(float volume)
	{
		const float v = (volume >= 0.0f) ? volume : 0.0f;
		_wetVolume.store(v, std::memory_order_relaxed);
		_audioAPI->SetMasterVolume(v);
	}

	void ResonanceAudioListenerContext::SetDryVolume(float volume)
	{
		_dryVolume.store(volume, std::memory_order_relaxed);
	}

	void ResonanceAudioListenerContext::SetDopplerEffect(float factor, float speedOfSound)
	{
		const float f = (factor > 0.0f) ? factor : 0.0f;
		const float c = (speedOfSound > 0.001f) ? speedOfSound : 0.001f;
		_dopplerFactor = f;
		_dopplerSpeedOfSound = c;
	}

	void ResonanceAudioListenerContext::SetDopplerVelocitySmoothing(float oldVelocityWeight)
	{
		_dopplerVelocitySmoothing = std::clamp(oldVelocityWeight, 0.0f, 0.999f);
	}

	void ResonanceAudioListenerContext::SetSimpleRoom(const PositionType &position, Vector3 dimensions, float reflectionConstant, ResonanceAudioMaterial left, ResonanceAudioMaterial right, ResonanceAudioMaterial bottom, ResonanceAudioMaterial top, ResonanceAudioMaterial front, ResonanceAudioMaterial back)
	{
		_roomPosition = position;
		_roomDimensions = dimensions;
		_roomReflectionConstant = reflectionConstant;
		_roomMaterials[0] = left;
		_roomMaterials[1] = right;
		_roomMaterials[2] = bottom;
		_roomMaterials[3] = top;
		_roomMaterials[4] = front;
		_roomMaterials[5] = back;
		_roomDirty = true;
	}

	void ResonanceAudioListenerContext::SetSimpleRoomEnabled(bool enabled)
	{
		if(enabled && !_roomEnabled) _roomDirty = true;
		_roomEnabled = enabled;
		_audioAPI->EnableRoomEffects(enabled);
	}

	void ResonanceAudioListenerContext::SetInputSamplesCallback(std::function<void(uint32, uint32, uint32, const float *)> inputSamplesCallback)
	{
		LockGuard<RecursiveLockable> lock(_inputSamplesCallbackLock);
		_inputSamplesCallback = std::move(inputSamplesCallback);
	}

	void ResonanceAudioListenerContext::SetListener(SceneNode *listener)
	{
		LockGuard<RecursiveLockable> lock(ResonanceAudioWorld::_sourceLifetimeLock);
		SafeRetain(listener);
		SafeRelease(_listener);
		_listener = listener;
		_oldPosition = _listener ? _listener->GetWorldPosition() : PositionType();
		_roomDirty = true;

		_listenerState.position = _oldPosition;
		_listenerState.velocity = Vector3();
		_listenerState.rotation = _listener ? _listener->GetWorldRotation() : Quaternion();
		_listenerState.isValid = _listener != nullptr;
	}

	ResonanceAudioListenerState ResonanceAudioListenerContext::GetListenerState() const
	{
		LockGuard<RecursiveLockable> lock(ResonanceAudioWorld::_sourceLifetimeLock);
		return _listenerState;
	}

	void ResonanceAudioListenerContext::CopyMixSettings(const ResonanceAudioListenerContext &other)
	{
		SetWetVolume(other.GetWetVolume());
		SetDryVolume(other.GetDryVolume());
		SetDopplerEffect(other.GetDopplerFactor(), other.GetDopplerSpeedOfSound());
		SetDopplerVelocitySmoothing(other.GetDopplerVelocitySmoothing());
		SetSimpleRoom(other._roomPosition, other._roomDimensions, other._roomReflectionConstant, other._roomMaterials[0], other._roomMaterials[1], other._roomMaterials[2], other._roomMaterials[3], other._roomMaterials[4], other._roomMaterials[5]);
		SetSimpleRoomEnabled(other._roomEnabled);
	}

	void ResonanceAudioListenerContext::Update(float delta)
	{
		LockGuard<RecursiveLockable> lock(ResonanceAudioWorld::_sourceLifetimeLock);
		if(!_listener) return;

		const PositionType position = _listener->GetWorldPosition();
		const bool listenerMoved = !(position == _listenerState.position);
		_listenerState.position = position;
		_listenerState.rotation = _listener->GetWorldRotation();
		_listenerState.velocity = Vector3();
		if(delta > 0.0f)
		{
			_listenerState.velocity = Vector3(position - _oldPosition) / delta;
			_oldPosition = position;
		}
		const Vector3 roomPosition = _roomEnabled ? Vector3(_roomPosition - position) : Vector3();

		// Apply room properties if they changed.
		if(_roomEnabled && (_roomDirty || listenerMoved))
		{
			vraudio::RoomProperties roomProperties;
			roomProperties.dimensions[0] = _roomDimensions.x;
			roomProperties.dimensions[1] = _roomDimensions.y;
			roomProperties.dimensions[2] = _roomDimensions.z;
			roomProperties.position[0] = roomPosition.x;
			roomProperties.position[1] = roomPosition.y;
			roomProperties.position[2] = roomPosition.z;
			roomProperties.reflection_scalar = _roomReflectionConstant;
			roomProperties.material_names[0] = static_cast<vraudio::MaterialName>(_roomMaterials[0]);
			roomProperties.material_names[1] = static_cast<vraudio::MaterialName>(_roomMaterials[1]);
			roomProperties.material_names[2] = static_cast<vraudio::MaterialName>(_roomMaterials[2]);
			roomProperties.material_names[3] = static_cast<vraudio::MaterialName>(_roomMaterials[3]);
			roomProperties.material_names[4] = static_cast<vraudio::MaterialName>(_roomMaterials[4]);
			roomProperties.material_names[5] = static_cast<vraudio::MaterialName>(_roomMaterials[5]);

			_audioAPI->SetReverbProperties(vraudio::ComputeReverbProperties(roomProperties));
			_audioAPI->SetReflectionProperties(vraudio::ComputeReflectionProperties(roomProperties));

			_roomDirty = false;
		}
	}

	void ResonanceAudioListenerContext::CaptureInput(const float *input, uint32 sampleRate, uint32 channelCount, uint32 frameCount)
	{
		AutoreleasePool pool;
		LockGuard<RecursiveLockable> lock(_inputSamplesCallbackLock);
		// Keep the callback alive if it replaces itself.
		auto callback = _inputSamplesCallback;
		if(callback) callback(sampleRate, channelCount, frameCount, input);
	}

	void ResonanceAudioListenerContext::ApplyHeadRotation(const Quaternion &rotation)
	{
		_audioAPI->SetHeadRotation(rotation.x, rotation.y, rotation.z, rotation.w);
	}

	void ResonanceAudioListenerContext::MixSpatialOutput(float *output, uint32 frameCount)
	{
		if(!_audioAPI->FillInterleavedOutputBuffer(_channelCount, frameCount, output))
		{
			memset(output, 0, frameCount * _channelCount * sizeof(float));
		}
	}

	void ResonanceAudioListenerContext::FinishOutput(float *output, uint32 frameCount, float worldMasterVolume)
	{
		const float finalVolume = worldMasterVolume * GetMasterVolume();
		const uint32 sampleCount = frameCount * _channelCount;
		for(uint32 i = 0; i < sampleCount; i++)
		{
			output[i] = SoftClipTanhKnee(output[i] * finalVolume, 0.9f);
		}
	}
} // namespace RN
