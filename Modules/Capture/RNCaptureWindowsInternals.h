//
//  RNCaptureWindowsInternals.h
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RN_CAPTURE_WINDOWS_INTERNALS_H_
#define __RN_CAPTURE_WINDOWS_INTERNALS_H_

#include "RNCaptureEncodedSink.h"
#include "RNCaptureSession.h"
#include "RNCaptureWindowsSwapChain.h"

#if RN_PLATFORM_WINDOWS

	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	#include <unknwn.h>
	#include <windows.h>

	#ifndef far
		#define far
		#define RN_CAPTURE_RESTORE_WINDOWS_FAR_MACRO
	#endif
	#ifndef near
		#define near
		#define RN_CAPTURE_RESTORE_WINDOWS_NEAR_MACRO
	#endif
	#include <d3d11_1.h>
	#include <mfapi.h>
	#include <mferror.h>
	#include <mfidl.h>
	#include <mfreadwrite.h>
	#include <mftransform.h>
	#ifdef RN_CAPTURE_RESTORE_WINDOWS_FAR_MACRO
		#undef far
		#undef RN_CAPTURE_RESTORE_WINDOWS_FAR_MACRO
	#endif
	#ifdef RN_CAPTURE_RESTORE_WINDOWS_NEAR_MACRO
		#undef near
		#undef RN_CAPTURE_RESTORE_WINDOWS_NEAR_MACRO
	#endif
	#include <wrl/client.h>

namespace RN
{
	namespace CaptureWindowsInternals
	{
		static constexpr uint32_t AudioSampleRate = 48000;
		static constexpr uint32_t AudioChannelCount = 2;
		static constexpr uint32_t AudioBitsPerSample = 16;
		static constexpr uint32_t AudioBytesPerChannelSample = AudioBitsPerSample / 8;
		static constexpr uint32_t AudioFrameBytes = AudioChannelCount * AudioBytesPerChannelSample;
		static constexpr uint32_t DefaultFileVideoBitRate = 20000000;
		static constexpr uint32_t DefaultStreamVideoBitRate = 8000000;
		static constexpr uint32_t DefaultAudioBitRate = 128000;
		static constexpr DXGI_FORMAT CaptureSourceTextureFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		static constexpr DXGI_FORMAT VideoProcessorInputTextureFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
		static constexpr DXGI_FORMAT EncoderTextureFormat = DXGI_FORMAT_NV12;
		static constexpr DXGI_COLOR_SPACE_TYPE SourceColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		static constexpr DXGI_COLOR_SPACE_TYPE EncoderColorSpace = DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709;

		inline int64 GetAudioDurationForSamples(uint64 samples)
		{
			return static_cast<int64>((samples * 1000000) / AudioSampleRate);
		}

		std::wstring CreateWindowsWideString(const String *string);
		bool LogMediaFailure(const char *operation, HRESULT result);
		bool LogD3DMediaFailure(const char *operation, HRESULT result);
		UINT32 GetVideoFrameRate(const CaptureSession::Configuration &configuration);
		bool SetVideoColorMetadata(IMFMediaType *mediaType);
		bool CreateMemorySample(const uint8 *data, size_t byteCount, int64 presentationTimeUs, int64 durationUs, Microsoft::WRL::ComPtr<IMFSample> &sample);
		bool CreateDXGISurfaceSample(ID3D11Texture2D *texture, int64 presentationTimeUs, int64 durationUs, Microsoft::WRL::ComPtr<IMFSample> &sample);
		bool CopySampleData(IMFSample *sample, std::vector<uint8> &data);
	} // namespace CaptureWindowsInternals

	class CaptureWindowsEncoder;

	class CaptureScopedCOMInitialization
	{
	public:
		CaptureScopedCOMInitialization();
		~CaptureScopedCOMInitialization();

		bool GetIsValid() const;

	private:
		bool _didInitialize;
		bool _isValid;
	};

	class CaptureScopedKeyedMutex
	{
	public:
		CaptureScopedKeyedMutex(ID3D11Texture2D *texture);
		~CaptureScopedKeyedMutex();

		bool GetIsValid() const;

