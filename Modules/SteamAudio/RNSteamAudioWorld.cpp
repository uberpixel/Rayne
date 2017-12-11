//
//  RNSteamAudioWorld.cpp
//  Rayne-SteamAudio
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSteamAudioWorld.h"
#include "RNSteamAudioInternals.h"

#include "soundio/soundio.h"
#include "phonon.h"

namespace RN
{
	RNDefineMeta(SteamAudioDevice, Object)
	RNDefineMeta(SteamAudioWorld, SceneAttachment)

	SteamAudioWorld *SteamAudioWorld::_instance = nullptr;

	SteamAudioWorld* SteamAudioWorld::GetInstance()
	{
		return _instance;
	}

	void SteamAudioWorld::ReadCallback(struct SoundIoInStream *inStream, int minSampleCount, int maxSampleCount)
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
					for(int channel = 0; channel < 1/*inStream->layout.channel_count*/; channel += 1)
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
	
	void SteamAudioWorld::WriteCallback(struct SoundIoOutStream *outStream, int minSampleCount, int maxSampleCount)
	{
		if(!_instance || _instance->_isUpdatingScene)
			return;
		
		struct SoundIoChannelArea *areas;
		int remainingSamples = std::max(static_cast<int>(_instance->_frameSize), minSampleCount);
		
		while(remainingSamples > 0)
		{
			int sampleCount = remainingSamples;
			
			if(soundio_outstream_begin_write(outStream, &areas, &sampleCount) > 0)
				break;
			if(!sampleCount)
				break;

			const struct SoundIoChannelLayout *layout = &outStream->layout;
			float sampleLength = 1.0f / static_cast<float>(outStream->sample_rate);
			int currentSampleCount = 0;//std::max(std::min(static_cast<int>(_instance->_frameSize), maxSampleCount), minSampleCount);
			int processedSampleCount = 0;
			
			while(processedSampleCount < sampleCount)
			{
				currentSampleCount = std::min(static_cast<int>(_instance->_frameSize), sampleCount - processedSampleCount);
				float secondsPerFrame = sampleLength * currentSampleCount;

				IPLAudioBuffer mixingBuffer[3];
				mixingBuffer[0].format = _instance->_internals->internalAmbisonicsFormat;
				mixingBuffer[1].format = _instance->_internals->internalAmbisonicsFormat;
				mixingBuffer[2].format = _instance->_internals->internalAmbisonicsFormat;
				mixingBuffer[0].numSamples = currentSampleCount;
				mixingBuffer[1].numSamples = currentSampleCount;
				mixingBuffer[2].numSamples = currentSampleCount;
				mixingBuffer[1].interleavedBuffer = _instance->_mixedAmbisonicsFrameData0;
				mixingBuffer[2].interleavedBuffer = _instance->_mixedAmbisonicsFrameData1;

				Vector3 listenerPosition;
				Vector3 listenerForward(0.0f, 0.0f, -1.0f);
				Vector3 listenerUp(0.0f, 1.0f, 0.0f);
				SceneNode *listener = SteamAudioWorld::_instance->_listener;
				if(listener)
				{
					listenerPosition = listener->GetWorldPosition();
					listenerForward = listener->GetForward();
					listenerUp = listener->GetUp();
				}

				//Get indirect audio samples if possible
				if(_instance->_scene)
				{
					iplGetMixedEnvironmentalAudio(_instance->_environmentalRenderer,
						IPLVector3{ listenerPosition.x, listenerPosition.y, listenerPosition.z },
						IPLVector3{ listenerForward.x, listenerForward.y, listenerForward.z },
						IPLVector3{ listenerUp.x, listenerUp.y, listenerUp.z }, mixingBuffer[1]);
				}
				else
				{
					memset(_instance->_mixedAmbisonicsFrameData0, 0, sizeof(float) * currentSampleCount * _instance->_internals->internalAmbisonicsFormat.numSpeakers);
				}

				//Get direct audio samples and mix
				if(_instance->_environmentalRenderer)
				{
					_instance->_audioSources->Enumerate<SteamAudioSource>([&](SteamAudioSource *source, size_t index, bool &stop) {
						if(!source->IsPlaying())
							return;

						float *outData = nullptr;
						source->Update(secondsPerFrame, currentSampleCount, &outData);

						if(!outData)
							return;

						mixingBuffer[0].interleavedBuffer = outData;
						iplMixAudioBuffers(2, mixingBuffer, mixingBuffer[2]);
						float *tempPointer = mixingBuffer[2].interleavedBuffer;
						mixingBuffer[2].interleavedBuffer = mixingBuffer[1].interleavedBuffer;
						mixingBuffer[1].interleavedBuffer = tempPointer;
					});
				}

				//Turn ambisonics data into binaural stereo data TODO: Also support ambisonic panning effect here!
				IPLAudioBuffer outputBuffer;
				outputBuffer.format = _instance->_internals->outputFormat;
				outputBuffer.numSamples = currentSampleCount;
				outputBuffer.interleavedBuffer = _instance->_outputFrameData;
				iplApplyAmbisonicsBinauralEffect(_instance->_ambisonicsBinauralEffect, mixingBuffer[1], outputBuffer);

				//Mix final output with audio players
				if(_instance->_audioPlayers->GetCount() > 0)
				{
					mixingBuffer[0].format = _instance->_internals->outputFormat;
					mixingBuffer[1].format = _instance->_internals->outputFormat;
					mixingBuffer[2].format = _instance->_internals->outputFormat;
					mixingBuffer[1].interleavedBuffer = _instance->_outputFrameData;

					_instance->_audioPlayers->Enumerate<SteamAudioPlayer>([&](SteamAudioPlayer *source, size_t index, bool &stop) {
						if(!source->IsPlaying())
							return;

						float *outData = nullptr;
						source->Update(secondsPerFrame, currentSampleCount, &outData);

						if(!outData)
							return;

						mixingBuffer[0].interleavedBuffer = outData;
						iplMixAudioBuffers(2, mixingBuffer, mixingBuffer[2]);
						float *tempPointer = mixingBuffer[2].interleavedBuffer;
						mixingBuffer[2].interleavedBuffer = mixingBuffer[1].interleavedBuffer;
						mixingBuffer[1].interleavedBuffer = tempPointer;
					});

					memcpy(_instance->_outputFrameData, mixingBuffer[1].interleavedBuffer, layout->channel_count * currentSampleCount * sizeof(float));
				}
				
				//Write audio data to the device
				int actualSample = 0;
				for(int sample = processedSampleCount; sample < processedSampleCount+currentSampleCount; sample++)
				{
					for(int channel = 0; channel < layout->channel_count; channel++)
					{
						float *ptr = reinterpret_cast<float*>(areas[channel].ptr + areas[channel].step * sample);
						*ptr = _instance->_outputFrameData[actualSample * layout->channel_count + channel];
					}
					actualSample++;
				}
				
				processedSampleCount += currentSampleCount;
			}
			
			soundio_outstream_end_write(outStream);
			remainingSamples -= sampleCount;
		}
	}

