//
//  RNCaptureEncodedSink.h
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RN_CAPTURE_ENCODED_SINK_H_
#define __RN_CAPTURE_ENCODED_SINK_H_

#include "RNCaptureSession.h"

namespace RN
{
	struct CaptureEncodedVideoFormat
	{
		int32 width = 0;
		int32 height = 0;

		const uint8 *sequenceParameterSet = nullptr;
		size_t sequenceParameterSetSize = 0;
		const uint8 *pictureParameterSet = nullptr;
		size_t pictureParameterSetSize = 0;

		const uint8 *codecConfiguration = nullptr;
		size_t codecConfigurationSize = 0;
		const uint8 *codecConfiguration2 = nullptr;
		size_t codecConfiguration2Size = 0;
	};

	struct CaptureEncodedAudioFormat
	{
		int32 sampleRate = 0;
		int32 channelCount = 0;

		const uint8 *audioSpecificConfig = nullptr;
		size_t audioSpecificConfigSize = 0;
	};

	struct CaptureEncodedSample
	{
		const uint8 *data = nullptr;
		size_t offset = 0;
		size_t size = 0;
		int64 presentationTimeUs = 0;
		int64 durationUs = 0;
		bool isKeyFrame = false;
		bool isCodecConfig = false;
		bool isEndOfStream = false;
	};

	class CaptureEncodedSink
	{
	public:
		virtual ~CaptureEncodedSink() = default;

		virtual bool Start(const CaptureSession::Configuration &configuration) = 0;
		virtual void Stop() = 0;

		virtual bool DidReceiveVideoFormat(const CaptureEncodedVideoFormat &format) = 0;
		virtual bool DidReceiveAudioFormat(const CaptureEncodedAudioFormat &format) = 0;
		virtual bool GetIsReady() const = 0;

		virtual void WriteVideoSample(const CaptureEncodedSample &sample) = 0;
		virtual void WriteAudioSample(const CaptureEncodedSample &sample) = 0;
	};
} // namespace RN

#endif /* defined(__RN_CAPTURE_ENCODED_SINK_H_) */
