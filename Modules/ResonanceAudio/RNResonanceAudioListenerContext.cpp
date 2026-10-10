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
		const uint32 currentIndex = _inputSamplesCallbackIndex.load(std::memory_order_relaxed) & 1;
		const uint32 nextIndex = currentIndex ^ 1;
		_inputSamplesCallbackBuffers[nextIndex] = std::move(inputSamplesCallback);
		_inputSamplesCallbackIndex.store(nextIndex, std::memory_order_release);
	}

	void ResonanceAudioListenerContext::SetListener(SceneNode *listener)
	{
		SafeRelease(_listener);
		_listener = SafeRetain(listener);
		_oldPosition = _listener ? _listener->GetWorldPosition() : PositionType();
		_roomDirty = true;

		const uint32 currentIndex = _stateIndex.load(std::memory_order_relaxed) & 1;
		const uint32 nextIndex = currentIndex ^ 1;
		ResonanceAudioListenerState &dst = _stateBuffers[nextIndex];
		if(_listener)
		{
			dst.position = _oldPosition;
			dst.velocity = Vector3(0.0f, 0.0f, 0.0f);
			dst.rotation = _listener->GetWorldRotation();
			dst.isValid = true;

			_audioAPI->SetHeadPosition(0.0f, 0.0f, 0.0f);
			_audioAPI->SetHeadRotation(dst.rotation.x, dst.rotation.y, dst.rotation.z, dst.rotation.w);
		}
		else
		{
			dst.isValid = false;
		}

		_stateIndex.store(nextIndex, std::memory_order_release);
	}

	ResonanceAudioListenerState ResonanceAudioListenerContext::GetListenerState() const
	{
		const uint32 index = _stateIndex.load(std::memory_order_acquire) & 1;
		return _stateBuffers[index];
	}

	void ResonanceAudioListenerContext::Update(float delta)
	{
		if(!_listener)
			return;

		const PositionType position = _listener->GetWorldPosition();
		const Quaternion rotation = _listener->GetWorldRotation();
		const bool listenerMoved = !(position == _oldPosition);

		_audioAPI->SetHeadPosition(0.0f, 0.0f, 0.0f);
		_audioAPI->SetHeadRotation(rotation.x, rotation.y, rotation.z, rotation.w);

		Vector3 velocity(0.0f, 0.0f, 0.0f);
		if(delta > 0.0f)
		{
			velocity = Vector3(position - _oldPosition) / delta;
			_oldPosition = position;
		}

		const uint32 currentIndex = _stateIndex.load(std::memory_order_relaxed) & 1;
		const uint32 nextIndex = currentIndex ^ 1;
		ResonanceAudioListenerState &dst = _stateBuffers[nextIndex];
		dst.position = position;
		dst.velocity = velocity;
		dst.rotation = rotation;
		dst.isValid = true;
		_stateIndex.store(nextIndex, std::memory_order_release);

		ResonanceAudioWorld *world = _world;
		if(!world) return;

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

		// Keep per-source room gain + occlusion up to date (main thread only).
		vraudio::WorldPosition audioRoomPosition;
		audioRoomPosition[0] = roomPosition.x;
		audioRoomPosition[1] = roomPosition.y;
		audioRoomPosition[2] = roomPosition.z;
		vraudio::WorldRotation audioRoomRotation; // identity
		vraudio::WorldPosition audioRoomDimensions;
		audioRoomDimensions[0] = _roomDimensions.x;
		audioRoomDimensions[1] = _roomDimensions.y;
		audioRoomDimensions[2] = _roomDimensions.z;

		SceneNode *listener = _listener;
		const PositionType listenerPosition = listener ? listener->GetWorldPosition() : PositionType();

		world->Lock();
		for(ResonanceAudioSource *source : world->_audioSources)
		{
			if(!source || !source->IsPositional()) continue;

			const PositionType sourceWorldPosition = source->GetWorldPosition();
			Vector3 sourcePosition(sourceWorldPosition - listenerPosition);
			source->SetSpatialPosition(sourcePosition);
			if(!_roomEnabled) continue;

			vraudio::WorldPosition audioSourcePosition;
			audioSourcePosition[0] = sourcePosition.x;
			audioSourcePosition[1] = sourcePosition.y;
			audioSourcePosition[2] = sourcePosition.z;

			_audioAPI->SetSourceRoomEffectsGain(source->_sourceID, vraudio::ComputeRoomEffectsGain(audioSourcePosition, audioRoomPosition, audioRoomRotation, audioRoomDimensions));

			if(world->_raycastCallback && listener)
			{
				float distance;
				world->_raycastCallback(sourceWorldPosition, Vector3(listenerPosition - sourceWorldPosition), distance);
				_audioAPI->SetSoundObjectOcclusionIntensity(source->_sourceID, (distance > -0.5f) ? 10.0f : 0.0f);
			}
		}
		world->Unlock();
	}

	void ResonanceAudioListenerContext::RenderAudio(void *outputBuffer, const void *inputBuffer, uint32 sampleRate, uint32 channelCount, uint32 frameCount, uint32 status)
	{
		AutoreleasePool pool;

		ResonanceAudioWorld *world = _world;
		if(!world)
		{
			if(outputBuffer)
			{
				memset(outputBuffer, 0, frameCount * channelCount * sizeof(float));
			}
			return;
		}

		// Capture microphone samples if requested
		const uint32 callbackIndex = _inputSamplesCallbackIndex.load(std::memory_order_acquire) & 1;
		const auto &inputSamplesCallback = _inputSamplesCallbackBuffers[callbackIndex];
		if(inputSamplesCallback && inputBuffer)
		{
			const float *floatInput = static_cast<const float *>(inputBuffer);
			inputSamplesCallback(sampleRate, channelCount, frameCount, floatInput);
		}

		if(!outputBuffer)
		{
			return;
		}

		Array *sourcesSnapshot = world->AcquireAudioSourcesSnapshot();

		sourcesSnapshot->Enumerate<ResonanceAudioSource>([&](ResonanceAudioSource *source, size_t index, bool &stop){
			source->Update();
		});

		const uint32 outputSampleCount = frameCount * channelCount;
		const float worldMasterVolume = world->_worldMasterVolume.load(std::memory_order_relaxed);
		const float listenerMasterVolume = _listenerMasterVolume.load(std::memory_order_relaxed);
		const float dryVolume = _dryVolume.load(std::memory_order_relaxed);
		const float finalVolume = worldMasterVolume * listenerMasterVolume;

		float *floatOutputBuffer = static_cast<float *>(outputBuffer);
		if(!GetAudioAPI()->FillInterleavedOutputBuffer(channelCount, frameCount, floatOutputBuffer))
		{
			memset(floatOutputBuffer, 0, outputSampleCount * sizeof(float));
		}

		const float outputSampleRate = static_cast<float>(sampleRate);
		sourcesSnapshot->Enumerate<ResonanceAudioSource>([&](ResonanceAudioSource *source, size_t index, bool &stop){
			if(source->IsPositional()) return;

			float *frameData = _sharedFrameData;
			if(source->Update(frameCount / outputSampleRate, frameCount, &frameData, channelCount))
			{
				for(uint32 i = 0; i < outputSampleCount; i++)
				{
					floatOutputBuffer[i] += frameData[i] * dryVolume;
				}
			}
		});

		for(uint32 i = 0; i < outputSampleCount; i++)
		{
			floatOutputBuffer[i] = SoftClipTanhKnee(floatOutputBuffer[i] * finalVolume, 0.9f);
		}
	}
} // namespace RN
