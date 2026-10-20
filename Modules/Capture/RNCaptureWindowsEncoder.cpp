//
//  RNCaptureWindowsEncoder.cpp
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCaptureWindowsEncoder.h"

#if RN_PLATFORM_WINDOWS

namespace RN
{
	using Microsoft::WRL::ComPtr;

	namespace CaptureEncoderDetails
	{
		int GetAACSamplingFrequencyIndex(uint32_t sampleRate)
		{
			static constexpr std::array<uint32_t, 13> sampleRates = {
			96000,
			88200,
			64000,
			48000,
			44100,
			32000,
			24000,
			22050,
			16000,
			12000,
			11025,
			8000,
			7350};

			for(size_t i = 0; i < sampleRates.size(); i++)
			{
				if(sampleRates[i] == sampleRate) return static_cast<int>(i);
			}

			return 3;
		}

		void BuildAACAudioSpecificConfig(std::vector<uint8> &config)
		{
			const int audioObjectType = 2;
			const int sampleRateIndex = GetAACSamplingFrequencyIndex(CaptureWindowsInternals::AudioSampleRate);
			const int channelCount = CaptureWindowsInternals::AudioChannelCount;

			config.resize(2);
			config[0] = static_cast<uint8>((audioObjectType << 3) | (sampleRateIndex >> 1));
			config[1] = static_cast<uint8>(((sampleRateIndex & 1) << 7) | (channelCount << 3));
		}

		DWORD GetDefaultMFTOutputBufferSize(bool isVideo, UINT32 width, UINT32 height)
		{
			if(isVideo) return std::max<DWORD>(1024 * 1024, static_cast<DWORD>(width * height));
			return 64 * 1024;
		}
	} // namespace CaptureEncoderDetails

	using namespace CaptureEncoderDetails;

	CaptureWindowsEncoder::CaptureWindowsEncoder() :
		_mediaSink(nullptr),
		_videoInputStreamID(0),
		_videoOutputStreamID(0),
		_audioInputStreamID(0),
		_audioOutputStreamID(0),
		_dxgiDeviceManagerResetToken(0),
		_nextFallbackVideoPresentationTimeUs(0),
		_nextFallbackAudioPresentationTimeUs(0),
		_didInitializeCOM(false),
		_didStartMediaFoundation(false),
		_hasVideoFormat(false),
		_hasAudioFormat(false),
		_usesD3DVideoInput(false),
		_usesAsyncVideoEncoder(false),
		_isReady(false)
	{}

	CaptureWindowsEncoder::~CaptureWindowsEncoder()
	{
		Stop();
	}

	bool CaptureWindowsEncoder::Start(const CaptureSession::Configuration &configuration, ID3D11Device *d3dDevice, CaptureEncodedSink *mediaSink)
	{
		if(!mediaSink)
		{
			RNWarning("Recording Windows media encoder requires an encoded media sink");
			return false;
		}
		if(!d3dDevice) return false;

		_mediaSink = mediaSink;

		HRESULT result = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if(SUCCEEDED(result))
		{
			_didInitializeCOM = true;
		}
		else if(result != RPC_E_CHANGED_MODE)
		{
			CaptureWindowsInternals::LogMediaFailure("COM initialization", result);
			return false;
		}

		result = MFStartup(MF_VERSION);
		if(CaptureWindowsInternals::LogMediaFailure("startup", result)) return false;
		_didStartMediaFoundation = true;

		if(!_videoProcessor.Start(d3dDevice, configuration, "encoding")) return false;
		if(!CreateDXGIDeviceManager(d3dDevice)) return false;
		if(!ConfigureVideoEncoder(configuration)) return false;
		if(!ConfigureAudioEncoder(configuration)) return false;

		if(!SendAudioFormat()) return false;
		SendVideoFormatFromCurrentType();

		_isReady = true;
		RNInfo("Started Windows media encoder");
		return true;
	}