	//TODO: Allow to initialize with preferred device names and fall back to defaults
	SteamAudioWorld::SteamAudioWorld(SteamAudioDevice *outputDevice, uint8 ambisonicsOrder, uint32 sampleRate, uint32 frameSize) :
		_listener(nullptr),
		_inDevice(nullptr),
		_outDevice(nullptr),
		_inStream(nullptr),
		_outStream(nullptr),
		_ambisonicsOrder(ambisonicsOrder),
		_isUpdatingScene(true),
		_scene(nullptr),
		_sceneMesh(nullptr),
		_environment(nullptr),
		_ambisonicsBinauralEffect(nullptr),
		_environmentalRenderer(nullptr),
		_inputBuffer(nullptr),
		_audioSources(new Array()),
		_audioPlayers(new Array()),
		_frameSize(frameSize),
		_sampleRate(sampleRate),
		_mixedAmbisonicsFrameData0(nullptr),
		_mixedAmbisonicsFrameData1(nullptr),
		_outputFrameData(nullptr),
		_internals(new SteamAudioWorldInternals())
	{
		RN_ASSERT(!_instance, "There already is a SteamAudioWorld!");
		RN_ASSERT(!outputDevice || outputDevice->type == SteamAudioDevice::Type::Output, "outputDevice has to be an output device!");

		//Initialize libsoundio
		_soundio = soundio_create();
		soundio_connect(_soundio);
		soundio_flush_events(_soundio);

		SetOutputDevice(outputDevice);

		//Initialize Steam Audio
		iplCreateContext(nullptr, nullptr, nullptr, &_internals->context);
		
		_internals->settings.samplingRate = sampleRate;
		_internals->settings.frameSize = frameSize;
		_internals->settings.convolutionType = IPL_CONVOLUTIONTYPE_PHONON;

		IPLHrtfParams hrtfParams{IPL_HRTFDATABASETYPE_DEFAULT, nullptr, 0, nullptr, nullptr};
		iplCreateBinauralRenderer(_internals->context, _internals->settings, hrtfParams, &_binauralRenderer); //TODO: HRTF is only a good idea with headphones, add alternative

		_internals->internalAmbisonicsFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_AMBISONICS;
		_internals->internalAmbisonicsFormat.numSpeakers = (ambisonicsOrder + 1) * (ambisonicsOrder + 1);
		_internals->internalAmbisonicsFormat.ambisonicsOrder = ambisonicsOrder;
		_internals->internalAmbisonicsFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;
		_internals->internalAmbisonicsFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		_internals->internalAmbisonicsFormat.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		_internals->outputFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
		_internals->outputFormat.channelLayout = IPL_CHANNELLAYOUT_STEREO;
		_internals->outputFormat.numSpeakers = 2;
		_internals->outputFormat.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

		iplCreateAmbisonicsBinauralEffect(_binauralRenderer, _internals->internalAmbisonicsFormat, _internals->outputFormat, &_ambisonicsBinauralEffect);

		_mixedAmbisonicsFrameData0 = new float[_frameSize * std::max(_internals->internalAmbisonicsFormat.numSpeakers, 2)]; //TODO: Don't hardcode maximum number of supported output channels (2) here
		_mixedAmbisonicsFrameData1 = new float[_frameSize * std::max(_internals->internalAmbisonicsFormat.numSpeakers, 2)]; //TODO: Don't hardcode maximum number of supported output channels (2) here
		_outputFrameData = new float[_frameSize * 2]; //TODO: Don't hardcode maximum number of supported output channels (2) here
		_inputFrameData = new float[_frameSize * 1]; //TODO: Don't hardcode maximum number of supported input channels (1) here

		_sharedSourceInputFrameData = new float[_frameSize];
		_sharedSourceOutputFrameData = new float[_frameSize * std::max(_internals->internalAmbisonicsFormat.numSpeakers, 2)]; //TODO: Don't hardcode maximum number of supported output channels (2) here

		_instance = this;
		UpdateScene();
	}
		
