//
//  RNCaptureWindowsInternals.cpp
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCaptureWindowsInternals.h"

#include "RNCaptureWindowsEncoder.h"

#if RN_PLATFORM_WINDOWS

namespace RN
{
	using Microsoft::WRL::ComPtr;

	namespace CaptureWindowsInternals
	{
		std::wstring CreateWindowsWideString(const String *string)
		{
			if(String::IsNullOrEmpty(string)) return std::wstring();

			const int wideLength = ::MultiByteToWideChar(CP_UTF8, 0, string->GetUTF8String(), -1, nullptr, 0);
			if(wideLength <= 0) return std::wstring();

			std::wstring wideString(wideLength, L'\0');
			if(::MultiByteToWideChar(CP_UTF8, 0, string->GetUTF8String(), -1, wideString.data(), wideLength) == 0)
			{
				return std::wstring();
			}

			wideString.resize(wideLength - 1);
			return wideString;
		}

		bool LogMediaFailure(const char *operation, HRESULT result)
		{
			if(SUCCEEDED(result)) return false;

			RNWarning("Recording Media Foundation " << operation << " failed: 0x" << std::hex << result << std::dec);
			return true;
		}

		bool LogD3DMediaFailure(const char *operation, HRESULT result)
		{
			if(SUCCEEDED(result)) return false;

			RNWarning("Recording D3D media " << operation << " failed: 0x" << std::hex << result << std::dec);
			return true;
		}

		UINT32 GetVideoFrameRate(const CaptureSession::Configuration &configuration)
		{
			double framePeriod = configuration.videoFramePeriod > 0.0 ? configuration.videoFramePeriod : (1.0 / 30.0);
			return static_cast<UINT32>(std::max(1.0, std::round(1.0 / framePeriod)));
		}

		bool SetVideoColorMetadata(IMFMediaType *mediaType)
		{
			HRESULT result = mediaType->SetUINT32(MF_MT_VIDEO_PRIMARIES, MFVideoPrimaries_BT709);
			if(LogMediaFailure("video primaries setup", result)) return false;

			result = mediaType->SetUINT32(MF_MT_TRANSFER_FUNCTION, MFVideoTransFunc_709);
			if(LogMediaFailure("video transfer function setup", result)) return false;

			result = mediaType->SetUINT32(MF_MT_YUV_MATRIX, MFVideoTransferMatrix_BT709);
			if(LogMediaFailure("video YUV matrix setup", result)) return false;

			result = mediaType->SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_16_235);
			if(LogMediaFailure("video nominal range setup", result)) return false;

			return true;
		}

		bool CreateMemorySample(const uint8 *data, size_t byteCount, int64 presentationTimeUs, int64 durationUs, ComPtr<IMFSample> &sample)
		{
			if(byteCount > 0 && !data)
			{
				RNWarning("Recording Media Foundation sample data is null for non-empty sample");
				return false;
			}

			if(byteCount > static_cast<size_t>(std::numeric_limits<DWORD>::max()))
			{
				RNWarning("Recording Media Foundation sample block is too large: " << byteCount);
				return false;
			}

			ComPtr<IMFMediaBuffer> mediaBuffer;
			HRESULT result = MFCreateMemoryBuffer(static_cast<DWORD>(byteCount), &mediaBuffer);
			if(LogMediaFailure("memory buffer creation", result)) return false;

			if(byteCount > 0)
			{
				BYTE *destination = nullptr;
				DWORD maxLength = 0;
				DWORD currentLength = 0;
				result = mediaBuffer->Lock(&destination, &maxLength, &currentLength);
				if(LogMediaFailure("memory buffer lock", result)) return false;
				if(maxLength < byteCount)
				{
					mediaBuffer->Unlock();
					RNWarning("Recording Media Foundation sample buffer is smaller than requested. Requested: " << byteCount << ", available: " << maxLength);
					return false;
				}

				if(data) std::memcpy(destination, data, byteCount);
				result = mediaBuffer->Unlock();
				if(LogMediaFailure("memory buffer unlock", result)) return false;
			}

			result = mediaBuffer->SetCurrentLength(static_cast<DWORD>(byteCount));
			if(LogMediaFailure("memory buffer current length setup", result)) return false;

			result = MFCreateSample(&sample);
			if(LogMediaFailure("sample creation", result)) return false;

			result = sample->AddBuffer(mediaBuffer.Get());
			if(LogMediaFailure("sample buffer attachment", result)) return false;

			result = sample->SetSampleTime(presentationTimeUs * 10);
			if(LogMediaFailure("sample timestamp", result)) return false;

			result = sample->SetSampleDuration(durationUs * 10);
			if(LogMediaFailure("sample duration", result)) return false;

			return true;
		}

