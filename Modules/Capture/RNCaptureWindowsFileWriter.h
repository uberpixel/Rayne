//
//  RNCaptureWindowsFileWriter.h
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RN_CAPTURE_WINDOWS_FILE_WRITER_H_
#define __RN_CAPTURE_WINDOWS_FILE_WRITER_H_

#include "RNCaptureEncodedSink.h"
#include "RNCaptureWindowsInternals.h"

#if RN_PLATFORM_WINDOWS

namespace RN
{
	class CaptureWindowsFileWriter final : public CaptureEncodedSink
	{
	public:
		CaptureWindowsFileWriter();
		~CaptureWindowsFileWriter() override;

		bool Start(const CaptureSession::Configuration &configuration) override;
		void Stop() override;

		bool DidReceiveVideoFormat(const CaptureEncodedVideoFormat &format) override;
		bool DidReceiveAudioFormat(const CaptureEncodedAudioFormat &format) override;
		bool GetIsReady() const override;

		void WriteVideoSample(const CaptureEncodedSample &sample) override;
		void WriteAudioSample(const CaptureEncodedSample &sample) override;

	private:
		bool CreateOutputPath(const CaptureSession::Configuration &configuration);
		bool SetBlobAttribute(IMFMediaType *mediaType, const GUID &key, const uint8 *data, size_t size);
		bool ConfigureVideoStream(const CaptureEncodedVideoFormat &format);
		bool ConfigureAudioStream(const CaptureEncodedAudioFormat &format);
		bool StartWritingIfReady();
		bool BuildVideoSample(const CaptureEncodedSample &sample, const std::vector<uint8> &sequenceHeader, std::vector<uint8> &data);
		bool WriteSample(DWORD streamIndex, const CaptureEncodedSample &sample, const uint8 *data, size_t size, bool isVideo);

		Lockable _lock;
		Microsoft::WRL::ComPtr<IMFByteStream> _byteStream;
		Microsoft::WRL::ComPtr<IMFMediaSink> _mediaSink;
		Microsoft::WRL::ComPtr<IMFSinkWriter> _sinkWriter;
		Microsoft::WRL::ComPtr<IMFMediaType> _videoMediaType;
		Microsoft::WRL::ComPtr<IMFMediaType> _audioMediaType;
		std::vector<uint8> _videoSequenceHeader;
		DWORD _videoStreamIndex;
		DWORD _audioStreamIndex;
		UINT32 _videoFrameRate;
		UINT32 _videoBitRate;
		UINT32 _audioBitRate;
		String *_outputPath;
		bool _didInitializeCOM;
		bool _didStartMediaFoundation;
		bool _isStarted;
		bool _hasVideoFormat;
		bool _hasAudioFormat;
		bool _didWriteVideoSample;
		bool _didWriteAudioSample;
		bool _isReady;
	};
} // namespace RN

#endif

#endif /* defined(__RN_CAPTURE_WINDOWS_FILE_WRITER_H_) */