	SteamAudioWorld::~SteamAudioWorld()
	{
		SafeRelease(_audioSources);
		SafeRelease(_audioPlayers);

		if(_inStream)
			soundio_instream_destroy(_inStream);

		if(_inDevice)
			soundio_device_unref(_inDevice);

		if(_outStream)
			soundio_outstream_destroy(_outStream);

		if(_outDevice)
			soundio_device_unref(_outDevice);

		if(_soundio)
			soundio_destroy(_soundio);

		if(_environmentalRenderer)
		{
			iplDestroyEnvironmentalRenderer(&_environmentalRenderer);
		}

		if(_environment)
		{
			iplDestroyEnvironment(&_environment);
		}

		if (_sceneMesh)
		{
			iplDestroyStaticMesh(&_sceneMesh);
			_sceneMesh = nullptr;
		}

		if(_scene)
		{
			iplDestroyScene(&_scene);
		}

		if(_ambisonicsBinauralEffect)
		{
			iplDestroyAmbisonicsBinauralEffect(&_ambisonicsBinauralEffect);
		}

		iplDestroyBinauralRenderer(&_binauralRenderer);

		if(_frameSize > 0)
		{
			delete[] _mixedAmbisonicsFrameData0;
			delete[] _mixedAmbisonicsFrameData1;
			delete[] _outputFrameData;
			delete[] _inputFrameData;
			delete[] _sharedSourceInputFrameData;
			delete[] _sharedSourceOutputFrameData;

			_mixedAmbisonicsFrameData0 = nullptr;
			_mixedAmbisonicsFrameData1 = nullptr;
			_outputFrameData = nullptr;
			_inputFrameData = nullptr;
			_sharedSourceInputFrameData = nullptr;
			_sharedSourceOutputFrameData = nullptr;

			_frameSize = 0;
		}
		
		iplDestroyContext(&_internals->context);

		_instance = nullptr;
		delete _internals;
	}

	void SteamAudioWorld::SetOutputDevice(SteamAudioDevice *outputDevice)
	{
		RN_ASSERT(!outputDevice || outputDevice->type == SteamAudioDevice::Type::Output, "Not an output device!");

		if(_outStream)
		{
			soundio_outstream_destroy(_outStream);
			_outStream = nullptr;
		}

		if(_outDevice)
		{
			soundio_device_unref(_outDevice);
			_outDevice = nullptr;
		}

		if(!outputDevice)
			return;


		int deviceIndex = outputDevice->index;
		if(deviceIndex < 0)
		{
			RNDebug("Not a valid audio device.");
			return;
		}

		_outDevice = soundio_get_output_device(_soundio, deviceIndex);
		if(!_outDevice)
		{
			RNDebug("Failed opening audio device.");
			return;
		}

		RNInfo("Using audio device: " << _outDevice->name);

		_outStream = soundio_outstream_create(_outDevice);
		_outStream->format = SoundIoFormatFloat32NE;
		_outStream->sample_rate = _sampleRate;
		_outStream->software_latency = _frameSize / static_cast<float>(_sampleRate);
		_outStream->write_callback = WriteCallback;

		int err;
		if((err = soundio_outstream_open(_outStream)))
		{
			RNDebug("Failed opening audio device with error:" << soundio_strerror(err));
			return;
		}

		if(_outStream->layout_error)
			RNDebug("Unable to set channel layout with error:" << soundio_strerror(_outStream->layout_error));

		if((err = soundio_outstream_start(_outStream)))
		{
			RNDebug("Failed opening audio device with error:" << soundio_strerror(err));
			return;
		}
	}

