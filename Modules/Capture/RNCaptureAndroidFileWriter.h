//
//  RNCaptureAndroidFileWriter.h
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RN_CAPTURE_ANDROID_FILE_WRITER_H_
#define __RN_CAPTURE_ANDROID_FILE_WRITER_H_

#include "RNCaptureEncodedSink.h"

#if RN_PLATFORM_ANDROID
	#include <sys/types.h>

struct AMediaFormat;
struct AMediaMuxer;
#endif

namespace RN
{
#if RN_PLATFORM_ANDROID
	class CaptureAndroidFileWriter : public CaptureEncodedSink
	{
	public:
		CaptureAndroidFileWriter();
		~CaptureAndroidFileWriter() override;

		bool Start(const CaptureSession::Configuration &configuration) override;
		void Stop() override;

		bool DidReceiveVideoFormat(const CaptureEncodedVideoFormat &format) override;
		bool DidReceiveAudioFormat(const CaptureEncodedAudioFormat &format) override;
		bool GetIsReady() const override;

		void WriteVideoSample(const CaptureEncodedSample &sample) override;
		void WriteAudioSample(const CaptureEncodedSample &sample) override;

	private:
		static constexpr uint32 AndroidCodecBufferFlagKeyFrame = 1;

		bool StartMuxerIfReady();
		bool AddBufferToFormat(AMediaFormat *mediaFormat, const char *name, const uint8 *data, size_t size);
		void WriteSample(ssize_t trackIndex, const CaptureEncodedSample &sample);

		int _videoFD;
		String *_outputPath = nullptr;
		std::function<void(int, bool)> _finishFileCallback;
		AMediaMuxer *_muxer;
		ssize_t _videoTrackIndex;
		ssize_t _audioTrackIndex;
		bool _hasVideoFormat;
		bool _hasAudioFormat;
		bool _isReady;
		uint32 _sampleWriteFailureCount;
	};
#endif
} // namespace RN

#endif /* defined(__RN_CAPTURE_ANDROID_FILE_WRITER_H_) */