	private:
		Microsoft::WRL::ComPtr<IDXGIKeyedMutex> _keyedMutex;
		bool _didAcquire;
		bool _isValid;
	};

	class CaptureWindowsVideoProcessor
	{
	public:
		CaptureWindowsVideoProcessor();
		~CaptureWindowsVideoProcessor();

		bool Start(ID3D11Device *d3dDevice, const CaptureSession::Configuration &configuration, const char *logContext);
		void Stop();

		bool CreateEncoderTexture(ID3D11Texture2D *sourceTexture, Microsoft::WRL::ComPtr<ID3D11Texture2D> &encoderTexture);

		UINT32 GetWidth() const;
		UINT32 GetHeight() const;
		UINT32 GetFrameRate() const;

	private:
		bool CheckD3DFormatSupport(DXGI_FORMAT format, UINT requiredFlags, const char *name);
		bool CheckVideoProcessorFormat(DXGI_FORMAT format, UINT requiredFlags, const char *name);
		bool CheckVideoProcessorConversion();
		void ConfigureVideoProcessorState();
		bool CreateVideoInputTexture(ID3D11Texture2D *sourceTexture, Microsoft::WRL::ComPtr<ID3D11Texture2D> &videoInputTexture);

		Lockable _lock;
		Microsoft::WRL::ComPtr<ID3D11Device> _d3dDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> _d3dContext;
		Microsoft::WRL::ComPtr<ID3D11VideoDevice> _videoDevice;
		Microsoft::WRL::ComPtr<ID3D11VideoContext> _videoContext;
		Microsoft::WRL::ComPtr<ID3D11VideoContext1> _videoContext1;
		Microsoft::WRL::ComPtr<ID3D11VideoProcessorEnumerator> _videoProcessorEnumerator;
		Microsoft::WRL::ComPtr<ID3D11VideoProcessor> _videoProcessor;
		UINT32 _width;
		UINT32 _height;
		UINT32 _frameRate;
		bool _isReady;
	};

	void ReleaseCaptureWindowsSwapChainAfterRenderThreadDrain(CaptureWindowsSwapChain *swapChain, std::function<void()> completion);

	class CaptureWindowsEncodingSession final : public std::enable_shared_from_this<CaptureWindowsEncodingSession>
	{
	public:
		static std::shared_ptr<CaptureWindowsEncodingSession> WithMediaSink(const CaptureSession::Configuration &configuration, ID3D11Device *d3dDevice, std::unique_ptr<CaptureEncodedSink> mediaSink);
		~CaptureWindowsEncodingSession();

		void AddPendingFrame();
		void CancelPendingFrames(size_t frameCount);

		void WriteFrame(ID3D11Texture2D *texture, int64 presentationTimeUs, int64 durationUs);
		bool WriteAudioSamples(const uint8 *samples, size_t sampleCount, int64 presentationTimeUs);
		void FinishWhenReady(CaptureWindowsSwapChain *swapChain, std::function<void()> completion);

		uint64 GetPendingVideoFrameCount() const;
		uint64 GetWrittenVideoFrameCount() const;
		uint64 GetWrittenAudioSampleCount() const;

	private:
		CaptureWindowsEncodingSession();

		bool Start(const CaptureSession::Configuration &configuration, ID3D11Device *d3dDevice, std::unique_ptr<CaptureEncodedSink> mediaSink);
		void DecrementPendingFrame();
		void TryFinishPendingCapture();
		void FinishNow();

		Lockable _finishLock;
		std::unique_ptr<CaptureWindowsEncoder> _mediaEncoder;
		std::unique_ptr<CaptureEncodedSink> _mediaSink;
		std::atomic<uint64> _pendingVideoFrameCount;
		std::atomic<uint64> _writtenVideoFrameCount;
		std::atomic<uint64> _writtenAudioSampleCount;
		CaptureWindowsSwapChain *_swapChain;
		std::function<void()> _completion;
		std::shared_ptr<CaptureWindowsEncodingSession> _keepAlive;
		bool _isFinishing;
		bool _didFinish;
	};
} // namespace RN

#endif

#endif /* defined(__RN_CAPTURE_WINDOWS_INTERNALS_H_) */
