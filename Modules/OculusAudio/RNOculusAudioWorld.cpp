//
//  RNOculusAudioWorld.cpp
//  Rayne-OculusAudio
//
//  Copyright 2019 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNOculusAudioWorld.h"
#include "RNOculusAudioInternals.h"

namespace RN
{
	RNDefineMeta(OculusAudioWorld, SceneAttachment)

	OculusAudioWorld *OculusAudioWorld::_instance = nullptr;

	OculusAudioWorld* OculusAudioWorld::GetInstance()
	{
		return _instance;
	}
	
	void OculusAudioWorld::AudioCallback(void *outputBuffer, void *inputBuffer, unsigned int frameSize, unsigned int status)
	{
		if(inputBuffer && _instance->_inputBuffer)
		{
			//TODO: Handle overflow? (push silence?)
			_instance->_inputBuffer->PushData(inputBuffer, frameSize * sizeof(float));
		}
		
		if(outputBuffer)
		{
			float *buffer = static_cast<float*>(outputBuffer);
			double bufferLength = 1.0/static_cast<double>(_instance->_audioSystem->_sampleRate) * frameSize;
			
			memset(buffer, 0, frameSize * 2 * sizeof(float));
			
			//TODO: Handle underflow?
			if(status)
				std::cout << "Stream underflow detected!" << std::endl;
			
			uint32_t oculusAudioStatus = 0;
			
			//Mix audio sources
			if(_instance->_audioSources->GetCount() > 0)
			{
				_instance->_audioSources->Enumerate<OculusAudioSource>([&](OculusAudioSource *source, size_t index, bool &stop) {
					if(source->_oculusAudioSourceIndex == -1)
					{
						//Too many active audio sources
						return;
					}
					
					float *outData = nullptr;
					source->Update(bufferLength, frameSize, &outData);
					
					if(!outData)
						return;

					// Spatialize the sound into the output buffer.  Note that there
					// are two APIs, one for interleaved sample data and another for
					// separate left/right sample data
					ovrAudio_SpatializeMonoSourceInterleaved(_instance->_internals->oculusAudioContext, source->_oculusAudioSourceIndex, &oculusAudioStatus, _instance->_tempFrameData, outData);
					
					for(int i = 0; i < frameSize; i++)
					{
						for(int j = 0; j < 2; j++)
						{
							buffer[i*2 + j] += _instance->_tempFrameData[i*2 + j];
						}
					}
				});
			}
			
			ovrAudio_MixInSharedReverbInterleaved(_instance->_internals->oculusAudioContext, &oculusAudioStatus, buffer);

			
			//Mix output with audio players
			if(_instance->_audioPlayers->GetCount() > 0)
			{
				_instance->_audioPlayers->Enumerate<OculusAudioPlayer>([&](OculusAudioPlayer *source, size_t index, bool &stop) {
					if(!source->IsPlaying())
						return;
					
					float *outData = nullptr;
					source->Update(bufferLength, frameSize, &outData);
					
					if(!outData)
						return;
					
					for(int i = 0; i < frameSize; i++)
					{
						for(int j = 0; j < 2; j++)
						{
							buffer[i*2 + j] += outData[i*2 + j];
						}
					}
				});
			}
		}
	}
	
	void OculusAudioWorld::RaycastCallback(ovrAudioVector3f origin, ovrAudioVector3f direction, ovrAudioVector3f* hit, ovrAudioVector3f* normal, void* pctx)
	{
		if(_instance->_raycastCallback)
		{
			RN::Vector3 tempHitPosition;
			RN::Vector3 tempHitNormal;
			_instance->_raycastCallback(RN::Vector3(origin.x, origin.y, origin.z), RN::Vector3(direction.x, direction.y, direction.z), tempHitPosition, tempHitNormal);
			
			hit->x = tempHitPosition.x;
			hit->y = tempHitPosition.y;
			hit->z = tempHitPosition.z;
			
			normal->x = tempHitNormal.x;
			normal->y = tempHitNormal.y;
			normal->z = tempHitNormal.z;
		}
	}