	void SteamAudioWorld::SetInputDevice(SteamAudioDevice *inputDevice, AudioAsset *targetAsset)
	{
		RN_ASSERT(!inputDevice || inputDevice->type == SteamAudioDevice::Type::Input, "Not an input device!");
		RN_ASSERT(!targetAsset || (targetAsset->GetData()->GetLength() > 2 * _frameSize), "Requires an input buffer big enough to contain two frames of audio data!");

		if(_inStream)
		{
			soundio_instream_destroy(_inStream);
			_inStream = nullptr;
		}

		if(_inDevice)
		{
			soundio_device_unref(_inDevice);
			_inDevice = nullptr;
		}

		SafeRelease(_inputBuffer);

		if(!inputDevice)
			return;

		_inputBuffer = targetAsset->Retain();

		int deviceIndex = inputDevice->index;
		if(deviceIndex < 0)
		{
			RNDebug("Not a valid audio device.");
			return;
		}

		_inDevice = soundio_get_input_device(_soundio, deviceIndex);
		if(!_inDevice)
		{
			RNDebug("Failed opening audio device.");
			return;
		}

		RNInfo("Using audio device: " << _inDevice->name);

		_inStream = soundio_instream_create(_inDevice);
		_inStream->format = SoundIoFormatFloat32NE;
		_inStream->sample_rate = _sampleRate;
		_inStream->layout = _inDevice->current_layout;
		_inStream->software_latency = _frameSize/static_cast<float>(_sampleRate);
		_inStream->read_callback = ReadCallback;

		int err;
		if((err = soundio_instream_open(_inStream)))
		{
			RNDebug("Failed opening audio device with error:" << soundio_strerror(err));
			return;
		}

		if(_inStream->layout_error)
			RNDebug("Unable to set channel layout with error:" << soundio_strerror(_inStream->layout_error));

		if((err = soundio_instream_start(_inStream)))
		{
			RNDebug("Failed opening audio device with error:" << soundio_strerror(err));
			return;
		}
	}

	void SteamAudioWorld::AddMaterial(const SteamAudioMaterial &material)
	{
		_sceneMaterials.push_back(material);
	}

	void SteamAudioWorld::AddStaticGeometry(const SteamAudioGeometry &geometry)
	{
		_sceneGeometry.push_back(geometry);
	}

