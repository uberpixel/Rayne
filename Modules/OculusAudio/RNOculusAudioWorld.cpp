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
	RNDefineMeta(OculusAudioDevice, Object)
	RNDefineMeta(OculusAudioWorld, SceneAttachment)

	OculusAudioWorld *OculusAudioWorld::_instance = nullptr;

	OculusAudioWorld* OculusAudioWorld::GetInstance()
	{
		return _instance;
	}
	
	int OculusAudioWorld::AudioCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void *userData)
	{
		float *buffer = static_cast<float*>(outputBuffer);
		double bufferLength = 1.0/static_cast<double>(_instance->_sampleRate) * nBufferFrames;
		
		memset(buffer, 0, nBufferFrames * 2 * sizeof(float));
		
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
				source->Update(bufferLength, nBufferFrames, &outData);
				
				if(!outData)
					return;

				// Spatialize the sound into the output buffer.  Note that there
				// are two APIs, one for interleaved sample data and another for
				// separate left/right sample data
				ovrAudio_SpatializeMonoSourceInterleaved(_instance->_internals->oculusAudioContext, source->_oculusAudioSourceIndex, &oculusAudioStatus, _instance->_tempFrameData, outData);
				
				for(int i = 0; i < nBufferFrames; i++)
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
				source->Update(bufferLength, nBufferFrames, &outData);
				
				if(!outData)
					return;
				
				for(int i = 0; i < nBufferFrames; i++)
				{
					for(int j = 0; j < 2; j++)
					{
						buffer[i*2 + j] += outData[i*2 + j];
					}
				}
			});
		}
		
		return 0;
	}