	//TODO: Allow to initialize with preferred device names and fall back to defaults
	OculusAudioWorld::OculusAudioWorld(OculusAudioSystem *audioSystem, uint32 maxSources) :
		_audioSystem(audioSystem),
		_listener(nullptr),
		_inputBuffer(nullptr),
		_audioSources(new Array()),
		_audioPlayers(new Array()),
		_maxSourceCount(maxSources),
		_isSourceAvailable(nullptr),
		_sharedFrameData(nullptr),
		_tempFrameData(nullptr),
		_internals(new OculusAudioWorldInternals())
	{
		RN_ASSERT(!_instance, "There already is a OculusAudioWorld!");
		RN_ASSERT(_audioSystem, "Audio system needs to be provided when creating an audio world!");

		// Version checking is not strictly necessary but it's a good idea!
		int major, minor, patch;
		const char *VERSION_STRING;
		
		VERSION_STRING = ovrAudio_GetVersion(&major, &minor, &patch);
		RN_ASSERT(major == OVR_AUDIO_MAJOR_VERSION && minor == OVR_AUDIO_MINOR_VERSION, "Mismatched Oculus Audio SDK version!");
		RNDebug(RNSTR("Using OVRAudio: " << VERSION_STRING));
		
		ovrAudioContextConfiguration config = {};
		config.acc_Size = sizeof(config);
		config.acc_SampleRate = _audioSystem->_sampleRate;
		config.acc_BufferLength = _audioSystem->_frameSize;
		config.acc_MaxNumSources = _maxSourceCount;
		
		if(ovrAudio_CreateContext(&_internals->oculusAudioContext, &config) != ovrSuccess)
		{
			RNDebug(RNCSTR("WARNING: Could not create context!"));
			return;
		}
		
		ovrAudio_SetUnitScale(_internals->oculusAudioContext, 1.0f);
		
		ovrAudio_Enable(_internals->oculusAudioContext, ovrAudioEnable_SimpleRoomModeling, 0);
		ovrAudio_Enable(_internals->oculusAudioContext, ovrAudioEnable_LateReverberation, 1);
		ovrAudio_Enable(_internals->oculusAudioContext, ovrAudioEnable_RandomizeReverb, 1);
		
		_isSourceAvailable = new bool[_maxSourceCount];
		for(int i = 0; i < _maxSourceCount; i++) _isSourceAvailable[i] = true;
		_sharedFrameData = ovrAudio_AllocSamples(_audioSystem->_frameSize * 2); //TODO: Don't hardcode number of output channels (2) here
		_tempFrameData = ovrAudio_AllocSamples(_audioSystem->_frameSize * 2); //TODO: Don't hardcode number of output channels (2) here

		_instance = this;
		UpdateScene();
		
		_audioSystem->Retain();
		_audioSystem->SetAudioCallback(AudioCallback);
	}
		
	OculusAudioWorld::~OculusAudioWorld()
	{
		SafeRelease(_audioSources);
		SafeRelease(_audioPlayers);
		
		_audioSystem->Release();

		if(_sharedFrameData)
		{
			ovrAudio_FreeSamples(_sharedFrameData);
			_sharedFrameData = nullptr;
		}
		
		if(_tempFrameData)
		{
			ovrAudio_FreeSamples(_tempFrameData);
			_tempFrameData = nullptr;
		}
		
		if(_isSourceAvailable)
		{
			delete[] _isSourceAvailable;
			_isSourceAvailable = nullptr;
		}
		
		ovrAudio_DestroyContext(_internals->oculusAudioContext);
		ovrAudio_Shutdown();

		_instance = nullptr;
		delete _internals;
	}
	
	void OculusAudioWorld::SetSimpleRoom(float width, float height, float depth, float reflectionConstant)
	{
		ovrAudioBoxRoomParameters brp = {};
		brp.brp_Size = sizeof(brp);
		brp.brp_ReflectLeft = brp.brp_ReflectRight = brp.brp_ReflectUp = brp.brp_ReflectDown = brp.brp_ReflectFront = brp.brp_ReflectBehind = reflectionConstant;
		brp.brp_Width = width;
		brp.brp_Height = height;
		brp.brp_Depth = depth;
		ovrAudio_SetSimpleBoxRoomParameters(_internals->oculusAudioContext, &brp);
	}
	
	void OculusAudioWorld::SetRaycastCallback(const std::function<void (Vector3, Vector3, Vector3 &, Vector3 &)> &raycastCallback)
	{
		_raycastCallback = raycastCallback;
		if(raycastCallback)
			ovrAudio_AssignRaycastCallback(_internals->oculusAudioContext, &RaycastCallback, nullptr);
		else
			ovrAudio_AssignRaycastCallback(_internals->oculusAudioContext, nullptr, nullptr);
	}

