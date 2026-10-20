//
//  RNCaptureRTMPWriter.h
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RN_CAPTURE_RTMP_WRITER_H_
#define __RN_CAPTURE_RTMP_WRITER_H_

#include "RNCaptureEncodedSink.h"

namespace RN
{
	class CAPTUREAPI CaptureRTMPWriter : public CaptureEncodedSink
	{
	public:
		CaptureRTMPWriter();
		~CaptureRTMPWriter() override;

		bool Start(const CaptureSession::Configuration &configuration) override;
		void Stop() override;

		bool DidReceiveVideoFormat(const CaptureEncodedVideoFormat &format) override;
		bool DidReceiveAudioFormat(const CaptureEncodedAudioFormat &format) override;
		bool GetIsReady() const override;

		void WriteVideoSample(const CaptureEncodedSample &sample) override;
		void WriteAudioSample(const CaptureEncodedSample &sample) override;

		static bool TestConnection(const String *streamURL, String **errorMessage);
		static bool TestPublishConnection(const String *streamURL, String **errorMessage);

	private:
		struct State;
		State *_state;
	};
} // namespace RN

#endif /* defined(__RN_CAPTURE_RTMP_WRITER_H_) */