	void CaptureWindowsEncoder::Stop()
	{
		LockGuard<Lockable> lock(_lock);
		if(_videoEncoder && _isReady)
		{
			if(_usesAsyncVideoEncoder)
			{
				bool didDrain = false;
				ProcessVideoEncoderEvents(didDrain);

				_videoEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, _videoInputStreamID);
				_videoEncoder->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, _videoInputStreamID);

				for(int i = 0; i < 5000 && !didDrain; i++)
				{
					ProcessVideoEncoderEvents(didDrain);
					if(!didDrain) ::Sleep(1);
				}

				if(!didDrain)
				{
					RNWarning("Recording Windows media encoder timed out waiting for async video drain");
				}
			}
			else
			{
				_videoEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
				_videoEncoder->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
				DrainVideoOutput();
			}
		}
		if(_audioEncoder && _isReady)
		{
			_audioEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
			_audioEncoder->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
			DrainAudioOutput();
		}
		if(_videoEncoder && _usesD3DVideoInput)
		{
			_videoEncoder->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, 0);
		}

		_isReady = false;
		_mediaSink = nullptr;
		_hasVideoFormat = false;
		_hasAudioFormat = false;
		_usesD3DVideoInput = false;
		_usesAsyncVideoEncoder = false;
		_pendingVideoTiming.clear();
		_pendingAudioTiming.clear();
		_videoEncoder.Reset();
		_audioEncoder.Reset();
		_videoEventGenerator.Reset();
		_dxgiDeviceManager.Reset();
		_dxgiDeviceManagerResetToken = 0;
		_videoProcessor.Stop();

		if(_didStartMediaFoundation)
		{
			MFShutdown();
			_didStartMediaFoundation = false;
		}

		if(_didInitializeCOM)
		{
			::CoUninitialize();
			_didInitializeCOM = false;
		}
	}

	bool CaptureWindowsEncoder::WriteFrame(ID3D11Texture2D *sourceTexture, int64 presentationTimeUs, int64 durationUs)
	{
		CaptureScopedCOMInitialization comInitialization;
		if(!comInitialization.GetIsValid()) return false;

		LockGuard<Lockable> lock(_lock);
		if(!_isReady || !_videoEncoder || !_usesD3DVideoInput || !sourceTexture) return false;

		ComPtr<ID3D11Texture2D> encoderTexture;
		if(!_videoProcessor.CreateEncoderTexture(sourceTexture, encoderTexture)) return false;

		ComPtr<IMFSample> sample;
		if(!CaptureWindowsInternals::CreateDXGISurfaceSample(encoderTexture.Get(), presentationTimeUs, durationUs, sample)) return false;

		HRESULT result = _videoEncoder->ProcessInput(_videoInputStreamID, sample.Get(), 0);
		if(result == MF_E_NOTACCEPTING)
		{
			if(!DrainVideoOutput()) return false;
			result = _videoEncoder->ProcessInput(_videoInputStreamID, sample.Get(), 0);
		}
		if(CaptureWindowsInternals::LogMediaFailure("video encoder input", result)) return false;

		_pendingVideoTiming.push_back({presentationTimeUs, durationUs});
		return DrainVideoOutput();
	}

	bool CaptureWindowsEncoder::WriteAudioSamples(const uint8 *samples, size_t sampleCount, int64 presentationTimeUs)
	{
		if(sampleCount == 0) return true;
		if(!samples) return false;
		if(sampleCount > (static_cast<size_t>(std::numeric_limits<DWORD>::max()) / CaptureWindowsInternals::AudioFrameBytes))
		{
			RNWarning("Recording Windows streaming audio sample block is too large: " << sampleCount);
			return false;
		}

		CaptureScopedCOMInitialization comInitialization;
		if(!comInitialization.GetIsValid()) return false;

		LockGuard<Lockable> lock(_lock);
		if(!_isReady || !_audioEncoder) return false;

		const int64 durationUs = CaptureWindowsInternals::GetAudioDurationForSamples(sampleCount);
		ComPtr<IMFSample> sample;
		if(!CaptureWindowsInternals::CreateMemorySample(samples, sampleCount * CaptureWindowsInternals::AudioFrameBytes, presentationTimeUs, durationUs, sample)) return false;

		HRESULT result = _audioEncoder->ProcessInput(_audioInputStreamID, sample.Get(), 0);
		if(result == MF_E_NOTACCEPTING)
		{
			if(!DrainAudioOutput()) return false;
			result = _audioEncoder->ProcessInput(_audioInputStreamID, sample.Get(), 0);
		}
		if(CaptureWindowsInternals::LogMediaFailure("audio encoder input", result)) return false;

		_pendingAudioTiming.push_back({presentationTimeUs, durationUs});
		return DrainAudioOutput();
	}

	bool CaptureWindowsEncoder::ConfigureVideoEncoder(const CaptureSession::Configuration &configuration)
	{
		if(!CreateTransform(MFT_CATEGORY_VIDEO_ENCODER, MFVideoFormat_NV12, MFVideoFormat_H264, &_videoEncoder)) return false;

		ComPtr<IMFAttributes> attributes;
		HRESULT result = _videoEncoder->GetAttributes(&attributes);
		if(CaptureWindowsInternals::LogMediaFailure("video encoder attributes query", result)) return false;
		if(!attributes) return false;

		attributes->SetUINT32(MF_LOW_LATENCY, TRUE);
		if(!ConfigureVideoEncoderD3DManager(attributes.Get())) return false;
		if(!GetTransformStreamIDs(_videoEncoder.Get(), _videoInputStreamID, _videoOutputStreamID)) return false;

		const UINT32 frameRate = CaptureWindowsInternals::GetVideoFrameRate(configuration);
		const UINT32 defaultVideoBitRate = configuration.streamURL ? CaptureWindowsInternals::DefaultStreamVideoBitRate : CaptureWindowsInternals::DefaultFileVideoBitRate;
		const UINT32 videoBitRate = static_cast<UINT32>(configuration.videoBitRate > 0 ? configuration.videoBitRate : defaultVideoBitRate);

		ComPtr<IMFMediaType> outputType;
		result = MFCreateMediaType(&outputType);
		if(CaptureWindowsInternals::LogMediaFailure("video encoder output media type creation", result)) return false;

		outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
		outputType->SetUINT32(MF_MT_AVG_BITRATE, videoBitRate);
		outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
		MFSetAttributeSize(outputType.Get(), MF_MT_FRAME_SIZE, _videoProcessor.GetWidth(), _videoProcessor.GetHeight());
		MFSetAttributeRatio(outputType.Get(), MF_MT_FRAME_RATE, frameRate, 1);
		MFSetAttributeRatio(outputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
		if(!CaptureWindowsInternals::SetVideoColorMetadata(outputType.Get())) return false;

		result = _videoEncoder->SetOutputType(_videoOutputStreamID, outputType.Get(), 0);
		if(CaptureWindowsInternals::LogMediaFailure("video encoder output media type setup", result)) return false;

		ComPtr<IMFMediaType> inputType;
		result = MFCreateMediaType(&inputType);
		if(CaptureWindowsInternals::LogMediaFailure("video encoder input media type creation", result)) return false;

		inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
		inputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
		inputType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);
		MFSetAttributeSize(inputType.Get(), MF_MT_FRAME_SIZE, _videoProcessor.GetWidth(), _videoProcessor.GetHeight());
		MFSetAttributeRatio(inputType.Get(), MF_MT_FRAME_RATE, frameRate, 1);
		MFSetAttributeRatio(inputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
		if(!CaptureWindowsInternals::SetVideoColorMetadata(inputType.Get())) return false;

		result = _videoEncoder->SetInputType(_videoInputStreamID, inputType.Get(), 0);
		if(CaptureWindowsInternals::LogMediaFailure("video encoder input media type setup", result)) return false;

		_videoEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
		_videoEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
		return true;
	}

	bool CaptureWindowsEncoder::ConfigureAudioEncoder(const CaptureSession::Configuration &configuration)
	{
		if(!CreateTransform(MFT_CATEGORY_AUDIO_ENCODER, MFAudioFormat_PCM, MFAudioFormat_AAC, &_audioEncoder)) return false;
		if(!GetTransformStreamIDs(_audioEncoder.Get(), _audioInputStreamID, _audioOutputStreamID)) return false;

		const UINT32 audioBitRate = static_cast<UINT32>(configuration.audioBitRate > 0 ? configuration.audioBitRate : CaptureWindowsInternals::DefaultAudioBitRate);

		ComPtr<IMFMediaType> outputType;
		HRESULT result = MFCreateMediaType(&outputType);
		if(CaptureWindowsInternals::LogMediaFailure("audio encoder output media type creation", result)) return false;

		outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		outputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
		outputType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, CaptureWindowsInternals::AudioChannelCount);
		outputType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, CaptureWindowsInternals::AudioSampleRate);
		outputType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, CaptureWindowsInternals::AudioBitsPerSample);
		outputType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, std::max<UINT32>(1, audioBitRate / 8));
		outputType->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE, 0);

		result = _audioEncoder->SetOutputType(_audioOutputStreamID, outputType.Get(), 0);
		if(CaptureWindowsInternals::LogMediaFailure("audio encoder output media type setup", result)) return false;

		ComPtr<IMFMediaType> inputType;
		result = MFCreateMediaType(&inputType);
		if(CaptureWindowsInternals::LogMediaFailure("audio encoder input media type creation", result)) return false;

		inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		inputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
		inputType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, CaptureWindowsInternals::AudioChannelCount);
		inputType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, CaptureWindowsInternals::AudioSampleRate);
		inputType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, CaptureWindowsInternals::AudioBitsPerSample);
		inputType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, CaptureWindowsInternals::AudioFrameBytes);
		inputType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, CaptureWindowsInternals::AudioSampleRate * CaptureWindowsInternals::AudioFrameBytes);
		inputType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);

		result = _audioEncoder->SetInputType(_audioInputStreamID, inputType.Get(), 0);
		if(CaptureWindowsInternals::LogMediaFailure("audio encoder input media type setup", result)) return false;

		_audioEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
		_audioEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
		return true;
	}

	bool CaptureWindowsEncoder::CreateDXGIDeviceManager(ID3D11Device *d3dDevice)
	{
		_dxgiDeviceManager.Reset();
		_dxgiDeviceManagerResetToken = 0;
		if(!d3dDevice) return false;

		HRESULT result = MFCreateDXGIDeviceManager(&_dxgiDeviceManagerResetToken, &_dxgiDeviceManager);
		if(CaptureWindowsInternals::LogMediaFailure("DXGI device manager creation", result)) return false;

		result = _dxgiDeviceManager->ResetDevice(d3dDevice, _dxgiDeviceManagerResetToken);
		if(CaptureWindowsInternals::LogMediaFailure("DXGI device manager reset", result))
		{
			_dxgiDeviceManager.Reset();
			_dxgiDeviceManagerResetToken = 0;
			return false;
		}

		return true;
	}

	bool CaptureWindowsEncoder::ConfigureVideoEncoderD3DManager(IMFAttributes *attributes)
	{
		_usesD3DVideoInput = false;
		_usesAsyncVideoEncoder = false;
		_videoEventGenerator.Reset();
		if(!_videoEncoder || !_dxgiDeviceManager || !attributes) return false;

		UINT32 d3dAware = FALSE;
		HRESULT result = attributes->GetUINT32(MF_SA_D3D11_AWARE, &d3dAware);
		if(FAILED(result) || !d3dAware)
		{
			RNWarning("Recording Windows media encoder requires a D3D11-aware video encoder transform");
			return false;
		}

		UINT32 isAsync = FALSE;
		if(SUCCEEDED(attributes->GetUINT32(MF_TRANSFORM_ASYNC, &isAsync)) && isAsync)
		{
			result = attributes->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, TRUE);
			if(CaptureWindowsInternals::LogMediaFailure("async video encoder unlock", result)) return false;

			result = _videoEncoder.As(&_videoEventGenerator);
			if(CaptureWindowsInternals::LogMediaFailure("async video encoder event generator query", result)) return false;
			_usesAsyncVideoEncoder = true;
		}

		result = _videoEncoder->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(_dxgiDeviceManager.Get()));
		if(CaptureWindowsInternals::LogMediaFailure("video encoder D3D manager setup", result))
		{
			_dxgiDeviceManager.Reset();
			_dxgiDeviceManagerResetToken = 0;
			return false;
		}

		_usesD3DVideoInput = true;
		RNInfo("Recording Windows media encoder is using D3D video input samples");
		return true;
	}

	bool CaptureWindowsEncoder::CreateTransform(const GUID &category, const GUID &inputSubtype, const GUID &outputSubtype, IMFTransform **transform)
	{
		MFT_REGISTER_TYPE_INFO inputInfo = {};
		inputInfo.guidMajorType = IsEqualGUID(category, MFT_CATEGORY_AUDIO_ENCODER) ? MFMediaType_Audio : MFMediaType_Video;
		inputInfo.guidSubtype = inputSubtype;

		MFT_REGISTER_TYPE_INFO outputInfo = {};
		outputInfo.guidMajorType = inputInfo.guidMajorType;
		outputInfo.guidSubtype = outputSubtype;

		const bool requiresD3D11AwareTransform = IsEqualGUID(category, MFT_CATEGORY_VIDEO_ENCODER);
		const UINT32 enumFlags = requiresD3D11AwareTransform ? MFT_ENUM_FLAG_ALL : (MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_LOCALMFT | MFT_ENUM_FLAG_SORTANDFILTER);
		IMFActivate **activates = nullptr;
		UINT32 activateCount = 0;
		HRESULT result = MFTEnumEx(category, enumFlags, &inputInfo, &outputInfo, &activates, &activateCount);
		if((FAILED(result) || activateCount == 0) && activates)
		{
			for(UINT32 i = 0; i < activateCount; i++) activates[i]->Release();
			CoTaskMemFree(activates);
			activates = nullptr;
			activateCount = 0;
		}
		if(!requiresD3D11AwareTransform && (FAILED(result) || activateCount == 0))
		{
			result = MFTEnumEx(category, MFT_ENUM_FLAG_ALL, &inputInfo, &outputInfo, &activates, &activateCount);
		}
		if(CaptureWindowsInternals::LogMediaFailure("encoder transform enumeration", result)) return false;
		if(activateCount == 0 || !activates)
		{
			RNWarning("Recording Windows media encoder could not find a Media Foundation encoder transform");
			return false;
		}

		HRESULT activationResult = E_FAIL;
		for(UINT32 i = 0; i < activateCount; i++)
		{
			ComPtr<IMFTransform> candidate;
			activationResult = activates[i]->ActivateObject(IID_PPV_ARGS(&candidate));
			if(FAILED(activationResult)) continue;

			if(requiresD3D11AwareTransform)
			{
				ComPtr<IMFAttributes> attributes;
				UINT32 d3dAware = FALSE;
				if(FAILED(candidate->GetAttributes(&attributes)) || !attributes || FAILED(attributes->GetUINT32(MF_SA_D3D11_AWARE, &d3dAware)) || !d3dAware)
				{
					activationResult = E_FAIL;
					continue;
				}
			}

			*transform = candidate.Detach();
			activationResult = S_OK;
			break;
		}
		for(UINT32 i = 0; i < activateCount; i++)
		{
			activates[i]->Release();
		}
		CoTaskMemFree(activates);

		if(requiresD3D11AwareTransform && !*transform)
		{
			RNWarning("Recording Windows media encoder could not find a D3D11-aware video encoder transform");
			return false;
		}
		if(CaptureWindowsInternals::LogMediaFailure("encoder transform activation", activationResult)) return false;
		return true;
	}

	bool CaptureWindowsEncoder::GetTransformStreamIDs(IMFTransform *transform, DWORD &inputStreamID, DWORD &outputStreamID)
	{
		DWORD inputStreamCount = 0;
		DWORD outputStreamCount = 0;
		HRESULT result = transform->GetStreamCount(&inputStreamCount, &outputStreamCount);
		if(CaptureWindowsInternals::LogMediaFailure("encoder stream count query", result)) return false;
		if(inputStreamCount == 0 || outputStreamCount == 0) return false;

		result = transform->GetStreamIDs(1, &inputStreamID, 1, &outputStreamID);
		if(result == E_NOTIMPL)
		{
			inputStreamID = 0;
			outputStreamID = 0;
			return true;
		}

		if(CaptureWindowsInternals::LogMediaFailure("encoder stream ID query", result)) return false;
		return true;
	}

	bool CaptureWindowsEncoder::ProcessVideoEncoderEvents(bool &didDrain)
	{
		didDrain = false;
		if(!_videoEventGenerator) return true;

		while(true)
		{
			ComPtr<IMFMediaEvent> event;
			HRESULT result = _videoEventGenerator->GetEvent(MF_EVENT_FLAG_NO_WAIT, &event);
			if(result == MF_E_NO_EVENTS_AVAILABLE) return true;
			if(CaptureWindowsInternals::LogMediaFailure("video encoder event query", result)) return false;
			if(!event) return false;

			MediaEventType eventType = MEUnknown;
			result = event->GetType(&eventType);
			if(CaptureWindowsInternals::LogMediaFailure("video encoder event type query", result)) return false;

			HRESULT eventStatus = S_OK;
			result = event->GetStatus(&eventStatus);
			if(CaptureWindowsInternals::LogMediaFailure("video encoder event status query", result)) return false;
			if(CaptureWindowsInternals::LogMediaFailure("video encoder event", eventStatus)) return false;

			if(eventType == METransformHaveOutput)
			{
				if(!DrainTransformOutput(_videoEncoder.Get(), _videoOutputStreamID, true)) return false;
			}
			else if(eventType == METransformDrainComplete)
			{
				didDrain = true;
				return true;
			}
		}
	}

	bool CaptureWindowsEncoder::SendAudioFormat()
	{
		std::vector<uint8> audioSpecificConfig;
		BuildAACAudioSpecificConfig(audioSpecificConfig);

		CaptureEncodedAudioFormat format;
		format.sampleRate = CaptureWindowsInternals::AudioSampleRate;
		format.channelCount = CaptureWindowsInternals::AudioChannelCount;
		format.audioSpecificConfig = audioSpecificConfig.data();
		format.audioSpecificConfigSize = audioSpecificConfig.size();

		if(!_mediaSink->DidReceiveAudioFormat(format)) return false;
		_hasAudioFormat = true;
		return true;
	}

	bool CaptureWindowsEncoder::SendVideoFormatFromCurrentType()
	{
		if(_hasVideoFormat || !_videoEncoder || !_mediaSink) return _hasVideoFormat;

		ComPtr<IMFMediaType> outputType;
		HRESULT result = _videoEncoder->GetOutputCurrentType(_videoOutputStreamID, &outputType);
		if(FAILED(result) || !outputType) return false;

		UINT32 blobSize = 0;
		UINT8 *blob = nullptr;
		result = outputType->GetAllocatedBlob(MF_MT_MPEG_SEQUENCE_HEADER, &blob, &blobSize);
		if(FAILED(result) || !blob || blobSize == 0)
		{
			if(blob) CoTaskMemFree(blob);
			return false;
		}

		CaptureEncodedVideoFormat format;
		format.width = static_cast<int32>(_videoProcessor.GetWidth());
		format.height = static_cast<int32>(_videoProcessor.GetHeight());
		format.codecConfiguration = blob;
		format.codecConfigurationSize = blobSize;

		bool accepted = _mediaSink->DidReceiveVideoFormat(format);
		CoTaskMemFree(blob);
		if(!accepted) return false;

		_hasVideoFormat = true;
		return true;
	}

	bool CaptureWindowsEncoder::SendVideoFormatFromSample(const std::vector<uint8> &data)
	{
		if(_hasVideoFormat || data.empty() || !_mediaSink) return _hasVideoFormat;

		CaptureEncodedVideoFormat format;
		format.width = static_cast<int32>(_videoProcessor.GetWidth());
		format.height = static_cast<int32>(_videoProcessor.GetHeight());
		format.codecConfiguration = data.data();
		format.codecConfigurationSize = data.size();

		if(!_mediaSink->DidReceiveVideoFormat(format)) return false;

		_hasVideoFormat = true;
		return true;
	}

	bool CaptureWindowsEncoder::DrainVideoOutput()
	{
		if(_usesAsyncVideoEncoder)
		{
			bool didDrain = false;
			return ProcessVideoEncoderEvents(didDrain);
		}

		return DrainTransformOutput(_videoEncoder.Get(), _videoOutputStreamID, true);
	}

	bool CaptureWindowsEncoder::DrainAudioOutput()
	{
		return DrainTransformOutput(_audioEncoder.Get(), _audioOutputStreamID, false);
	}

	bool CaptureWindowsEncoder::DrainTransformOutput(IMFTransform *transform, DWORD outputStreamID, bool isVideo)
	{
		if(!transform) return false;

		MFT_OUTPUT_STREAM_INFO streamInfo = {};
		HRESULT result = transform->GetOutputStreamInfo(outputStreamID, &streamInfo);
		if(CaptureWindowsInternals::LogMediaFailure(isVideo ? "video encoder output stream info query" : "audio encoder output stream info query", result)) return false;

		while(true)
		{
			ComPtr<IMFSample> outputSample;
			ComPtr<IMFMediaBuffer> outputBuffer;
			MFT_OUTPUT_DATA_BUFFER outputData = {};
			outputData.dwStreamID = outputStreamID;

			if((streamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) == 0)
			{
				DWORD bufferSize = streamInfo.cbSize > 0 ? streamInfo.cbSize : GetDefaultMFTOutputBufferSize(isVideo, _videoProcessor.GetWidth(), _videoProcessor.GetHeight());
				result = MFCreateSample(&outputSample);
				if(CaptureWindowsInternals::LogMediaFailure("encoder output sample creation", result)) return false;

				result = MFCreateMemoryBuffer(bufferSize, &outputBuffer);
				if(CaptureWindowsInternals::LogMediaFailure("encoder output buffer creation", result)) return false;

				result = outputSample->AddBuffer(outputBuffer.Get());
				if(CaptureWindowsInternals::LogMediaFailure("encoder output buffer attachment", result)) return false;

				outputData.pSample = outputSample.Get();
			}

			DWORD status = 0;
			result = transform->ProcessOutput(0, 1, &outputData, &status);

			ComPtr<IMFSample> providedSample;
			if((streamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) != 0 && outputData.pSample)
			{
				providedSample.Attach(outputData.pSample);
				outputData.pSample = nullptr;
			}

			if(outputData.pEvents) outputData.pEvents->Release();

			if(result == MF_E_TRANSFORM_NEED_MORE_INPUT)
			{
				return true;
			}
			if(isVideo && _usesAsyncVideoEncoder && result == E_UNEXPECTED)
			{
				return true;
			}
			if(result == MF_E_TRANSFORM_STREAM_CHANGE)
			{
				if(isVideo) SendVideoFormatFromCurrentType();
				continue;
			}
			if(CaptureWindowsInternals::LogMediaFailure(isVideo ? "video encoder output" : "audio encoder output", result)) return false;

			if((outputData.dwStatus & MFT_OUTPUT_DATA_BUFFER_FORMAT_CHANGE) != 0)
			{
				if(isVideo) SendVideoFormatFromCurrentType();
			}

			IMFSample *sample = providedSample ? providedSample.Get() : outputSample.Get();
			if(sample)
			{
				if(isVideo)
				{
					if(!HandleEncodedVideoSample(sample)) return false;
				}
				else
				{
					if(!HandleEncodedAudioSample(sample)) return false;
				}
			}
		}
	}

	bool CaptureWindowsEncoder::HandleEncodedVideoSample(IMFSample *sample)
	{
		std::vector<uint8> data;
		if(!CaptureWindowsInternals::CopySampleData(sample, data)) return false;
		if(data.empty()) return true;

		if(!_hasVideoFormat)
		{
			SendVideoFormatFromCurrentType();
			if(!_hasVideoFormat)
			{
				SendVideoFormatFromSample(data);
			}
			if(!_hasVideoFormat) return true;
		}

		PendingTiming timing = TakePendingTiming(_pendingVideoTiming, sample);
		if(timing.durationUs <= 0)
		{
			timing.durationUs = static_cast<int64>(std::max(1.0, std::round(1000000.0 / std::max<UINT32>(1, _videoProcessor.GetFrameRate()))));
		}
		if(timing.presentationTimeUs < _nextFallbackVideoPresentationTimeUs && _pendingVideoTiming.empty())
		{
			timing.presentationTimeUs = _nextFallbackVideoPresentationTimeUs;
		}
		_nextFallbackVideoPresentationTimeUs = timing.presentationTimeUs + timing.durationUs;

		CaptureEncodedSample encodedSample;
		encodedSample.data = data.data();
		encodedSample.offset = 0;
		encodedSample.size = data.size();
		encodedSample.presentationTimeUs = timing.presentationTimeUs;
		encodedSample.durationUs = timing.durationUs;
		encodedSample.isKeyFrame = IsH264KeyFrame(sample, data);

		if(_mediaSink->GetIsReady())
		{
			_mediaSink->WriteVideoSample(encodedSample);
		}

		return true;
	}

	bool CaptureWindowsEncoder::HandleEncodedAudioSample(IMFSample *sample)
	{
		if(!_hasAudioFormat) return true;

		std::vector<uint8> data;
		if(!CaptureWindowsInternals::CopySampleData(sample, data)) return false;
		if(data.empty()) return true;

		PendingTiming timing = TakePendingTiming(_pendingAudioTiming, sample);
		if(timing.durationUs <= 0)
		{
			timing.durationUs = CaptureWindowsInternals::GetAudioDurationForSamples(1024);
		}
		if(timing.presentationTimeUs < _nextFallbackAudioPresentationTimeUs && _pendingAudioTiming.empty())
		{
			timing.presentationTimeUs = _nextFallbackAudioPresentationTimeUs;
		}
		_nextFallbackAudioPresentationTimeUs = timing.presentationTimeUs + timing.durationUs;

		CaptureEncodedSample encodedSample;
		encodedSample.data = data.data();
		encodedSample.offset = 0;
		encodedSample.size = data.size();
		encodedSample.presentationTimeUs = timing.presentationTimeUs;
		encodedSample.durationUs = timing.durationUs;
		encodedSample.isKeyFrame = true;

		if(_mediaSink->GetIsReady())
		{
			_mediaSink->WriteAudioSample(encodedSample);
		}

		return true;
	}

	CaptureWindowsEncoder::PendingTiming CaptureWindowsEncoder::TakePendingTiming(std::deque<PendingTiming> &pendingTiming, IMFSample *sample)
	{
		PendingTiming timing;
		if(!pendingTiming.empty())
		{
			timing = pendingTiming.front();
			pendingTiming.pop_front();
		}

		int64 presentationTimeUs = 0;
		if(GetSampleTimestampUs(sample, presentationTimeUs))
		{
			timing.presentationTimeUs = presentationTimeUs;
		}

		int64 durationUs = 0;
		if(GetSampleDurationUs(sample, durationUs))
		{
			timing.durationUs = durationUs;
		}

		return timing;
	}

	bool CaptureWindowsEncoder::GetSampleTimestampUs(IMFSample *sample, int64 &presentationTimeUs) const
	{
		LONGLONG sampleTime = 0;
		HRESULT result = sample->GetSampleTime(&sampleTime);
		if(FAILED(result)) return false;

		presentationTimeUs = static_cast<int64>(sampleTime / 10);
		return true;
	}

	bool CaptureWindowsEncoder::GetSampleDurationUs(IMFSample *sample, int64 &durationUs) const
	{
		LONGLONG sampleDuration = 0;
		HRESULT result = sample->GetSampleDuration(&sampleDuration);
		if(FAILED(result)) return false;

		durationUs = static_cast<int64>(sampleDuration / 10);
		return true;
	}

	bool CaptureWindowsEncoder::IsH264KeyFrame(IMFSample *sample, const std::vector<uint8> &data) const
	{
		UINT32 cleanPoint = FALSE;
		if(SUCCEEDED(sample->GetUINT32(MFSampleExtension_CleanPoint, &cleanPoint)) && cleanPoint) return true;
		return ContainsH264NALType(data.data(), data.size(), 5);
	}

	bool CaptureWindowsEncoder::ContainsH264NALType(const uint8 *data, size_t size, uint8 nalType) const
	{
		if(!data || size == 0) return false;

		auto startsWithStartCode = [](const uint8 *bytes, size_t remaining) {
			return remaining >= 3 && bytes[0] == 0 && bytes[1] == 0 && (bytes[2] == 1 || (remaining >= 4 && bytes[2] == 0 && bytes[3] == 1));
		};
		auto startCodeLength = [](const uint8 *bytes, size_t remaining) -> size_t {
			return (remaining >= 4 && bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 1) ? 4 : 3;
		};
		auto findStartCode = [&](size_t offset) -> size_t {
			while(offset + 3 <= size)
			{
				if(startsWithStartCode(data + offset, size - offset)) return offset;
				offset++;
			}

			return std::string::npos;
		};

		if(startsWithStartCode(data, size))
		{
			size_t offset = 0;
			while(true)
			{
				const size_t start = findStartCode(offset);
				if(start == std::string::npos) break;

				const size_t unitStart = start + startCodeLength(data + start, size - start);
				const size_t nextStart = findStartCode(unitStart);
				const size_t unitEnd = nextStart == std::string::npos ? size : nextStart;
				if(unitStart < unitEnd && (data[unitStart] & 0x1f) == nalType) return true;
				if(nextStart == std::string::npos) break;
				offset = nextStart;
			}

			return false;
		}

		size_t offset = 0;
		while(offset + 4 <= size)
		{
			const size_t length = (static_cast<size_t>(data[offset]) << 24) | (static_cast<size_t>(data[offset + 1]) << 16) | (static_cast<size_t>(data[offset + 2]) << 8) | data[offset + 3];
			if(length == 0 || offset + 4 + length > size) break;
			if((data[offset + 4] & 0x1f) == nalType) return true;
			offset += 4 + length;
		}

		return (data[0] & 0x1f) == nalType;
	}
} // namespace RN

#endif