	void OculusAudioWorld::AddMaterial(const OculusAudioMaterial &material)
	{
		_sceneMaterials.push_back(material);
	}

	void OculusAudioWorld::AddStaticGeometry(const OculusAudioGeometry &geometry)
	{
		_sceneGeometry.push_back(geometry);
	}

	void OculusAudioWorld::UpdateScene()
	{
		/*
		 
		 OVRA_EXPORT ovrResult ovrAudio_SetPropagationQuality(ovrAudioContext context, float quality);
		 OVRA_EXPORT ovrResult ovrAudio_SetPropagationThreadAffinity(ovrAudioContext context, uint64_t cpuMask);
		 OVRA_EXPORT ovrResult ovrAudio_CreateAudioGeometry(ovrAudioContext context, ovrAudioGeometry* geometry);
		 OVRA_EXPORT ovrResult ovrAudio_DestroyAudioGeometry(ovrAudioGeometry geometry);
		 
		 OVRA_EXPORT ovrResult ovrAudio_AudioGeometryUploadMesh(ovrAudioGeometry geometry, const ovrAudioMesh* mesh);
		OVRA_EXPORT ovrResult ovrAudio_AudioGeometryUploadMeshArrays(ovrAudioGeometry geometry,
																	 const void* vertices, size_t verticesByteOffset, size_t vertexCount, size_t vertexStride, ovrAudioScalarType vertexType,
																	 const void* indices, size_t indicesByteOffset, size_t indexCount, ovrAudioScalarType indexType,
																	 const ovrAudioMeshGroup* groups, size_t groupCount);
		
		OVRA_EXPORT ovrResult ovrAudio_AudioGeometrySetTransform(ovrAudioGeometry geometry, const float matrix4x4[16]);
		OVRA_EXPORT ovrResult ovrAudio_AudioGeometryGetTransform(const ovrAudioGeometry geometry, float matrix4x4[16]);
		
		OVRA_EXPORT ovrResult ovrAudio_AudioGeometryWriteMeshFile(const ovrAudioGeometry geometry, const char *filePath);
		OVRA_EXPORT ovrResult ovrAudio_AudioGeometryReadMeshFile(ovrAudioGeometry geometry, const char *filePath);
		
		OVRA_EXPORT ovrResult ovrAudio_AudioGeometryWriteMeshFileObj(const ovrAudioGeometry geometry, const char *filePath);
		*/
		
		/* Material API */
		/*
		OVRA_EXPORT ovrResult ovrAudio_CreateAudioMaterial(ovrAudioContext context, ovrAudioMaterial* material);
		OVRA_EXPORT ovrResult ovrAudio_DestroyAudioMaterial(ovrAudioMaterial material);
		
		OVRA_EXPORT ovrResult ovrAudio_AudioMaterialSetFrequency(ovrAudioMaterial material, ovrAudioMaterialProperty property, float frequency, float value);
		OVRA_EXPORT ovrResult ovrAudio_AudioMaterialGetFrequency(const ovrAudioMaterial material, ovrAudioMaterialProperty property, float frequency, float* value);
		OVRA_EXPORT ovrResult ovrAudio_AudioMaterialReset(ovrAudioMaterial material, ovrAudioMaterialProperty property);
		*/
		
		/* Serialization API */
		/*
		OVRA_EXPORT ovrResult ovrAudio_AudioGeometryWriteMeshData(const ovrAudioGeometry geometry, const ovrAudioSerializer* serializer);
		OVRA_EXPORT ovrResult ovrAudio_AudioGeometryReadMeshData(ovrAudioGeometry geometry, const ovrAudioSerializer* serializer);
		*/
	}
		
