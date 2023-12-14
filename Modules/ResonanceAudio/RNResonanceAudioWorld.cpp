//
//  RNResonanceAudioWorld.cpp
//  Rayne-ResonanceAudio
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNResonanceAudioWorld.h"
#include "RNResonanceAudioInternals.h"

#include <api/resonance_audio_api.h>
#include <platforms/common/room_effects_utils.h>

namespace RN
{
	RNDefineMeta(ResonanceAudioWorld, SceneAttachment)

	ResonanceAudioWorld *ResonanceAudioWorld::_instance = nullptr;

	ResonanceAudioWorld* ResonanceAudioWorld::GetInstance()
	{
		return _instance;
	}
	
	void ResonanceAudioWorld::AudioCallback(void *outputBuffer, const void *inputBuffer, unsigned int frameSize, unsigned int status)
	{
		AutoreleasePool pool;
		for(ResonanceAudioSource *source : _instance->_audioSources)
		{
			source->Update();
		}

		float *floatOutputBuffer = static_cast<float*>(outputBuffer);
		if(!_instance->_audioAPI->FillInterleavedOutputBuffer(2, frameSize, floatOutputBuffer))
		{
			//RNDebug("Shit. " << frameSize);
		}
	}

	//TODO: Allow to initialize with preferred device names and fall back to defaults
	ResonanceAudioWorld::ResonanceAudioWorld(ResonanceAudioSystem *audioSystem) :
		_audioSystem(audioSystem),
		_listener(nullptr),
		_inputBuffer(nullptr),
		_sharedFrameData(nullptr)
	{
		RN_ASSERT(!_instance, "There already is a ResonanceAudioWorld!");
		RN_ASSERT(_audioSystem, "Audio system needs to be provided when creating an audio world!");

		_audioAPI = vraudio::CreateResonanceAudioApi(_audioSystem->_channelCount, _audioSystem->_frameSize, _audioSystem->_sampleRate);
		_audioAPI->SetMasterVolume(1.0f);
		_audioAPI->EnableRoomEffects(true);

		_sharedFrameData = new float[_audioSystem->_frameSize * _audioSystem->_channelCount];
		_instance = this;
		_audioSystem->Retain();
		_audioSystem->SetAudioCallback(AudioCallback);
	}
		
	ResonanceAudioWorld::~ResonanceAudioWorld()
	{
		_audioSystem->Release();

		if(_sharedFrameData)
		{
			delete [] _sharedFrameData;
			_sharedFrameData = nullptr;
		}

		_instance = nullptr;
	}

	void ResonanceAudioWorld::AddAudioSource(ResonanceAudioSource* source)
	{
		_audioSources.push_back(source);
	}

	void ResonanceAudioWorld::RemoveAudioSource(ResonanceAudioSource* source)
	{
		auto iterator = std::find(_audioSources.begin(), _audioSources.end(), source);
		if(iterator != _audioSources.end())
		{
			_audioSources.erase(iterator);
		}
	}

	void ResonanceAudioWorld::SetSimpleRoomEnabled(bool enabled)
	{
		_audioAPI->EnableRoomEffects(enabled);
	}
	
	void ResonanceAudioWorld::SetSimpleRoom(Vector3 position, Vector3 dimensions, float reflectionConstant, ResonanceAudioMaterial left, ResonanceAudioMaterial right, ResonanceAudioMaterial bottom, ResonanceAudioMaterial top, ResonanceAudioMaterial front, ResonanceAudioMaterial back)
	{
		vraudio::RoomProperties roomProperties;
		roomProperties.dimensions[0] = dimensions.x;
		roomProperties.dimensions[1] = dimensions.y;
		roomProperties.dimensions[2] = dimensions.z;
		roomProperties.position[0] = position.x;
		roomProperties.position[1] = position.y;
		roomProperties.position[2] = position.z;
		roomProperties.reflection_scalar = reflectionConstant;
		roomProperties.material_names[0] = static_cast<vraudio::MaterialName>(left);
		roomProperties.material_names[1] = static_cast<vraudio::MaterialName>(right);
		roomProperties.material_names[2] = static_cast<vraudio::MaterialName>(bottom);
		roomProperties.material_names[3] = static_cast<vraudio::MaterialName>(top);
		roomProperties.material_names[4] = static_cast<vraudio::MaterialName>(front);
		roomProperties.material_names[5] = static_cast<vraudio::MaterialName>(back);

		_audioAPI->SetReverbProperties(vraudio::ComputeReverbProperties(roomProperties));
		_audioAPI->SetReflectionProperties(vraudio::ComputeReflectionProperties(roomProperties));

		for(ResonanceAudioSource *source : _instance->_audioSources)
		{
			Vector3 sourcePosition = source->GetWorldPosition();
			vraudio::WorldPosition audioSourcePosition;
			audioSourcePosition[0] = sourcePosition.x;
			audioSourcePosition[1] = sourcePosition.y;
			audioSourcePosition[2] = sourcePosition.z;

			vraudio::WorldPosition audioRoomPosition;
			audioRoomPosition[0] = position.x;
			audioRoomPosition[1] = position.y;
			audioRoomPosition[2] = position.z;

			vraudio::WorldRotation audioRoomRotation;

			vraudio::WorldPosition audioRoomDimensions;
			audioRoomDimensions[0] = dimensions.x;
			audioRoomDimensions[1] = dimensions.y;
			audioRoomDimensions[2] = dimensions.z;

			_audioAPI->SetSourceRoomEffectsGain(source->_sourceID, vraudio::ComputeRoomEffectsGain(audioSourcePosition, audioRoomPosition, audioRoomRotation, audioRoomDimensions));

			if(_raycastCallback)
			{
				float distance;
				_raycastCallback(sourcePosition, _listener->GetWorldPosition() - sourcePosition, distance);
				if(distance > -0.5f)
				{
					/*float realDistance = _listener->GetWorldPosition().GetDistance(sourcePosition);
					float otherDistance;
					_raycastCallback(_listener->GetWorldPosition(), sourcePosition - _listener->GetWorldPosition(), otherDistance);
					if(otherDistance > -0.5f)
					{
						_audioAPI->SetSoundObjectOcclusionIntensity(source->_sourceID, realDistance - distance - otherDistance);
					}
					else
					{
						_audioAPI->SetSoundObjectOcclusionIntensity(source->_sourceID, realDistance - distance);
					}*/
					_audioAPI->SetSoundObjectOcclusionIntensity(source->_sourceID, 10.0f); //Maybe make this dependent on the material
				}
				else
				{
					_audioAPI->SetSoundObjectOcclusionIntensity(source->_sourceID, 0.0f);
				}
			}
		}
	}