	void SteamAudioWorld::UpdateScene()
	{
		_isUpdatingScene = true;

		_audioSources->Enumerate<SteamAudioSource>([&](SteamAudioSource *source, size_t index, bool &stop) {
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
		simulationSettings.maxConvolutionSources = 0;
		simulationSettings.numBounces = 16;
		simulationSettings.numDiffuseSamples = 512;
		simulationSettings.numRays = 64000;
		simulationSettings.sceneType = IPL_SCENETYPE_PHONON;

		if(_sceneGeometry.size() > 0)
		{
			iplCreateScene(_internals->context, nullptr, simulationSettings, _sceneMaterials.size(), &_scene);
			//TODO: Create scene!

			int counter = 0;
			for(const SteamAudioMaterial &material : _sceneMaterials)
			{
				iplSetSceneMaterial(_scene, counter++, IPLMaterial{ material.lowFrequencyAbsorption, material.midFrequencyAbsorption, material.highFrequencyAbsorption, material.scattering });
			}

			std::vector<IPLVector3> vertices;
			std::vector<IPLTriangle> triangles;
			std::vector<IPLint32> materials;

			for(const SteamAudioGeometry &geometry : _sceneGeometry)
			{
				const Mesh::VertexAttribute *vertexAttribute = geometry.mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
				RN_ASSERT(vertexAttribute && vertexAttribute->GetType() == PrimitiveType::Vector3, "SteamAudioGeometry mesh has an unsupported vertex format.");

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

		_audioSources->Enumerate<SteamAudioSource>([&](SteamAudioSource *source, size_t index, bool &stop) {
			source->FinalizeScene();
		});

		_isUpdatingScene = false;
	}
		
	void SteamAudioWorld::Update(float delta)
	{
//		soundio_flush_events(_soundio);	//TODO: Call this and handle changing devices
	}
		
	void SteamAudioWorld::SetListener(SceneNode *listener)
	{
		if(_listener)
			_listener->Release();
			
		_listener = nullptr;

		if(listener)
			_listener = listener->Retain();
	}
		
	SteamAudioPlayer *SteamAudioWorld::PlaySound(AudioAsset *resource) const
	{
		SteamAudioPlayer *player = new SteamAudioPlayer(resource);
		player->Play();

		return player->Autorelease();
	}

	void SteamAudioWorld::AddAudioSource(SteamAudioSource *source) const
	{
		_audioSources->AddObject(source);
	}

	void SteamAudioWorld::RemoveAudioSource(SteamAudioSource *source) const
	{
		_audioSources->RemoveObject(source);
	}

	void SteamAudioWorld::AddAudioPlayer(SteamAudioPlayer *player) const
	{
		_audioPlayers->AddObject(player);
	}

	void SteamAudioWorld::RemoveAudioPlayer(SteamAudioPlayer *player) const
	{
		_audioPlayers->RemoveObject(player);
	}

	Array *SteamAudioWorld::GetDevices()
	{
		//Initialize libsoundio
		SoundIo *soundio = soundio_create();
		SoundIoDevice *device;
		soundio_connect(soundio);
		soundio_flush_events(soundio);

		Array *deviceArray = new Array();

		//Get all output devices
		int defaultOutputDeviceIndex = soundio_default_output_device_index(soundio);
		int outputDeviceCount = soundio_output_device_count(soundio);
		for (int i = 0; i < outputDeviceCount; i++)
		{
			device = soundio_get_output_device(soundio, i);
			if(device && !device->is_raw)
			{
				SteamAudioDevice *saDevice = new SteamAudioDevice(SteamAudioDevice::Type::Output, i, device->name, device->id, i == defaultOutputDeviceIndex);
				deviceArray->AddObject(saDevice->Autorelease());
				soundio_device_unref(device);
			}
		}

		//Get all input devices
		int defaultInputDeviceIndex = soundio_default_input_device_index(soundio);
		int inputDeviceCount = soundio_input_device_count(soundio);
		for (int i = 0; i < inputDeviceCount; i++)
		{
			device = soundio_get_input_device(soundio, i);
			if (device && !device->is_raw)
			{
				SteamAudioDevice *saDevice = new SteamAudioDevice(SteamAudioDevice::Type::Input, i, device->name, device->id, i == defaultInputDeviceIndex);
				deviceArray->AddObject(saDevice->Autorelease());
				soundio_device_unref(device);
			}
		}

		soundio_destroy(soundio);

		return deviceArray->Autorelease();
	}

	SteamAudioDevice *SteamAudioWorld::GetDefaultInputDevice()
	{
		SoundIo *soundio = soundio_create();
		soundio_connect(soundio);
		soundio_flush_events(soundio);

		int deviceIndex = soundio_default_input_device_index(soundio);
		if(deviceIndex < 0)
		{
			RNDebug("No audio output device found.");
			return nullptr;
		}

		SteamAudioDevice *saDevice = nullptr;
		SoundIoDevice *device = soundio_get_input_device(soundio, deviceIndex);
		if(device)
		{
			saDevice = new SteamAudioDevice(SteamAudioDevice::Type::Input, deviceIndex, device->name, device->id, true);
			saDevice->Autorelease();
			soundio_device_unref(device);
		}

		soundio_destroy(soundio);

		return saDevice;
	}

	SteamAudioDevice *SteamAudioWorld::GetDefaultOutputDevice()
	{
		SoundIo *soundio = soundio_create();
		soundio_connect(soundio);
		soundio_flush_events(soundio);

		int deviceIndex = soundio_default_output_device_index(soundio);
		if(deviceIndex < 0)
		{
			RNDebug("No audio output device found.");
			return nullptr;
		}

		SteamAudioDevice *saDevice = nullptr;
		SoundIoDevice *device = soundio_get_output_device(soundio, deviceIndex);
		if(device)
		{
			saDevice = new SteamAudioDevice(SteamAudioDevice::Type::Output, deviceIndex, device->name, device->id, true);
			saDevice->Autorelease();
			soundio_device_unref(device);
		}

		soundio_destroy(soundio);

		return saDevice;
	}
}
