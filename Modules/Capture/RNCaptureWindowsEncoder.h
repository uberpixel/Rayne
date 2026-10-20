//
//  RNCaptureWindowsEncoder.h
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RN_CAPTURE_WINDOWS_ENCODER_H_
#define __RN_CAPTURE_WINDOWS_ENCODER_H_

#include "RNCaptureEncodedSink.h"
#include "RNCaptureWindowsInternals.h"

#if RN_PLATFORM_WINDOWS

namespace RN
{
	class CaptureWindowsEncoder final
	{
	public:
		CaptureWindowsEncoder();
		~CaptureWindowsEncoder();

		bool Start(const CaptureSession::Configuration &configuration, ID3D11Device *d3dDevice, CaptureEncodedSink *mediaSink);
		void Stop();

		bool WriteFrame(ID3D11Texture2D *sourceTexture, int64 presentationTimeUs, int64 durationUs);
		bool WriteAudioSamples(const uint8 *samples, size_t sampleCount, int64 presentationTimeUs);

	private:
		struct PendingTiming
		{
			int64 presentationTimeUs = 0;
			int64 durationUs = 0;
		};

		bool ConfigureVideoEncoder(const CaptureSession::Configuration &configuration);
		bool ConfigureAudioEncoder(const CaptureSession::Configuration &configuration);
		bool CreateDXGIDeviceManager(ID3D11Device *d3dDevice);
		bool ConfigureVideoEncoderD3DManager(IMFAttributes *attributes);
		bool CreateTransform(const GUID &category, const GUID &inputSubtype, const GUID &outputSubtype, IMFTransform **transform);
		bool GetTransformStreamIDs(IMFTransform *transform, DWORD &inputStreamID, DWORD &outputStreamID);
		bool ProcessVideoEncoderEvents(bool &didDrain);
		bool SendAudioFormat();
		bool SendVideoFormatFromCurrentType();
		bool SendVideoFormatFromSample(const std::vector<uint8> &data);
		bool DrainVideoOutput();
		bool DrainAudioOutput();
		bool DrainTransformOutput(IMFTransform *transform, DWORD outputStreamID, bool isVideo);
		bool HandleEncodedVideoSample(IMFSample *sample);
		bool HandleEncodedAudioSample(IMFSample *sample);
		PendingTiming TakePendingTiming(std::deque<PendingTiming> &pendingTiming, IMFSample *sample);
		bool GetSampleTimestampUs(IMFSample *sample, int64 &presentationTimeUs) const;
		bool GetSampleDurationUs(IMFSample *sample, int64 &durationUs) const;
		bool IsH264KeyFrame(IMFSample *sample, const std::vector<uint8> &data) const;
		bool ContainsH264NALType(const uint8 *data, size_t size, uint8 nalType) const;

		Lockable _lock;
		CaptureEncodedSink *_mediaSink;
		CaptureWindowsVideoProcessor _videoProcessor;
		Microsoft::WRL::ComPtr<IMFTransform> _videoEncoder;
		Microsoft::WRL::ComPtr<IMFTransform> _audioEncoder;
		Microsoft::WRL::ComPtr<IMFMediaEventGenerator> _videoEventGenerator;
		Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> _dxgiDeviceManager;
		DWORD _videoInputStreamID;
		DWORD _videoOutputStreamID;
		DWORD _audioInputStreamID;
		DWORD _audioOutputStreamID;
		UINT _dxgiDeviceManagerResetToken;
		std::deque<PendingTiming> _pendingVideoTiming;
		std::deque<PendingTiming> _pendingAudioTiming;
		int64 _nextFallbackVideoPresentationTimeUs;
		int64 _nextFallbackAudioPresentationTimeUs;
		bool _didInitializeCOM;
		bool _didStartMediaFoundation;
		bool _hasVideoFormat;
		bool _hasAudioFormat;
		bool _usesD3DVideoInput;
		bool _usesAsyncVideoEncoder;
		bool _isReady;
	};
} // namespace RN

#endif

#endif /* defined(__RN_CAPTURE_WINDOWS_ENCODER_H_) */