		bool CreateDXGISurfaceSample(ID3D11Texture2D *texture, int64 presentationTimeUs, int64 durationUs, ComPtr<IMFSample> &sample)
		{
			if(!texture)
			{
				RNWarning("Recording Media Foundation DXGI sample texture is null");
				return false;
			}

			D3D11_TEXTURE2D_DESC description = {};
			texture->GetDesc(&description);
			if(description.Format != EncoderTextureFormat)
			{
				RNWarning("Recording Media Foundation DXGI sample received unexpected texture format: " << static_cast<int>(description.Format));
				return false;
			}

			const uint64 byteCount = (static_cast<uint64>(description.Width) * description.Height * 3) / 2;
			if(byteCount > static_cast<uint64>(std::numeric_limits<DWORD>::max()))
			{
				RNWarning("Recording Media Foundation DXGI sample block is too large: " << byteCount);
				return false;
			}

			ComPtr<IMFMediaBuffer> mediaBuffer;
			HRESULT result = MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), texture, 0, FALSE, &mediaBuffer);
			if(LogMediaFailure("DXGI surface buffer creation", result)) return false;

			result = mediaBuffer->SetCurrentLength(static_cast<DWORD>(byteCount));
			if(LogMediaFailure("DXGI surface buffer current length setup", result)) return false;

			result = MFCreateSample(&sample);
			if(LogMediaFailure("DXGI surface sample creation", result)) return false;

			result = sample->AddBuffer(mediaBuffer.Get());
			if(LogMediaFailure("DXGI surface sample buffer attachment", result)) return false;

			result = sample->SetSampleTime(presentationTimeUs * 10);
			if(LogMediaFailure("DXGI surface sample timestamp", result)) return false;

			result = sample->SetSampleDuration(durationUs * 10);
			if(LogMediaFailure("DXGI surface sample duration", result)) return false;

			return true;
		}

		bool CopySampleData(IMFSample *sample, std::vector<uint8> &data)
		{
			data.clear();
			if(!sample) return false;

			ComPtr<IMFMediaBuffer> mediaBuffer;
			HRESULT result = sample->ConvertToContiguousBuffer(&mediaBuffer);
			if(LogMediaFailure("sample contiguous buffer conversion", result)) return false;

			BYTE *source = nullptr;
			DWORD maxLength = 0;
			DWORD currentLength = 0;
			result = mediaBuffer->Lock(&source, &maxLength, &currentLength);
			if(LogMediaFailure("sample buffer lock", result)) return false;

			if(currentLength > 0)
			{
				data.assign(source, source + currentLength);
			}

			result = mediaBuffer->Unlock();
			if(LogMediaFailure("sample buffer unlock", result)) return false;

			return true;
		}
	} // namespace CaptureWindowsInternals

	CaptureScopedCOMInitialization::CaptureScopedCOMInitialization() :
		_didInitialize(false),
		_isValid(false)
	{
		HRESULT result = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if(SUCCEEDED(result))
		{
			_didInitialize = true;
			_isValid = true;
		}
		else if(result == RPC_E_CHANGED_MODE)
		{
			_isValid = true;
		}
		else
		{
			CaptureWindowsInternals::LogMediaFailure("COM initialization", result);
		}
	}

	CaptureScopedCOMInitialization::~CaptureScopedCOMInitialization()
	{
		if(_didInitialize) ::CoUninitialize();
	}

	bool CaptureScopedCOMInitialization::GetIsValid() const
	{
		return _isValid;
	}

	CaptureScopedKeyedMutex::CaptureScopedKeyedMutex(ID3D11Texture2D *texture) :
		_didAcquire(false),
		_isValid(true)
	{
		if(!texture) return;

		HRESULT result = texture->QueryInterface(IID_PPV_ARGS(&_keyedMutex));
		if(result == E_NOINTERFACE) return;
		if(CaptureWindowsInternals::LogD3DMediaFailure("shared texture keyed mutex query", result))
		{
			_isValid = false;
			return;
		}

		result = _keyedMutex->AcquireSync(0, 1000);
		if(CaptureWindowsInternals::LogD3DMediaFailure("shared texture keyed mutex acquire", result))
		{
			_keyedMutex.Reset();
			_isValid = false;
			return;
		}

		_didAcquire = true;
	}

	CaptureScopedKeyedMutex::~CaptureScopedKeyedMutex()
	{
		if(_didAcquire)
		{
			HRESULT result = _keyedMutex->ReleaseSync(0);
			CaptureWindowsInternals::LogD3DMediaFailure("shared texture keyed mutex release", result);
		}
	}

	bool CaptureScopedKeyedMutex::GetIsValid() const
	{
		return _isValid;
	}

	CaptureWindowsVideoProcessor::CaptureWindowsVideoProcessor() :
		_width(0),
		_height(0),
		_frameRate(0),
		_isReady(false)
	{}

	CaptureWindowsVideoProcessor::~CaptureWindowsVideoProcessor()
	{
		Stop();
	}

	bool CaptureWindowsVideoProcessor::Start(ID3D11Device *d3dDevice, const CaptureSession::Configuration &configuration, const char *logContext)
	{
		LockGuard<Lockable> lock(_lock);
		Stop();

		if(!d3dDevice) return false;
		if(configuration.resolutionX <= 0 || configuration.resolutionY <= 0)
		{
			RNWarning("Recording Windows " << logContext << " requires a positive video resolution");
			return false;
		}

		if((configuration.resolutionX % 2) != 0 || (configuration.resolutionY % 2) != 0)
		{
			RNWarning("Recording Windows " << logContext << " requires even video dimensions for NV12 encoding");
			return false;
		}

		_width = static_cast<UINT32>(configuration.resolutionX);
		_height = static_cast<UINT32>(configuration.resolutionY);
		_frameRate = CaptureWindowsInternals::GetVideoFrameRate(configuration);

		_d3dDevice = d3dDevice;
		_d3dDevice->GetImmediateContext(&_d3dContext);

		HRESULT result = _d3dDevice.As(&_videoDevice);
		if(CaptureWindowsInternals::LogD3DMediaFailure("video device query", result)) return false;

		result = _d3dContext.As(&_videoContext);
		if(CaptureWindowsInternals::LogD3DMediaFailure("video context query", result)) return false;

		result = _d3dContext.As(&_videoContext1);
		if(CaptureWindowsInternals::LogD3DMediaFailure("video context 1 query", result)) return false;

		if(!CheckD3DFormatSupport(CaptureWindowsInternals::VideoProcessorInputTextureFormat, D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_VIDEO_PROCESSOR_INPUT, "BGRA video processor input texture")) return false;
		if(!CheckD3DFormatSupport(CaptureWindowsInternals::EncoderTextureFormat, D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_VIDEO_PROCESSOR_OUTPUT | D3D11_FORMAT_SUPPORT_VIDEO_ENCODER, "NV12 encoder texture")) return false;

		D3D11_VIDEO_PROCESSOR_CONTENT_DESC contentDescription = {};
		contentDescription.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
		contentDescription.InputFrameRate.Numerator = _frameRate;
		contentDescription.InputFrameRate.Denominator = 1;
		contentDescription.InputWidth = _width;
		contentDescription.InputHeight = _height;
		contentDescription.OutputFrameRate.Numerator = _frameRate;
		contentDescription.OutputFrameRate.Denominator = 1;
		contentDescription.OutputWidth = _width;
		contentDescription.OutputHeight = _height;
		contentDescription.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

		result = _videoDevice->CreateVideoProcessorEnumerator(&contentDescription, &_videoProcessorEnumerator);
		if(CaptureWindowsInternals::LogD3DMediaFailure("video processor enumerator creation", result)) return false;

		if(!CheckVideoProcessorFormat(CaptureWindowsInternals::VideoProcessorInputTextureFormat, D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT, "BGRA video processor input")) return false;
		if(!CheckVideoProcessorFormat(CaptureWindowsInternals::EncoderTextureFormat, D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_OUTPUT, "NV12 video processor output")) return false;
		if(!CheckVideoProcessorConversion()) return false;

		result = _videoDevice->CreateVideoProcessor(_videoProcessorEnumerator.Get(), 0, &_videoProcessor);
		if(CaptureWindowsInternals::LogD3DMediaFailure("video processor creation", result)) return false;
		ConfigureVideoProcessorState();

		_isReady = true;
		return true;
	}

	void CaptureWindowsVideoProcessor::Stop()
	{
		_isReady = false;
		_videoProcessor.Reset();
		_videoProcessorEnumerator.Reset();
		_videoContext1.Reset();
		_videoContext.Reset();
		_videoDevice.Reset();
		_d3dContext.Reset();
		_d3dDevice.Reset();
		_width = 0;
		_height = 0;
		_frameRate = 0;
	}

	bool CaptureWindowsVideoProcessor::CreateEncoderTexture(ID3D11Texture2D *sourceTexture, ComPtr<ID3D11Texture2D> &encoderTexture)
	{
		LockGuard<Lockable> lock(_lock);
		if(!_isReady || !_d3dDevice || !_d3dContext || !sourceTexture) return false;

		D3D11_TEXTURE2D_DESC sourceDescription = {};
		sourceTexture->GetDesc(&sourceDescription);
		if((sourceDescription.Format != CaptureWindowsInternals::CaptureSourceTextureFormat && sourceDescription.Format != CaptureWindowsInternals::VideoProcessorInputTextureFormat) || sourceDescription.Width != _width || sourceDescription.Height != _height)
		{
			RNWarning("Recording Windows media received an unexpected source texture. Format: " << static_cast<int>(sourceDescription.Format) << ", size: " << sourceDescription.Width << "x" << sourceDescription.Height);
			return false;
		}

		ComPtr<ID3D11Texture2D> videoInputTexture;
		if(!CreateVideoInputTexture(sourceTexture, videoInputTexture)) return false;

		D3D11_TEXTURE2D_DESC encoderDescription = {};
		encoderDescription.Width = _width;
		encoderDescription.Height = _height;
		encoderDescription.MipLevels = 1;
		encoderDescription.ArraySize = 1;
		encoderDescription.Format = CaptureWindowsInternals::EncoderTextureFormat;
		encoderDescription.SampleDesc.Count = 1;
		encoderDescription.SampleDesc.Quality = 0;
		encoderDescription.Usage = D3D11_USAGE_DEFAULT;
		encoderDescription.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_VIDEO_ENCODER;
		encoderDescription.CPUAccessFlags = 0;
		encoderDescription.MiscFlags = 0;

		HRESULT result = _d3dDevice->CreateTexture2D(&encoderDescription, nullptr, &encoderTexture);
		if(CaptureWindowsInternals::LogD3DMediaFailure("NV12 encoder texture creation", result)) return false;

		ComPtr<ID3D11VideoProcessorInputView> inputView;
		D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC inputDescription = {};
		inputDescription.FourCC = 0;
		inputDescription.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
		inputDescription.Texture2D.MipSlice = 0;
		inputDescription.Texture2D.ArraySlice = 0;
		result = _videoDevice->CreateVideoProcessorInputView(videoInputTexture.Get(), _videoProcessorEnumerator.Get(), &inputDescription, &inputView);
		if(CaptureWindowsInternals::LogD3DMediaFailure("video processor input view creation", result)) return false;

		ComPtr<ID3D11VideoProcessorOutputView> outputView;
		D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC outputDescription = {};
		outputDescription.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
		outputDescription.Texture2D.MipSlice = 0;
		result = _videoDevice->CreateVideoProcessorOutputView(encoderTexture.Get(), _videoProcessorEnumerator.Get(), &outputDescription, &outputView);
		if(CaptureWindowsInternals::LogD3DMediaFailure("video processor output view creation", result)) return false;

		D3D11_VIDEO_PROCESSOR_STREAM stream = {};
		stream.Enable = TRUE;
		stream.OutputIndex = 0;
		stream.InputFrameOrField = 0;
		stream.pInputSurface = inputView.Get();
		result = _videoContext->VideoProcessorBlt(_videoProcessor.Get(), outputView.Get(), 0, 1, &stream);
		if(CaptureWindowsInternals::LogD3DMediaFailure("BGRA to NV12 conversion", result)) return false;

		_d3dContext->Flush();
		return true;
	}

	UINT32 CaptureWindowsVideoProcessor::GetWidth() const
	{
		return _width;
	}

	UINT32 CaptureWindowsVideoProcessor::GetHeight() const
	{
		return _height;
	}

	UINT32 CaptureWindowsVideoProcessor::GetFrameRate() const
	{
		return _frameRate;
	}

	bool CaptureWindowsVideoProcessor::CheckD3DFormatSupport(DXGI_FORMAT format, UINT requiredFlags, const char *name)
	{
		UINT supportedFlags = 0;
		HRESULT result = _d3dDevice->CheckFormatSupport(format, &supportedFlags);
		if(CaptureWindowsInternals::LogD3DMediaFailure(name, result)) return false;

		if((supportedFlags & requiredFlags) != requiredFlags)
		{
			RNWarning("Recording Windows media " << name << " is missing required D3D format support flags. Required: 0x" << std::hex << requiredFlags << ", supported: 0x" << supportedFlags << std::dec);
			return false;
		}

		return true;
	}

	bool CaptureWindowsVideoProcessor::CheckVideoProcessorFormat(DXGI_FORMAT format, UINT requiredFlags, const char *name)
	{
		UINT supportedFlags = 0;
		HRESULT result = _videoProcessorEnumerator->CheckVideoProcessorFormat(format, &supportedFlags);
		if(CaptureWindowsInternals::LogD3DMediaFailure(name, result)) return false;

		if((supportedFlags & requiredFlags) != requiredFlags)
		{
			RNWarning("Recording Windows media " << name << " is missing required video processor support flags. Required: 0x" << std::hex << requiredFlags << ", supported: 0x" << supportedFlags << std::dec);
			return false;
		}

		return true;
	}

	bool CaptureWindowsVideoProcessor::CheckVideoProcessorConversion()
	{
		ComPtr<ID3D11VideoProcessorEnumerator1> videoProcessorEnumerator;
		HRESULT result = _videoProcessorEnumerator.As(&videoProcessorEnumerator);
		if(CaptureWindowsInternals::LogD3DMediaFailure("video processor conversion support query", result)) return false;

		BOOL isSupported = FALSE;
		result = videoProcessorEnumerator->CheckVideoProcessorFormatConversion(CaptureWindowsInternals::VideoProcessorInputTextureFormat, CaptureWindowsInternals::SourceColorSpace, CaptureWindowsInternals::EncoderTextureFormat, CaptureWindowsInternals::EncoderColorSpace, &isSupported);
		if(CaptureWindowsInternals::LogD3DMediaFailure("BGRA to NV12 conversion support query", result)) return false;
		if(!isSupported)
		{
			RNWarning("Recording Windows media D3D video processor does not support BGRA to NV12 conversion");
			return false;
		}

		return true;
	}

	void CaptureWindowsVideoProcessor::ConfigureVideoProcessorState()
	{
		RECT frameRect = {};
		frameRect.right = static_cast<LONG>(_width);
		frameRect.bottom = static_cast<LONG>(_height);

		_videoContext->VideoProcessorSetStreamFrameFormat(_videoProcessor.Get(), 0, D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE);
		_videoContext->VideoProcessorSetStreamSourceRect(_videoProcessor.Get(), 0, TRUE, &frameRect);
		_videoContext->VideoProcessorSetStreamDestRect(_videoProcessor.Get(), 0, TRUE, &frameRect);
		_videoContext->VideoProcessorSetStreamAutoProcessingMode(_videoProcessor.Get(), 0, FALSE);
		_videoContext->VideoProcessorSetOutputTargetRect(_videoProcessor.Get(), TRUE, &frameRect);
		_videoContext1->VideoProcessorSetStreamColorSpace1(_videoProcessor.Get(), 0, CaptureWindowsInternals::SourceColorSpace);
		_videoContext1->VideoProcessorSetOutputColorSpace1(_videoProcessor.Get(), CaptureWindowsInternals::EncoderColorSpace);
	}

	bool CaptureWindowsVideoProcessor::CreateVideoInputTexture(ID3D11Texture2D *sourceTexture, ComPtr<ID3D11Texture2D> &videoInputTexture)
	{
		D3D11_TEXTURE2D_DESC inputDescription = {};
		inputDescription.Width = _width;
		inputDescription.Height = _height;
		inputDescription.MipLevels = 1;
		inputDescription.ArraySize = 1;
		inputDescription.Format = CaptureWindowsInternals::VideoProcessorInputTextureFormat;
		inputDescription.SampleDesc.Count = 1;
		inputDescription.SampleDesc.Quality = 0;
		inputDescription.Usage = D3D11_USAGE_DEFAULT;
		inputDescription.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		inputDescription.CPUAccessFlags = 0;
		inputDescription.MiscFlags = 0;

		HRESULT result = _d3dDevice->CreateTexture2D(&inputDescription, nullptr, &videoInputTexture);
		if(CaptureWindowsInternals::LogD3DMediaFailure("BGRA video processor input texture creation", result)) return false;

		CaptureScopedKeyedMutex sourceMutex(sourceTexture);
		if(!sourceMutex.GetIsValid()) return false;

		_d3dContext->CopyResource(videoInputTexture.Get(), sourceTexture);
		return true;
	}

	void ReleaseCaptureWindowsSwapChainAfterRenderThreadDrain(CaptureWindowsSwapChain *swapChain, std::function<void()> completion)
	{
		if(!swapChain)
		{
			if(completion) completion();
			return;
		}

		Renderer::GetActiveRenderer()->ScheduleRenderThreadWork([swapChain, completion] {
			WorkGroup *group = new WorkGroup();
			group->Perform(WorkQueue::GetMainQueue(), [swapChain, completion] {
				AutoreleasePool pool;
				swapChain->Release();
				if(completion) completion();
			});
			group->Release();
		});
	}

	std::shared_ptr<CaptureWindowsEncodingSession> CaptureWindowsEncodingSession::WithMediaSink(const CaptureSession::Configuration &configuration, ID3D11Device *d3dDevice, std::unique_ptr<CaptureEncodedSink> mediaSink)
	{
		std::shared_ptr<CaptureWindowsEncodingSession> session(new CaptureWindowsEncodingSession());
		if(!session->Start(configuration, d3dDevice, std::move(mediaSink))) return nullptr;
		return session;
	}

	CaptureWindowsEncodingSession::CaptureWindowsEncodingSession() :
		_pendingVideoFrameCount(0),
		_writtenVideoFrameCount(0),
		_writtenAudioSampleCount(0),
		_swapChain(nullptr),
		_isFinishing(false),
		_didFinish(false)
	{}

	CaptureWindowsEncodingSession::~CaptureWindowsEncodingSession()
	{
		if(_swapChain) _swapChain->SetFrameReadyCallback(nullptr);
		if(_mediaEncoder) _mediaEncoder->Stop();
		if(_mediaSink) _mediaSink->Stop();
	}

	bool CaptureWindowsEncodingSession::Start(const CaptureSession::Configuration &configuration, ID3D11Device *d3dDevice, std::unique_ptr<CaptureEncodedSink> mediaSink)
	{
		if(!mediaSink) return false;

		_mediaSink = std::move(mediaSink);
		if(!_mediaSink->Start(configuration)) return false;

		_mediaEncoder = std::make_unique<CaptureWindowsEncoder>();
		if(!_mediaEncoder->Start(configuration, d3dDevice, _mediaSink.get())) return false;

		return true;
	}

	void CaptureWindowsEncodingSession::AddPendingFrame()
	{
		_pendingVideoFrameCount.fetch_add(1);
	}

	void CaptureWindowsEncodingSession::CancelPendingFrames(size_t frameCount)
	{
		for(size_t i = 0; i < frameCount; i++)
		{
			DecrementPendingFrame();
		}
	}

	void CaptureWindowsEncodingSession::WriteFrame(ID3D11Texture2D *texture, int64 presentationTimeUs, int64 durationUs)
	{
		CaptureWindowsEncoder *mediaEncoder = nullptr;
		{
			LockGuard<Lockable> lock(_finishLock);
			if(!_didFinish) mediaEncoder = _mediaEncoder.get();
		}

		if(mediaEncoder && mediaEncoder->WriteFrame(texture, presentationTimeUs, durationUs))
		{
			_writtenVideoFrameCount.fetch_add(1);
		}

		DecrementPendingFrame();
	}

	bool CaptureWindowsEncodingSession::WriteAudioSamples(const uint8 *samples, size_t sampleCount, int64 presentationTimeUs)
	{
		CaptureWindowsEncoder *mediaEncoder = nullptr;
		{
			LockGuard<Lockable> lock(_finishLock);
			if(!_didFinish) mediaEncoder = _mediaEncoder.get();
		}

		if(!mediaEncoder) return false;
		if(!mediaEncoder->WriteAudioSamples(samples, sampleCount, presentationTimeUs)) return false;

		_writtenAudioSampleCount.fetch_add(sampleCount);
		return true;
	}

	void CaptureWindowsEncodingSession::FinishWhenReady(CaptureWindowsSwapChain *swapChain, std::function<void()> completion)
	{
		bool shouldFinishNow = false;
		{
			LockGuard<Lockable> lock(_finishLock);
			if(_isFinishing) return;

			_isFinishing = true;
			_swapChain = swapChain;
			_completion = completion;
			_keepAlive = shared_from_this();
			shouldFinishNow = (_pendingVideoFrameCount.load() == 0);
		}

		if(shouldFinishNow)
		{
			FinishNow();
		}
	}

	uint64 CaptureWindowsEncodingSession::GetPendingVideoFrameCount() const
	{
		return _pendingVideoFrameCount.load();
	}

	uint64 CaptureWindowsEncodingSession::GetWrittenVideoFrameCount() const
	{
		return _writtenVideoFrameCount.load();
	}

	uint64 CaptureWindowsEncodingSession::GetWrittenAudioSampleCount() const
	{
		return _writtenAudioSampleCount.load();
	}

	void CaptureWindowsEncodingSession::DecrementPendingFrame()
	{
		uint64 pendingFrameCount = _pendingVideoFrameCount.load();
		while(pendingFrameCount != 0)
		{
			if(_pendingVideoFrameCount.compare_exchange_weak(pendingFrameCount, pendingFrameCount - 1))
			{
				if(pendingFrameCount == 1)
				{
					TryFinishPendingCapture();
				}

				return;
			}
		}
	}

	void CaptureWindowsEncodingSession::TryFinishPendingCapture()
	{
		if(_pendingVideoFrameCount.load() != 0) return;

		bool shouldFinish = false;
		{
			LockGuard<Lockable> lock(_finishLock);
			shouldFinish = _isFinishing && !_didFinish;
		}

		if(!shouldFinish) return;

		std::shared_ptr<CaptureWindowsEncodingSession> self = shared_from_this();
		WorkGroup *group = new WorkGroup();
		group->Perform(WorkQueue::GetMainQueue(), [self] {
			AutoreleasePool pool;
			self->FinishNow();
		});
		group->Release();
	}

	void CaptureWindowsEncodingSession::FinishNow()
	{
		CaptureWindowsSwapChain *swapChain = nullptr;
		std::unique_ptr<CaptureWindowsEncoder> mediaEncoder;
		std::unique_ptr<CaptureEncodedSink> mediaSink;
		std::function<void()> completion;
		{
			LockGuard<Lockable> lock(_finishLock);
			if(_didFinish) return;

			_didFinish = true;
			swapChain = _swapChain;
			_swapChain = nullptr;
			mediaEncoder = std::move(_mediaEncoder);
			mediaSink = std::move(_mediaSink);
			completion = _completion;
			_completion = nullptr;
		}

		if(swapChain) swapChain->SetFrameReadyCallback(nullptr);
		if(mediaEncoder)
		{
			mediaEncoder->Stop();
		}
		if(mediaSink)
		{
			mediaSink->Stop();
		}

		RNInfo("Finished Windows media session after writing " << _writtenVideoFrameCount.load() << " video frames and " << _writtenAudioSampleCount.load() << " audio samples");

		std::shared_ptr<CaptureWindowsEncodingSession> self = shared_from_this();
		ReleaseCaptureWindowsSwapChainAfterRenderThreadDrain(swapChain, [self, completion] {
			if(completion) completion();

			LockGuard<Lockable> lock(self->_finishLock);
			self->_keepAlive.reset();
		});
	}
} // namespace RN

#endif