/*	void OculusAudioWorld::ReadCallback(struct SoundIoInStream *inStream, int minSampleCount, int maxSampleCount)
	{
		if(!_instance)
			return;

		struct SoundIoChannelArea *areas;
		int err;
		int remainingSamples = maxSampleCount;
		while(remainingSamples > 0)
		{
			int sampleCount = std::min(std::max(static_cast<int>(_instance->_frameSize), minSampleCount), remainingSamples);
			char *data = reinterpret_cast<char *>(_instance->_inputFrameData);

			if(soundio_instream_begin_read(inStream, &areas, &sampleCount) != 0)
				return;

			if(!sampleCount)
				break;

			if(!areas)
			{
				// Due to an overflow there is a hole. Fill the ring buffer with
				// silence for the size of the hole.
				memset(_instance->_inputFrameData, 0, sampleCount * inStream->bytes_per_frame);
			}
			else
			{
				for(int sample = 0; sample < sampleCount; sample += 1)
				{
					//TODO: Support multiple input channels
					for(int channel = 0; channel < 1*//*inStream->layout.channel_count*//*; channel += 1)
					{
						
						memcpy(data, areas[channel].ptr, inStream->bytes_per_sample);
						//_instance->_inputFrameData[sample] = *static_cast<float*>(areas[channel].ptr);
						areas[channel].ptr += areas[channel].step;
						data += inStream->bytes_per_sample;
					}
				}
			}

			int err;
			if(err = soundio_instream_end_read(inStream))
			{
				const char *errorstr = soundio_strerror(err);
				return;
			}

			_instance->_inputBuffer->PushData(_instance->_inputFrameData, sampleCount * inStream->bytes_per_sample);
			remainingSamples -= sampleCount;
		}
	}
*/

	//TODO: Allow to initialize with preferred device names and fall back to defaults
	OculusAudioWorld::OculusAudioWorld(OculusAudioDevice *outputDevice, uint32 sampleRate, uint32 frameSize, uint32 maxSources) :
		_listener(nullptr),
		_inputBuffer(nullptr),
		_audioSources(new Array()),
		_audioPlayers(new Array()),
		_frameSize(frameSize),
		_sampleRate(sampleRate),
		_maxSourceCount(maxSources),
		_isSourceAvailable(nullptr),
		_sharedFrameData(nullptr),
		_tempFrameData(nullptr),
		_isUpdatingScene(true),
		_internals(new OculusAudioWorldInternals())
	{
		RN_ASSERT(!_instance, "There already is a OculusAudioWorld!");
		RN_ASSERT(!outputDevice || outputDevice->type == OculusAudioDevice::Type::Output, "outputDevice has to be an output device!");

		// Version checking is not strictly necessary but it's a good idea!
		int major, minor, patch;
		const char *VERSION_STRING;
		
		VERSION_STRING = ovrAudio_GetVersion(&major, &minor, &patch);
		RN_ASSERT(major == OVR_AUDIO_MAJOR_VERSION && minor == OVR_AUDIO_MINOR_VERSION, "Mismatched Oculus Audio SDK version!");
		RNDebug(RNSTR("Using OVRAudio: " << VERSION_STRING));
		
		ovrAudioContextConfiguration config = {};
		config.acc_Size = sizeof(config);
		config.acc_SampleRate = sampleRate;
		config.acc_BufferLength = frameSize;
		config.acc_MaxNumSources = _maxSourceCount;
		
		if(ovrAudio_CreateContext(&_internals->oculusAudioContext, &config) != ovrSuccess)
		{
			RNDebug(RNCSTR("WARNING: Could not create context!"));
			return;
		}
		
		ovrAudio_SetUnitScale(_internals->oculusAudioContext, 1.0f);
		
		ovrAudio_Enable(_internals->oculusAudioContext, ovrAudioEnable_SimpleRoomModeling, 1);
		ovrAudio_Enable(_internals->oculusAudioContext, ovrAudioEnable_LateReverberation, 1);
		ovrAudio_Enable(_internals->oculusAudioContext, ovrAudioEnable_RandomizeReverb, 1);
		
		SetOutputDevice(outputDevice);
		
		_isSourceAvailable = new bool[_maxSourceCount];
		for(int i = 0; i < _maxSourceCount; i++) _isSourceAvailable[i] = true;
		_sharedFrameData = ovrAudio_AllocSamples(_frameSize * 2); //TODO: Don't hardcode number of output channels (2) here
		_tempFrameData = ovrAudio_AllocSamples(_frameSize * 2); //TODO: Don't hardcode number of output channels (2) here

		_instance = this;
		UpdateScene();
	}
		
	OculusAudioWorld::~OculusAudioWorld()
	{
		SafeRelease(_audioSources);
		SafeRelease(_audioPlayers);

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

		_frameSize = 0;
		
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

	void OculusAudioWorld::SetOutputDevice(OculusAudioDevice *outputDevice)
	{
		RN_ASSERT(!outputDevice || outputDevice->type == OculusAudioDevice::Type::Output, "Not an output device!");
		
//		if(_internals->rtAudioContext.isStreamRunning())
//			_internals->rtAudioContext.stopStream();
		
		if(_internals->rtAudioContext.isStreamOpen())
			_internals->rtAudioContext.closeStream();
		
		RtAudio::StreamParameters parameters;
		parameters.deviceId = outputDevice->index;
		parameters.nChannels = 2;
		parameters.firstChannel = 0;
		try
		{
			_internals->rtAudioContext.openStream(&parameters, NULL, RTAUDIO_FLOAT32, _sampleRate, &_frameSize, &AudioCallback, nullptr);
			_internals->rtAudioContext.startStream();
		}
		catch(RtAudioError& e)
		{
			e.printMessage();
			return;
		}

		RNInfo("Using audio device: " << outputDevice->name);
	}

	void OculusAudioWorld::SetInputDevice(OculusAudioDevice *inputDevice, AudioAsset *targetAsset)
	{
		RN_ASSERT(!inputDevice || inputDevice->type == OculusAudioDevice::Type::Input, "Not an input device!");
		RN_ASSERT(!targetAsset || (targetAsset->GetData()->GetLength() > 2 * _frameSize), "Requires an input buffer big enough to contain two frames of audio data!");

//		RNInfo("Using audio device: " << _inDevice->name);
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
/*		_isUpdatingScene = true;

		_audioSources->Enumerate<OculusAudioSource>([&](OculusAudioSource *source, size_t index, bool &stop) {
			source->ResetScene();
		});

		if(_environmentalRenderer)
		{
			iplDestroyEnvironmentalRenderer(&_environmentalRenderer);
			_environmentalRenderer = nullptr;
		}

		if(_environment)
		{
			iplDestroyEnvironment(&_environment);
			_environment = nullptr;
		}

		if(_sceneMesh)
		{
			iplDestroyStaticMesh(&_sceneMesh);
			_sceneMesh = nullptr;
		}

		if (_scene)
		{
			iplDestroyScene(&_scene);
			_scene = nullptr;
		}

		IPLSimulationSettings simulationSettings;
		simulationSettings.ambisonicsOrder = _ambisonicsOrder;
		simulationSettings.irDuration = 0.5f;
		simulationSettings.maxConvolutionSources = 2;
		simulationSettings.numBounces = 16;
		simulationSettings.numDiffuseSamples = 16;
		simulationSettings.numRays = 32000;
		simulationSettings.sceneType = IPL_SCENETYPE_PHONON;

		if(_sceneGeometry.size() > 0)
		{
			iplCreateScene(_internals->context, nullptr, simulationSettings, _sceneMaterials.size(), &_scene);
			//TODO: Create scene!

			int counter = 0;
			for(const OculusAudioMaterial &material : _sceneMaterials)
			{
				iplSetSceneMaterial(_scene, counter++, IPLMaterial{ material.lowFrequencyAbsorption, material.midFrequencyAbsorption, material.highFrequencyAbsorption, material.scattering });
			}

			std::vector<IPLVector3> vertices;
			std::vector<IPLTriangle> triangles;
			std::vector<IPLint32> materials;

			for(const OculusAudioGeometry &geometry : _sceneGeometry)
			{
				const Mesh::VertexAttribute *vertexAttribute = geometry.mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
				RN_ASSERT(vertexAttribute && vertexAttribute->GetType() == PrimitiveType::Vector3, "OculusAudioGeometry mesh has an unsupported vertex format.");

				//Collect vertices
				Mesh::Chunk chunk = geometry.mesh->GetChunk();
				Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);
				for(size_t i = 0; i < geometry.mesh->GetVerticesCount(); i++)
				{
					if(i > 0) vertexIterator++;
					const Vector3 &vertex = *vertexIterator;
					Vector3 transformedVertex = geometry.scale * geometry.rotation.GetRotatedVector(vertex) + geometry.position;
					vertices.push_back(IPLVector3{ transformedVertex.x, transformedVertex.y, transformedVertex.z });
				}

				//Collect triangles and materials
				Mesh::ElementIterator<uint16> indexIterator = chunk.GetIterator<uint16>(Mesh::VertexAttribute::Feature::Indices);
				for(size_t i = 0; i < geometry.mesh->GetIndicesCount()/3; i++)
				{
					if(i > 0) indexIterator++;
					const uint16 index1 = *(indexIterator++);
					const uint16 index2 = *(indexIterator++);
					const uint16 index3 = *(indexIterator);

					triangles.push_back(IPLTriangle{ {static_cast<IPLint32>(index1), static_cast<IPLint32>(index2), static_cast<IPLint32>(index3)} });
					materials.push_back(geometry.materialIndex);
				}
			}

			iplCreateStaticMesh(_scene, vertices.size(), triangles.size(), &_sceneMesh);
			iplSetStaticMeshVertices(_scene, _sceneMesh, &vertices[0]);
			iplSetStaticMeshTriangles(_scene, _sceneMesh, &triangles[0]);
			iplSetStaticMeshMaterials(_scene, _sceneMesh, &materials[0]);

			iplFinalizeScene(_scene, nullptr);
		}

		_sceneMaterials.clear();
		_sceneGeometry.clear();

		iplCreateEnvironment(_internals->context, nullptr, simulationSettings, _scene, nullptr, &_environment);	//TODO: 4th paramter should be a scene object for indirect sound modeling
		iplCreateEnvironmentalRenderer(_internals->context, _environment, _internals->settings, _internals->internalAmbisonicsFormat, nullptr, nullptr, &_environmentalRenderer);

		_audioSources->Enumerate<OculusAudioSource>([&](OculusAudioSource *source, size_t index, bool &stop) {
			source->FinalizeScene();
		});

		_isUpdatingScene = false;*/
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
				if(!source->IsPlaying() || source->GetWorldPosition().GetDistance(_instance->_listener->GetWorldPosition()) > source->_minMaxRange.y)
				{
					if(source->_oculusAudioSourceIndex != -1)
					{
						_instance->ReleaseSourceIndex(source->_oculusAudioSourceIndex);
						source->_oculusAudioSourceIndex = -1;
					}
					return;
				}
				
				if(source->_oculusAudioSourceIndex == -1)
				{
					source->_oculusAudioSourceIndex = _instance->RetainSourceIndex();
				}
				
				if(source->_oculusAudioSourceIndex == -1)
				{
					//Too many active audio sources
					return;
				}
				
				ovrAudio_SetAudioSourceFlags(_instance->_internals->oculusAudioContext, source->_oculusAudioSourceIndex, ovrAudioSourceFlag_DirectTimeOfArrival);
				
				ovrAudio_SetAudioSourceAttenuationMode(_instance->_internals->oculusAudioContext, source->_oculusAudioSourceIndex, ovrAudioSourceAttenuationMode_InverseSquare, 1.0f);
				
				// Set the sound's position in space (using OVR coordinates)
				// NOTE: if a pose state has been specified by a previous call to
				// ovrAudio_ListenerPoseStatef then it will be transformed
				// by that as well
				RN::Vector3 sourcePosition = source->GetWorldPosition();
				ovrAudio_SetAudioSourcePos(_instance->_internals->oculusAudioContext, source->_oculusAudioSourceIndex, sourcePosition.x, sourcePosition.y, sourcePosition.z);
				
				// This sets the attenuation range from max volume to silent
				// NOTE: attenuation can be disabled or enabled
				ovrAudio_SetAudioSourceRange(_instance->_internals->oculusAudioContext, source->_oculusAudioSourceIndex, source->_minMaxRange.x, source->_minMaxRange.y);
			});
		}
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

	Array *OculusAudioWorld::GetDevices()
	{
		RtAudio audioContext;
		unsigned int devices = audioContext.getDeviceCount();
		
		Array *deviceArray = new Array();
		
		RtAudio::DeviceInfo info;
		for(unsigned int i = 0; i < devices; i++)
		{
			info = audioContext.getDeviceInfo(i);
			if(info.probed == true)
			{
				if(info.outputChannels > 0)
				{
					OculusAudioDevice *outputDevice = new OculusAudioDevice(OculusAudioDevice::Type::Output, i, info.name, info.isDefaultOutput);
					deviceArray->AddObject(outputDevice->Autorelease());
				}
				
				if(info.inputChannels > 0)
				{
					OculusAudioDevice *inputDevice = new OculusAudioDevice(OculusAudioDevice::Type::Input, i, info.name, info.isDefaultInput);
					deviceArray->AddObject(inputDevice->Autorelease());
				}
			}
		}
		
		return deviceArray->Autorelease();
	}

	OculusAudioDevice *OculusAudioWorld::GetDefaultOutputDevice()
	{
		RtAudio audioContext;
		unsigned int devices = audioContext.getDeviceCount();
		
		RtAudio::DeviceInfo info;
		for(unsigned int i = 0; i < devices; i++)
		{
			info = audioContext.getDeviceInfo(i);
			if(info.probed == true && info.isDefaultOutput)
			{
				OculusAudioDevice *outputDevice = new OculusAudioDevice(OculusAudioDevice::Type::Output, i, info.name, info.isDefaultOutput);
				return outputDevice->Autorelease();
			}
		}
		
		RNDebug("No default audio output device found.");
		return nullptr;
	}

	OculusAudioDevice *OculusAudioWorld::GetDefaultInputDevice()
	{
		RtAudio audioContext;
		unsigned int devices = audioContext.getDeviceCount();
		
		RtAudio::DeviceInfo info;
		for(unsigned int i = 0; i < devices; i++)
		{
			info = audioContext.getDeviceInfo(i);
			if(info.probed == true && info.isDefaultInput)
			{
				OculusAudioDevice *inputDevice = new OculusAudioDevice(OculusAudioDevice::Type::Input, i, info.name, info.isDefaultInput);
				return inputDevice->Autorelease();
			}
		}
		
		RNDebug("No default audio output device found.");
		return nullptr;
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
