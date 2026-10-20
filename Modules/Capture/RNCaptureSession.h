//
//  RNCaptureSession.h
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_CAPTURE_SESSION_H_
#define __RAYNE_CAPTURE_SESSION_H_

#include "RNCapture.h"

namespace RN
{
	class CaptureAudioSource;

	class CAPTUREAPI CaptureSession
	{
	public:
		struct Configuration
		{
			PostProcessingAPIStage *resolvePass = nullptr;
			Framebuffer *restoreFramebuffer = nullptr;
			const String *filename = nullptr;
			const String *streamURL = nullptr;

			int resolutionX = 0;
			int resolutionY = 0;
			int videoBitRate = 0;
			int videoMaxBitRate = 0;
			int audioBitRate = 0;
			double videoFramePeriod = 1.0 / 30.0;

			std::function<void()> completionCallback;
			std::function<void(const String *)> errorCallback;

			// Optional PCM16 stereo 48 kHz pull source for manual audio rendering
			// The caller owns listener setup and keeps these callbacks valid until completion
			std::function<size_t(float)> audioFrameCountCallback;

			// The callback must advance without writing when destination is nullptr
			std::function<void(size_t, void *)> audioSamplesCallback;

			// Optional Android MediaStore integration
			// openFileCallback returns an owned writable fd or -1
			// The session closes it before finishFileCallback(fd, keepFile)
			// keepFile is true only after successful muxer finalization
			// Both callbacks and their captures must remain valid until completion
			std::function<int(const String *)> openFileCallback;
			std::function<void(int, bool)> finishFileCallback;

			bool IsValid() const;
		};

		CaptureSession();
		virtual ~CaptureSession() = default;
		static CaptureSession *WithPlatform();

		// True accepts startup and initialization errors may arrive asynchronously through errorCallback
		virtual bool Start(const Configuration &configuration) = 0;
		virtual void Stop() = 0;

		// Apple drains native work before teardown while other backends retain asynchronous Stop behavior
		virtual void PrepareForShutdown() { Stop(); }
		virtual void Update(float delta) = 0;

		// Apple includes pending initialization and active capture ends at Stop while finishing remains true until completion
		virtual bool GetIsRecording() const = 0;
		virtual bool GetIsFinishing() const = 0;
		virtual Framebuffer *GetFramebuffer() const = 0;

		// Push source accepts interleaved stereo float samples at 48 kHz
		// Missing and overwritten samples become silence without shortening the timeline
		// After underrun resumed samples follow the emitted silence
		// Submission never waits and returns false if the bounded queue is busy
		bool SubmitAudioSamples(const float *samples, size_t frames, uint32 sampleRate = 48000);
		const std::shared_ptr<CaptureAudioSource> &GetAudioSource() const { return _audioSource; }

	private:
		std::shared_ptr<CaptureAudioSource> _audioSource;
	};

	class CAPTUREAPI CaptureAudioSource
	{
	public:
		CaptureAudioSource();
		void Configure(const CaptureSession::Configuration &configuration);
		bool SubmitSamples(const float *samples, size_t frames, uint32 sampleRate);
		size_t GetFrameCount(float duration);
		void ReadFrames(size_t frames, void *destination);

	private:
		static constexpr size_t Capacity = 48000 * 2;
		Lockable _lock;
		int16 _samples[Capacity * 2];
		uint64 _sampleFrames[Capacity] = {};
		std::atomic<uint64> _droppedFrames;
		uint64 _readFrame;
		uint64 _writeFrame;
		double _fractionalFrames;
		std::function<size_t(float)> _frameCountCallback;
		std::function<void(size_t, void *)> _samplesCallback;
	};
} // namespace RN

#endif