	void ResonanceAudioWorld::SetRaycastCallback(const std::function<void (Vector3, Vector3, float &distance)> &raycastCallback)
	{
		_raycastCallback = raycastCallback;
	}
		
	void ResonanceAudioWorld::Update(float delta)
	{
		//Update listener position
		if(_listener)
		{
			Vector3 listenerPosition = _listener->GetWorldPosition();
			Quaternion listenerRotation = _listener->GetWorldRotation();
			_audioAPI->SetHeadPosition(listenerPosition.x, listenerPosition.y, listenerPosition.z);
			_audioAPI->SetHeadRotation(listenerRotation.x, listenerRotation.y, listenerRotation.z, listenerRotation.w);

			//Calculate current room properties
			Vector3 dimensions(10.0f, 10.0f, 10.0f);
			ResonanceAudioMaterial material[6] = {ResonanceAudioMaterialBrickBare, ResonanceAudioMaterialBrickBare, ResonanceAudioMaterialBrickBare, ResonanceAudioMaterialBrickBare, ResonanceAudioMaterialBrickBare, ResonanceAudioMaterialBrickBare};

			if(_instance->_raycastCallback)
			{
				Vector3 directions[6];
				directions[0].x = -1.0f;
				directions[1].x = 1.0f;
				directions[2].y = -1.0f;
				directions[3].y = 1.0f;
				directions[4].z = -1.0f;
				directions[5].z = 1.0f;

				float distance[6];
				for(int i = 0; i < 6; i++)
				{
					_instance->_raycastCallback(listenerPosition, directions[i] * 100.0f, distance[i]);

					if(distance[i] < -0.5f)
					{
						distance[i] = 1.0f;
						material[i] = ResonanceAudioMaterialTransparent;
					}
					else
					{
						material[i] = ResonanceAudioMaterialBrickBare;
					}
				}

				dimensions.x = distance[0] + distance[1];
				dimensions.y = distance[2] + distance[3];
				dimensions.z = distance[4] + distance[5];

				//Move to actual room center
				listenerPosition.x += (distance[0] * directions[0].x + distance[1] * directions[1].x) * 0.5f;
				listenerPosition.y += (distance[2] * directions[2].x + distance[3] * directions[3].x) * 0.5f;
				listenerPosition.z += (distance[4] * directions[4].x + distance[5] * directions[5].x) * 0.5f;
			}
			SetSimpleRoom(listenerPosition, dimensions, 1.0f, material[0], material[1], material[2], material[3], material[4], material[5]);
		}
	}
	
	void ResonanceAudioWorld::SetInputBuffer(AudioAsset *inputBuffer)
	{
		RN_ASSERT(!inputBuffer || (inputBuffer->GetData()->GetLength() > 2 * _audioSystem->_frameSize), "Requires an input buffer big enough to contain two frames of audio data!");
		
		SafeRelease(_inputBuffer);
		_inputBuffer = inputBuffer;
		SafeRetain(_inputBuffer);
	}
		
	void ResonanceAudioWorld::SetListener(SceneNode *listener)
	{
		if(_listener)
			_listener->Release();
			
		_listener = nullptr;

		if(listener)
			_listener = listener->Retain();
	}
		
	ResonanceAudioSource *ResonanceAudioWorld::PlaySound(AudioAsset *resource) const
	{
		ResonanceAudioSource *source = new ResonanceAudioSource(resource);
		source->Play();

		GetParent()->AddNode(source);

		return source->Autorelease();
	}

	ResonanceAudioSource *ResonanceAudioWorld::PlaySound(AudioAsset *resource, Vector3 position) const
	{
		ResonanceAudioSource *source = new ResonanceAudioSource(resource);
		source->SetWorldPosition(position);
		source->Play();

		GetParent()->AddNode(source);

		return source->Autorelease();
	}
	
/*	void ResonanceAudioWorld::SetCustomWriteCallback(const std::function<void (double)> &customWriteCallback)
	{
		_customWriteCallback = customWriteCallback;
	}*/
}