	void OculusAudioWorld::Update(float delta)
	{
		//Update listener position
		if(_listener)
		{
			Vector3 listenerPosition = _listener->GetWorldPosition();
			Vector3 listenerForward = _listener->GetForward();
			Vector3 listenerUp = _listener->GetUp();
			ovrAudio_SetListenerVectors(_internals->oculusAudioContext, listenerPosition.x, listenerPosition.y, listenerPosition.z, listenerForward.x, listenerForward.y, listenerForward.z, listenerUp.x, listenerUp.y, listenerUp.z);
		}
		
		//Update audio source properties
		if(_audioSources->GetCount() > 0)
		{
			_audioSources->Enumerate<OculusAudioSource>([&](OculusAudioSource *source, size_t index, bool &stop) {
				if(!source->IsPlaying() || source->GetWorldPosition().GetDistance(_listener->GetWorldPosition()) > source->_minMaxRange.y)
				{
					if(source->_oculusAudioSourceIndex != -1)
					{
						ReleaseSourceIndex(source->_oculusAudioSourceIndex);
						source->_oculusAudioSourceIndex = -1;
					}
					return;
				}
				
				if(source->_oculusAudioSourceIndex == -1)
				{
					source->_oculusAudioSourceIndex = RetainSourceIndex();
				}
				
				if(source->_oculusAudioSourceIndex == -1)
				{
					//Too many active audio sources
					return;
				}
				
				uint32_t sourceFlags = 0;
				if(source->HasTimeOfFlight()) sourceFlags |= ovrAudioSourceFlag_DirectTimeOfArrival;
				ovrAudio_SetAudioSourceFlags(_internals->oculusAudioContext, source->_oculusAudioSourceIndex, sourceFlags);
				
				ovrAudio_SetAudioSourceAttenuationMode(_internals->oculusAudioContext, source->_oculusAudioSourceIndex, ovrAudioSourceAttenuationMode_InverseSquare, 1.0f);
				
				ovrAudio_SetAudioSourceRadius(_internals->oculusAudioContext, source->_oculusAudioSourceIndex, source->_radius);
				
				// Set the sound's position in space (using OVR coordinates)
				// NOTE: if a pose state has been specified by a previous call to
				// ovrAudio_ListenerPoseStatef then it will be transformed
				// by that as well
				RN::Vector3 sourcePosition = source->GetWorldPosition();
				ovrAudio_SetAudioSourcePos(_internals->oculusAudioContext, source->_oculusAudioSourceIndex, sourcePosition.x, sourcePosition.y, sourcePosition.z);
				
				// This sets the attenuation range from max volume to silent
				// NOTE: attenuation can be disabled or enabled
				ovrAudio_SetAudioSourceRange(_internals->oculusAudioContext, source->_oculusAudioSourceIndex, source->_minMaxRange.x, source->_minMaxRange.y);
			});
		}
		
		if(_raycastCallback)
			ovrAudio_UpdateRoomModel(_internals->oculusAudioContext, 0.2f);
	}
	
	void OculusAudioWorld::SetInputBuffer(AudioAsset *inputBuffer)
	{
		RN_ASSERT(!inputBuffer || (inputBuffer->GetData()->GetLength() > 2 * _audioSystem->_frameSize), "Requires an input buffer big enough to contain two frames of audio data!");
		
		SafeRelease(_inputBuffer);
		_inputBuffer = inputBuffer;
		SafeRetain(_inputBuffer);
	}
		
	void OculusAudioWorld::SetListener(SceneNode *listener)
	{
		if(_listener)
			_listener->Release();
			
		_listener = nullptr;

		if(listener)
			_listener = listener->Retain();
	}
		
	OculusAudioPlayer *OculusAudioWorld::PlaySound(AudioAsset *resource) const
	{
		OculusAudioPlayer *player = new OculusAudioPlayer(resource);
		player->Play();

		return player->Autorelease();
	}

	void OculusAudioWorld::AddAudioSource(OculusAudioSource *source) const
	{
		_audioSources->AddObject(source);
	}

	void OculusAudioWorld::RemoveAudioSource(OculusAudioSource *source) const
	{
		_audioSources->RemoveObject(source);
	}

	void OculusAudioWorld::AddAudioPlayer(OculusAudioPlayer *player) const
	{
		_audioPlayers->AddObject(player);
	}

	void OculusAudioWorld::RemoveAudioPlayer(OculusAudioPlayer *player) const
	{
		_audioPlayers->RemoveObject(player);
	}
	
	void OculusAudioWorld::SetCustomWriteCallback(const std::function<void (double)> &customWriteCallback)
	{
		_customWriteCallback = customWriteCallback;
	}
	
	uint32 OculusAudioWorld::RetainSourceIndex()
	{
		for(uint32 i = 0; i < _maxSourceCount; i++)
		{
			if(_isSourceAvailable[i])
			{
				_isSourceAvailable[i] = false;
				return i;
			}
		}
		
		return -1;
	}
	
	void OculusAudioWorld::ReleaseSourceIndex(uint32 index)
	{
		_isSourceAvailable[index] = true;
		ovrAudio_ResetAudioSource(_internals->oculusAudioContext, index);
	}
}
