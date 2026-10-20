//
//  RNCaptureAndroidSession.cpp
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCaptureSession.h"

#if RN_PLATFORM_ANDROID

	#include "RNCaptureAndroidFileWriter.h"
	#include "RNCaptureRTMPWriter.h"

	#include <time.h>

	#include <android/api-level.h>
	#include <media/NdkMediaCodec.h>
	#include <media/NdkMediaFormat.h>

namespace RN
{
	class CaptureAndroidSession final : public CaptureSession
	{
	public:
		CaptureAndroidSession();
		~CaptureAndroidSession() override;

		bool Start(const Configuration &configuration) override;
		void Stop() override;
		void Update(float delta) override;

		bool GetIsRecording() const override;
		bool GetIsFinishing() const override;
		Framebuffer *GetFramebuffer() const override;

	private:
		struct State;
		std::shared_ptr<State> _state;
	};

	struct CaptureAndroidSession::State : public std::enable_shared_from_this<State>
	{
		static constexpr int AudioSampleRate = 48000;
		static constexpr size_t AudioChannelCount = 2;
		static constexpr size_t AudioBytesPerChannelSample = 2;
		static constexpr size_t AudioFrameBytes = AudioChannelCount * AudioBytesPerChannelSample;
		static constexpr int DefaultFileVideoBitRate = 20000000;
		static constexpr int DefaultFileVideoMaxBitRate = 40000000;
		static constexpr int DefaultStreamVideoBitRate = 8000000;
		static constexpr int DefaultStreamVideoMaxBitRate = 12000000;
		static constexpr int DefaultAudioBitRate = 128000;
		static constexpr int64 AudioInputBufferTimeoutUs = 2000;
		static constexpr size_t MaxStreamPendingAudioSamples = AudioSampleRate / 4;
		static constexpr uint32 AndroidCodecBufferFlagKeyFrame = 1;

		State() :
			isRecording(false),
			isWaitingToEndRecording(false),
			isReleasingRecordingResources(false),
			isDrainThreadRunning(false),
			window(nullptr),
			surfaceWindow(nullptr),
			encoderVideo(nullptr),
			encoderAudio(nullptr),
			mediaSink(nullptr),
			audioSubmittedSamples(0),
			audioInputBufferMissCount(0),
			audioSkippedSamples(0),
			audioSkipWarningCount(0),
			scheduledVideoFrameCount(0),
			writtenVideoFrameCount(0),
			hasStartedMediaTimeline(false),
			hasQueuedAudioEndOfStream(false),
			hasReachedVideoEndOfStream(false),
			hasReachedAudioEndOfStream(false),
			firstVideoPresentationTimeUs(0),
			firstAudioPresentationTimeUs(0),
			lastVideoOutputPresentationTimeUs(0),
			lastAudioOutputPresentationTimeUs(0),
			recordingStartTimeUs(0),
			lastScheduledPresentationTimeUs(0),
			hasFirstVideoPresentationTime(false),
			hasFirstAudioPresentationTime(false),
			hasLastVideoOutputPresentationTime(false),
			hasLastAudioOutputPresentationTime(false),
			videoMediaSinkNotReadyCount(0),
			audioMediaSinkNotReadyCount(0),
			isMediaSinkActiveForVideo(false),
			isMediaSinkActiveForAudio(false),
			isMediaSinkReady(false),
			audioPendingDurationUs(0),
			audioPendingSamples(0),
			videoFrameAccumulator(0.0)
		{}

		~State() = default;

		CaptureEncodedSink *CreateMediaSink() const
		{
	#if RN_CAPTURE_ENABLE_RTMP
			if(configuration.streamURL) return new CaptureRTMPWriter();
	#endif
			return new CaptureAndroidFileWriter();
		}

		bool Start(const Configuration &newConfiguration)
		{
			if(GetIsRecording()) return false;

			if(newConfiguration.streamURL && !RN_CAPTURE_ENABLE_RTMP) return false;
			configuration = newConfiguration;
			retainedResolvePass = configuration.resolvePass;
			retainedRestoreFramebuffer = configuration.restoreFramebuffer;
			retainedFilename = configuration.filename;
			retainedStreamURL = configuration.streamURL;
			isRecording = true;
			ResetCaptureState();
			videoFrameAccumulator = configuration.videoFramePeriod;

			encoderVideo = AMediaCodec_createEncoderByType("video/avc");
			if(!encoderVideo)
			{
				RNWarning("Failed to create video encoder");
				StopInternal(false);
				return false;
			}

			AMediaFormat *format = AMediaFormat_new();
			AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, configuration.resolutionX);
			AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, configuration.resolutionY);
			AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, "video/avc"); // H.264 Advanced Video Coding
			AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, 2130708361); // COLOR_FormatSurface
			AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BITRATE_MODE, 1); //1 = VBR
			AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COMPLEXITY, 3);
			int defaultVideoBitRate = configuration.streamURL ? DefaultStreamVideoBitRate : DefaultFileVideoBitRate;
			int defaultVideoMaxBitRate = configuration.streamURL ? DefaultStreamVideoMaxBitRate : DefaultFileVideoMaxBitRate;
			int videoBitRate = configuration.videoBitRate > 0 ? configuration.videoBitRate : defaultVideoBitRate;
			int videoMaxBitRate = configuration.videoMaxBitRate > 0 ? configuration.videoMaxBitRate : defaultVideoMaxBitRate;
			AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, videoBitRate);
			AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_MAX_BIT_RATE, videoMaxBitRate);
			AMediaFormat_setFloat(format, AMEDIAFORMAT_KEY_FRAME_RATE, 30);
			AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 2); //Time in seconds between keyframes!

			media_status_t status = AMediaCodec_configure(encoderVideo, format, nullptr, nullptr, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
			AMediaFormat_delete(format);
			if(status != AMEDIA_OK)
			{
				RNWarning("Failed configuring video encoder");
				StopInternal(false);
				return false;
			}

			status = AMediaCodec_createInputSurface(encoderVideo, &surfaceWindow);
			if(status != AMEDIA_OK || !surfaceWindow)
			{
				RNWarning("Failed creating video encoder surface " << status);
				StopInternal(false);
				return false;
			}

			Window::SwapChainDescriptor swapChainDescriptor;
			swapChainDescriptor.colorFormat = Texture::Format::RGBA_8_SRGB;
			swapChainDescriptor.usageHint = Texture::UsageHint::RenderTarget;
			//swapChainDescriptor.bufferCount = 1;
			//swapChainDescriptor.vsync = false;
			window = Renderer::GetActiveRenderer()->CreateAWindow(Vector2(configuration.resolutionX, configuration.resolutionY), nullptr, swapChainDescriptor, surfaceWindow);

			encoderAudio = AMediaCodec_createEncoderByType("audio/mp4a-latm");
			if(!encoderAudio)
			{
				RNWarning("Failed to create audio encoder");
				StopInternal(false);
				return false;
			}

			AMediaFormat *formatAudio = AMediaFormat_new();
			AMediaFormat_setString(formatAudio, AMEDIAFORMAT_KEY_MIME, "audio/mp4a-latm");
			//AMediaFormat_setInt32(formatAudio, AMEDIAFORMAT_KEY_AAC_PROFILE, AMediaCodecInfo.CodecProfileLevel.AACObjectLC); // Maybe try MPEG4ProfileMain
			AMediaFormat_setInt32(formatAudio, AMEDIAFORMAT_KEY_SAMPLE_RATE, AudioSampleRate);
			AMediaFormat_setInt32(formatAudio, AMEDIAFORMAT_KEY_CHANNEL_COUNT, AudioChannelCount);
			int audioBitRate = configuration.audioBitRate > 0 ? configuration.audioBitRate : DefaultAudioBitRate;
			AMediaFormat_setInt32(formatAudio, AMEDIAFORMAT_KEY_BIT_RATE, audioBitRate);
			AMediaFormat_setInt32(formatAudio, AMEDIAFORMAT_KEY_MAX_INPUT_SIZE, 4096);

			status = AMediaCodec_configure(encoderAudio, formatAudio, nullptr, nullptr, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
			AMediaFormat_delete(formatAudio);
			if(status != AMEDIA_OK)
			{
				RNWarning("Failed configuring audio encoder");
				StopInternal(false);
				return false;
			}

			status = AMediaCodec_start(encoderVideo);
			if(status != AMEDIA_OK)
			{
				RNWarning("Failed starting video encoder");
				StopInternal(false);
				return false;
			}

			status = AMediaCodec_start(encoderAudio);
			if(status != AMEDIA_OK)
			{
				RNWarning("Failed starting audio encoder");
				StopInternal(false);
				return false;
			}

			mediaSink = CreateMediaSink();
			if(!mediaSink->Start(configuration))
			{
				StopInternal(false);
				return false;
			}

			configuration.resolvePass->SetFramebuffer(configuration.restoreFramebuffer);

			WorkGroup *group = new WorkGroup();
			isDrainThreadRunning = true;
			group->Perform(WorkQueue::GetGlobalQueue(WorkQueue::Priority::Default), [this, lifetime = shared_from_this()] {
				AutoreleasePool pool;
				while(isRecording || isMediaSinkActiveForVideo || isMediaSinkActiveForAudio)
				{
					UpdateMediaSinkReadiness();
					DrainEncoderVideo(isWaitingToEndRecording);
					DrainEncoderAudio(isWaitingToEndRecording);

					if(ShouldFeedAudio())
					{
						FeedPendingAudio();
					}

					DrainEncoderAudio(isWaitingToEndRecording);
					if(hasReachedVideoEndOfStream && hasReachedAudioEndOfStream) break;
				}
				isDrainThreadRunning = false;
				StopInternal(true);
			});
			group->Release();

			return true;
		}

		void Stop()
		{
			if(!isRecording) return;

			if(isWaitingToEndRecording) return;
			if(hasStartedMediaTimeline)
			{
				int64 pendingAudioDurationToFlushUs = GetCapturePresentationTimeUs() - lastScheduledPresentationTimeUs;
				if(pendingAudioDurationToFlushUs > 0)
				{
					AddPendingAudioDuration(pendingAudioDurationToFlushUs);
					lastScheduledPresentationTimeUs += pendingAudioDurationToFlushUs;
				}
			}
			isWaitingToEndRecording = true;
			if(configuration.resolvePass) configuration.resolvePass->SetFramebuffer(configuration.restoreFramebuffer);
			configuration.resolvePass = nullptr;
			configuration.restoreFramebuffer = nullptr;
			retainedResolvePass = nullptr;
			retainedRestoreFramebuffer = nullptr;
			SignalVideoEndOfStreamAfterRenderThreadDrain();
		}

		void Update(float delta)
		{
			bool didScheduleRecordingFrame = false;
			int64 scheduledAudioDurationUs = 0;
			if(isRecording && !isWaitingToEndRecording)
			{
				videoFrameAccumulator += delta;
				if(videoFrameAccumulator >= configuration.videoFramePeriod)
				{
					videoFrameAccumulator -= configuration.videoFramePeriod;

					int64 scheduledPresentationTimeUs = 0;
					bool didStartMediaTimeline = !hasStartedMediaTimeline.exchange(true);
					if(didStartMediaTimeline)
					{
						recordingStartTimeUs = GetMonotonicTimeUs();
						lastScheduledPresentationTimeUs = 0;
						RNDebug("Recording recorder media timeline started with first scheduled video frame. Delta: " << delta << ", video frame period: " << configuration.videoFramePeriod);
					}
					else
					{
						scheduledPresentationTimeUs = GetCapturePresentationTimeUs();
						if(scheduledPresentationTimeUs < lastScheduledPresentationTimeUs)
						{
							RNWarning("Recording recorder scheduled PTS went backwards, previous: " << lastScheduledPresentationTimeUs << ", current: " << scheduledPresentationTimeUs);
							scheduledPresentationTimeUs = lastScheduledPresentationTimeUs;
						}

						scheduledAudioDurationUs = scheduledPresentationTimeUs - lastScheduledPresentationTimeUs;
						lastScheduledPresentationTimeUs = scheduledPresentationTimeUs;
					}

					scheduledVideoFrameCount.fetch_add(1);
					didScheduleRecordingFrame = true;
					configuration.resolvePass->SetFramebuffer(window->GetFramebuffer());
				}
				else
				{
					configuration.resolvePass->SetFramebuffer(configuration.restoreFramebuffer);
				}
			}

			if(didScheduleRecordingFrame)
			{
				AddPendingAudioDuration(scheduledAudioDurationUs);
			}
		}

		bool GetIsRecording() const
		{
			return isRecording || isDrainThreadRunning || isReleasingRecordingResources;
		}

		void StopInternal(bool notifyCompletion)
		{
			if(Thread::GetCurrentThread() != Thread::GetMainThread())
			{
				WorkGroup *group = new WorkGroup();
				group->Perform(WorkQueue::GetMainQueue(), [this, lifetime = shared_from_this(), notifyCompletion] {
					AutoreleasePool pool;
					StopInternal(notifyCompletion);
				});
				group->Release();
				return;
			}

			if(!isRecording) return;
			if(isWaitingToEndRecording)
			{
				if(!hasReachedVideoEndOfStream || !hasReachedAudioEndOfStream) return;
			}
			else if(isMediaSinkActiveForVideo || isMediaSinkActiveForAudio)
			{
				return;
			}

			isRecording = false;
			isWaitingToEndRecording = false;

			RNDebug("Releasing encoder objects");
			if(encoderVideo != nullptr)
			{
				RNDebug("Stop video encoder");
				AMediaCodec_stop(encoderVideo);
			}

			if(encoderAudio != nullptr)
			{
				RNDebug("Stop audio encoder");
				AMediaCodec_stop(encoderAudio);
			}

			if(configuration.resolvePass) configuration.resolvePass->SetFramebuffer(configuration.restoreFramebuffer);

			Window *recordingWindow = window;
			ANativeWindow *recordingSurfaceWindow = surfaceWindow;
			window = nullptr;
			surfaceWindow = nullptr;

			if(encoderVideo != nullptr)
			{
				RNDebug("Release video encoder");
				AMediaCodec_delete(encoderVideo);
				encoderVideo = nullptr;
			}

			if(encoderAudio != nullptr)
			{
				RNDebug("Release audio encoder");
				AMediaCodec_delete(encoderAudio);
				encoderAudio = nullptr;
			}

			if(mediaSink)
			{
				mediaSink->Stop();
				delete mediaSink;
				mediaSink = nullptr;
			}
			SetMediaSinkInactive();

			LogCaptureSummary();
			RNInfo("Stopped recording");

			std::function<void()> completionCallback = configuration.completionCallback;
			if(recordingWindow || recordingSurfaceWindow)
			{
				std::function<void()> releaseCompletion;
				if(notifyCompletion)
				{
					isReleasingRecordingResources = true;
					releaseCompletion = [this, lifetime = shared_from_this(), completionCallback] {
						isReleasingRecordingResources = false;
						if(completionCallback) completionCallback();
					};
				}
				ReleaseCaptureWindowAfterRenderThreadDrain(recordingWindow, recordingSurfaceWindow, releaseCompletion);
			}
			else if(notifyCompletion && completionCallback)
			{
				completionCallback();
			}
		}

		void ResetCaptureState()
		{
			audioSubmittedSamples = 0;
			audioInputBufferMissCount = 0;
			audioSkippedSamples = 0;
			audioSkipWarningCount = 0;
			scheduledVideoFrameCount = 0;
			writtenVideoFrameCount = 0;
			hasStartedMediaTimeline = false;
			hasQueuedAudioEndOfStream = false;
			hasReachedVideoEndOfStream = false;
			hasReachedAudioEndOfStream = false;
			firstVideoPresentationTimeUs = 0;
			firstAudioPresentationTimeUs = 0;
			lastVideoOutputPresentationTimeUs = 0;
			lastAudioOutputPresentationTimeUs = 0;
			recordingStartTimeUs = 0;
			lastScheduledPresentationTimeUs = 0;
			hasFirstVideoPresentationTime = false;
			hasFirstAudioPresentationTime = false;
			hasLastVideoOutputPresentationTime = false;
			hasLastAudioOutputPresentationTime = false;
			videoMediaSinkNotReadyCount = 0;
			audioMediaSinkNotReadyCount = 0;
			SetMediaSinkInactive();
			audioPendingDurationUs = 0;
			audioPendingSamples = 0;
		}

		bool ShouldFeedAudio()
		{
			bool canStartOrFinishAudio = hasStartedMediaTimeline || isWaitingToEndRecording;
			return canStartOrFinishAudio && !hasQueuedAudioEndOfStream;
		}

		void FeedPendingAudio()
		{
			int64 pendingAudioDurationToFeedUs = TakePendingAudioDuration();
			if((pendingAudioDurationToFeedUs > 0 || audioPendingSamples > 0 || isWaitingToEndRecording) && !hasQueuedAudioEndOfStream)
			{
				if(pendingAudioDurationToFeedUs > 0)
				{
					audioPendingSamples += audioSource->GetFrameCount(static_cast<float>(static_cast<double>(pendingAudioDurationToFeedUs) / 1000000.0));
				}
				FeedEncoderAudio();
			}
		}

		void AddPendingAudioDuration(int64 durationUs)
		{
			if(durationUs <= 0) return;

			audioPendingDurationLock.Lock();
			audioPendingDurationUs += durationUs;
			audioPendingDurationLock.Unlock();
		}

		int64 TakePendingAudioDuration()
		{
			audioPendingDurationLock.Lock();
			int64 durationUs = audioPendingDurationUs;
			audioPendingDurationUs = 0;
			audioPendingDurationLock.Unlock();

			return durationUs;
		}

		int64 GetMonotonicTimeUs()
		{
	#if RN_PLATFORM_POSIX
			struct timespec time;
			clock_gettime(CLOCK_MONOTONIC, &time);
			return static_cast<int64>(time.tv_sec * 1000000) + static_cast<int64>(time.tv_nsec / 1000);
	#else
			return 0;
	#endif
		}

		int64 GetCapturePresentationTimeUs()
		{
			return GetMonotonicTimeUs() - recordingStartTimeUs;
		}

		uint64 GetAudioPresentationTime()
		{
			return (GetAudioTimelineSampleCount() * 1000000) / AudioSampleRate;
		}

		uint64 GetAudioDurationForSamples(uint64 samples)
		{
			return (samples * 1000000) / AudioSampleRate;
		}

		uint64 GetAudioTimelineSampleCount()
		{
			return audioSubmittedSamples + audioSkippedSamples;
		}

		void LogOutputPresentationTime(const char *track, int64 presentationTimeUs, bool &hasLastPresentationTime, int64 &lastPresentationTimeUs, int64 largeGapThresholdUs)
		{
			if(hasLastPresentationTime)
			{
				int64 delta = presentationTimeUs - lastPresentationTimeUs;
				if(delta < 0)
				{
					RNWarning(track << " output PTS went backwards, previous: " << lastPresentationTimeUs << ", current: " << presentationTimeUs << ", delta: " << delta);
				}
				else if(delta > largeGapThresholdUs)
				{
					RNWarning(track << " output PTS large gap, previous: " << lastPresentationTimeUs << ", current: " << presentationTimeUs << ", delta: " << delta);
				}
			}

			lastPresentationTimeUs = presentationTimeUs;
			hasLastPresentationTime = true;
		}

		void LogCaptureSummary()
		{
			uint64 scheduledVideoFrames = scheduledVideoFrameCount.load();
			int64 videoDurationUs = lastVideoOutputPresentationTimeUs;
			int64 audioDurationUs = static_cast<int64>(GetAudioDurationForSamples(GetAudioTimelineSampleCount()));
			int64 durationDriftUs = audioDurationUs - videoDurationUs;
			if(durationDriftUs > 50000 || durationDriftUs < -50000)
			{
				RNWarning("Recording recorder audio/video duration drift, audio: " << audioDurationUs << "us, video: " << videoDurationUs << "us, drift: " << durationDriftUs << "us");
			}
			if(scheduledVideoFrames > writtenVideoFrameCount + 1 || writtenVideoFrameCount > scheduledVideoFrames + 1)
			{
				RNWarning("Recording recorder video frame count mismatch, scheduled: " << scheduledVideoFrames << ", written: " << writtenVideoFrameCount);
			}
			RNInfo("Recording recorder summary, scheduled video frames: " << scheduledVideoFrames << ", written video frames: " << writtenVideoFrameCount << ", video duration: " << videoDurationUs << "us, audio submitted: " << audioSubmittedSamples << " samples (" << GetAudioDurationForSamples(audioSubmittedSamples) << "us), audio skipped: " << audioSkippedSamples << " samples (" << GetAudioDurationForSamples(audioSkippedSamples) << "us), audio timeline duration: " << audioDurationUs << "us, audio/video duration drift: " << durationDriftUs << "us, audio input buffer misses: " << audioInputBufferMissCount);
		}

		void ReleaseCaptureWindowAfterRenderThreadDrain(Window *recordingWindow, ANativeWindow *recordingSurfaceWindow, std::function<void()> completion)
		{
			if(!recordingWindow && !recordingSurfaceWindow)
			{
				if(completion) completion();
				return;
			}

			Renderer::GetActiveRenderer()->ScheduleRenderThreadWork([recordingWindow, recordingSurfaceWindow, completion] {
				WorkGroup *group = new WorkGroup();
				group->Perform(WorkQueue::GetMainQueue(), [recordingWindow, recordingSurfaceWindow, completion] {
					AutoreleasePool pool;
					bool waitsForSwapChainRelease = false;
					if(recordingWindow && completion)
					{
						waitsForSwapChainRelease = recordingWindow->SetSwapChainReleaseCallback([completion] {
							PerformCompletionOnMainQueue(completion);
						});
					}
					if(recordingWindow) recordingWindow->Release();
					if(recordingSurfaceWindow) ANativeWindow_release(recordingSurfaceWindow);
					if(completion && !waitsForSwapChainRelease) completion();
				});
				group->Release();
			});
		}

		static void PerformCompletionOnMainQueue(std::function<void()> completion)
		{
			if(!completion) return;

			WorkGroup *group = new WorkGroup();
			group->Perform(WorkQueue::GetMainQueue(), [completion] {
				AutoreleasePool pool;
				completion();
			});
			group->Release();
		}

		void SignalVideoEndOfStreamAfterRenderThreadDrain()
		{
			Renderer::GetActiveRenderer()->ScheduleRenderThreadWork([this, lifetime = shared_from_this()] {
				WorkGroup *group = new WorkGroup();
				group->Perform(WorkQueue::GetMainQueue(), [this, lifetime = shared_from_this()] {
					AutoreleasePool pool;
					if(!isRecording || !encoderVideo) return;
					AMediaCodec_signalEndOfInputStream(encoderVideo);
				});
				group->Release();
			});
		}

		void RecordSkippedAudioSamples(size_t skippedSamples, const char *reason)
		{
			if(skippedSamples == 0) return;

			audioSkippedSamples += skippedSamples;
			audioSkipWarningCount += 1;
			if(audioSkipWarningCount <= 10 || audioSkipWarningCount % 60 == 0)
			{
				RNWarning("Recording recorder skipped " << skippedSamples << " audio samples (" << GetAudioDurationForSamples(skippedSamples) << "us) " << reason << ". Total skipped: " << audioSkippedSamples << " samples (" << GetAudioDurationForSamples(audioSkippedSamples) << "us), next audio PTS: " << GetAudioPresentationTime());
			}
		}

		void SkipAudioSamples(size_t skippedSamples, const char *reason)
		{
			if(skippedSamples == 0) return;

			audioSource->ReadFrames(skippedSamples, nullptr);
			RecordSkippedAudioSamples(skippedSamples, reason);
		}

		void FeedEncoderAudio()
		{
			if(!isRecording) return;
			if(hasQueuedAudioEndOfStream) return;

			if(configuration.streamURL && audioPendingSamples > MaxStreamPendingAudioSamples)
			{
				size_t skippedSamples = audioPendingSamples - MaxStreamPendingAudioSamples;
				SkipAudioSamples(skippedSamples, "to cap live stream audio backlog");
				audioPendingSamples = MaxStreamPendingAudioSamples;
			}

			while(audioPendingSamples > 0 || isWaitingToEndRecording)
			{
				size_t bufferSize;
				ssize_t bufferIndex = -1;
				bufferIndex = AMediaCodec_dequeueInputBuffer(encoderAudio, AudioInputBufferTimeoutUs);
				if(bufferIndex < 0)
				{
					audioInputBufferMissCount++;
					if(audioPendingSamples > 0)
					{
						if(audioInputBufferMissCount <= 10 || audioInputBufferMissCount % 60 == 0)
						{
							RNWarning("Audio encoder did not return an input buffer, keeping " << audioPendingSamples << " samples (" << GetAudioDurationForSamples(audioPendingSamples) << "us) pending. Total misses: " << audioInputBufferMissCount << ", next audio PTS: " << GetAudioPresentationTime());
						}
					}
					else
					{
						RNDebug("Audio encoder did not return an input buffer while ending stream. Total misses: " << audioInputBufferMissCount << ", next audio PTS: " << GetAudioPresentationTime());
					}
					break;
				}
				uint8_t *rawData = AMediaCodec_getInputBuffer(encoderAudio, bufferIndex, &bufferSize);
				bufferSize /= AudioFrameBytes; //Everything below treats this as stereo frame count.

				size_t samplesToRead = audioPendingSamples;
				if(samplesToRead > bufferSize) samplesToRead = bufferSize;

				if(samplesToRead > 0) audioSource->ReadFrames(samplesToRead, rawData);

				uint64 presentationTime = GetAudioPresentationTime();
				bool isEndOfStreamBuffer = isWaitingToEndRecording && samplesToRead == 0;
				media_status_t status = AMediaCodec_queueInputBuffer(encoderAudio, bufferIndex, 0, samplesToRead * AudioFrameBytes, presentationTime, isEndOfStreamBuffer ? AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM : 0);
				if(status != AMEDIA_OK)
				{
					RNWarning("Audio encoder queueInputBuffer failed: " << status << ", samples: " << samplesToRead << ", PTS: " << presentationTime << ", EOS: " << isEndOfStreamBuffer);
					audioPendingSamples -= samplesToRead;
					RecordSkippedAudioSamples(samplesToRead, "after audio encoder queueInputBuffer failed");
				}
				else
				{
					audioPendingSamples -= samplesToRead;
					audioSubmittedSamples += samplesToRead;
					if(isEndOfStreamBuffer)
					{
						hasQueuedAudioEndOfStream = true;
						RNDebug("Queued audio encoder EOS at PTS: " << presentationTime << ", submitted samples: " << audioSubmittedSamples << " (" << GetAudioDurationForSamples(audioSubmittedSamples) << "us)");
						break;
					}
				}
			}
		}

		void RequestKeyframe()
		{
			if(android_get_device_api_level() < 31) return;

			AMediaFormat *requestKeyframeFormat = AMediaFormat_new();
			AMediaFormat_setInt32(requestKeyframeFormat, "request-sync", 1); //AMEDIACODEC_KEY_REQUEST_SYNC_FRAME should be used here, but fails when compiling with minSDK < 31
			AMediaCodec_setParameters(encoderVideo, requestKeyframeFormat);
			AMediaFormat_delete(requestKeyframeFormat);
		}

		void UpdateMediaSinkReadiness()
		{
			bool wasReady = isMediaSinkReady;
			bool isReady = mediaSink && mediaSink->GetIsReady();
			isMediaSinkReady = isReady;

			if(!wasReady && isReady)
			{
				RequestKeyframe();
			}
		}

		void SetMediaSinkInactive()
		{
			isMediaSinkActiveForVideo = false;
			isMediaSinkActiveForAudio = false;
			isMediaSinkReady = false;
		}

		void LogMediaSinkNotReady(bool isVideo)
		{
			uint64 &counter = isVideo ? videoMediaSinkNotReadyCount : audioMediaSinkNotReadyCount;
			counter += 1;

			if(counter == 1)
			{
				RNDebug((isVideo ? "Video" : "Audio") << " encoder output is waiting for the media sink to become ready");
			}
			else if(counter % 300 == 0)
			{
				RNWarning((isVideo ? "Video" : "Audio") << " encoder output is still waiting for the media sink, skipped samples: " << counter);
			}
		}

		static void CopyAndroidFormatBuffer(AMediaFormat *format, const char *name, std::vector<uint8> &buffer)
		{
			buffer.clear();

			void *data = nullptr;
			size_t size = 0;
			if(AMediaFormat_getBuffer(format, name, &data, &size) && data && size > 0)
			{
				const uint8 *bytes = static_cast<const uint8 *>(data);
				buffer.assign(bytes, bytes + size);
			}
		}

		CaptureEncodedVideoFormat GetEncodedVideoFormat(AMediaFormat *format, const std::vector<uint8> &csd0, const std::vector<uint8> &csd1) const
		{
			CaptureEncodedVideoFormat encodedFormat;
			encodedFormat.width = configuration.resolutionX;
			encodedFormat.height = configuration.resolutionY;

			int32_t width = 0;
			int32_t height = 0;
			if(AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &width)) encodedFormat.width = width;
			if(AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &height)) encodedFormat.height = height;

			encodedFormat.codecConfiguration = csd0.empty() ? nullptr : csd0.data();
			encodedFormat.codecConfigurationSize = csd0.size();
			encodedFormat.codecConfiguration2 = csd1.empty() ? nullptr : csd1.data();
			encodedFormat.codecConfiguration2Size = csd1.size();
			return encodedFormat;
		}

		CaptureEncodedAudioFormat GetEncodedAudioFormat(AMediaFormat *format, const std::vector<uint8> &csd0) const
		{
			CaptureEncodedAudioFormat encodedFormat;
			encodedFormat.sampleRate = AudioSampleRate;
			encodedFormat.channelCount = AudioChannelCount;

			int32_t sampleRate = 0;
			int32_t channelCount = 0;
			if(AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &sampleRate)) encodedFormat.sampleRate = sampleRate;
			if(AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &channelCount)) encodedFormat.channelCount = channelCount;

			encodedFormat.audioSpecificConfig = csd0.empty() ? nullptr : csd0.data();
			encodedFormat.audioSpecificConfigSize = csd0.size();
			return encodedFormat;
		}

		static bool IsAndroidSampleKeyFrame(const AMediaCodecBufferInfo &bufferInfo)
		{
			return (bufferInfo.flags & AndroidCodecBufferFlagKeyFrame) != 0;
		}

		static CaptureEncodedSample GetEncodedSample(const uint8 *data, const AMediaCodecBufferInfo &bufferInfo)
		{
			CaptureEncodedSample sample;
			sample.data = data;
			sample.offset = static_cast<size_t>(bufferInfo.offset);
			sample.size = static_cast<size_t>(bufferInfo.size);
			sample.presentationTimeUs = bufferInfo.presentationTimeUs;
			sample.isKeyFrame = IsAndroidSampleKeyFrame(bufferInfo);
			sample.isCodecConfig = (bufferInfo.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG) != 0;
			sample.isEndOfStream = (bufferInfo.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0;
			return sample;
		}

		void HandleMediaSinkFormat(bool isVideo, AMediaFormat *format)
		{
			const char *track = isVideo ? "Video" : "Audio";
			if(format == nullptr)
			{
				RNWarning("Unable to set new " << track << " format.");
				SetMediaSinkInactive();
				return;
			}

			RNDebug(track << " encoder output format changed: " << AMediaFormat_toString(format));

			std::vector<uint8> csd0;
			CopyAndroidFormatBuffer(format, "csd-0", csd0);

			bool didReceiveFormat = false;
			if(isVideo)
			{
				std::vector<uint8> csd1;
				CopyAndroidFormatBuffer(format, "csd-1", csd1);
				CaptureEncodedVideoFormat encodedFormat = GetEncodedVideoFormat(format, csd0, csd1);
				didReceiveFormat = mediaSink && mediaSink->DidReceiveVideoFormat(encodedFormat);
			}
			else
			{
				CaptureEncodedAudioFormat encodedFormat = GetEncodedAudioFormat(format, csd0);
				didReceiveFormat = mediaSink && mediaSink->DidReceiveAudioFormat(encodedFormat);
			}
			if(!didReceiveFormat)
			{
				SetMediaSinkInactive();
				return;
			}

			if(isVideo)
			{
				isMediaSinkActiveForVideo = true;
			}
			else
			{
				isMediaSinkActiveForAudio = true;
			}
			UpdateMediaSinkReadiness();
		}

		void DrainEncoderVideo(bool endOfStream)
		{
			if(!isRecording) return;
			if(hasReachedVideoEndOfStream) return;

			while(true)
			{
				AMediaCodecBufferInfo bufferInfo;
				ssize_t encoderStatus = AMediaCodec_dequeueOutputBuffer(encoderVideo, &bufferInfo, 1000);

				if(encoderStatus == AMEDIACODEC_INFO_TRY_AGAIN_LATER)
				{
					break;
				}
				else if(encoderStatus == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED)
				{
					// not expected for an encoder
				}
				else if(encoderStatus == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
				{
					if(isMediaSinkActiveForVideo)
					{
						RNWarning("ERROR: format changed twice");
					}
					AMediaFormat *newFormat = AMediaCodec_getOutputFormat(encoderVideo);
					HandleMediaSinkFormat(true, newFormat);
					if(newFormat) AMediaFormat_delete(newFormat);
				}
				else if(encoderStatus < 0)
				{
					RNWarning("Unexpected result from encoder.dequeueOutputBuffer: " << encoderStatus);
				}
				else
				{
					size_t out_size;
					uint8_t *encodedData = AMediaCodec_getOutputBuffer(encoderVideo, encoderStatus, &out_size);

					if(out_size <= 0)
					{
						RNWarning("Encoded data of size 0.");
					}

					if(encodedData == nullptr)
					{
						RNWarning("encoderOutputBuffer " << encoderStatus << " was null");
					}

					if((bufferInfo.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG) != 0)
					{
						RNDebug("Ignoring BUFFER_FLAG_CODEC_CONFIG");
						bufferInfo.size = 0;
					}

					if(bufferInfo.size != 0)
					{
						if(isMediaSinkReady)
						{
							if(!hasFirstVideoPresentationTime)
							{
								firstVideoPresentationTimeUs = bufferInfo.presentationTimeUs;
								hasFirstVideoPresentationTime = true;
								RNDebug("First video encoder PTS: " << firstVideoPresentationTimeUs);
							}
							int64 rawPresentationTimeUs = bufferInfo.presentationTimeUs - firstVideoPresentationTimeUs;
							bufferInfo.presentationTimeUs = rawPresentationTimeUs;
							LogOutputPresentationTime("Video", bufferInfo.presentationTimeUs, hasLastVideoOutputPresentationTime, lastVideoOutputPresentationTimeUs, static_cast<int64>(configuration.videoFramePeriod * 1000000.0 * 3.0));

							if(mediaSink) mediaSink->WriteVideoSample(GetEncodedSample(encodedData, bufferInfo));
							writtenVideoFrameCount += 1;
						}
						else
						{
							LogMediaSinkNotReady(true);
						}
					}
					else
					{
						RNWarning("mBufferInfo empty " << bufferInfo.size);
					}

					AMediaCodec_releaseOutputBuffer(encoderVideo, encoderStatus, false);
					if((bufferInfo.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0)
					{
						if(!endOfStream)
						{
							RNWarning("Reached end of stream unexpectedly");
						}
						else
						{
							RNDebug("End of stream reached");
						}

						isMediaSinkActiveForVideo = false;
						hasReachedVideoEndOfStream = true;
						break;
					}
				}
			}
		}

		void DrainEncoderAudio(bool endOfStream)
		{
			if(!isRecording) return;
			if(hasReachedAudioEndOfStream) return;

			while(true)
			{
				AMediaCodecBufferInfo bufferInfo;
				ssize_t encoderStatus = AMediaCodec_dequeueOutputBuffer(encoderAudio, &bufferInfo, 1000);

				if(encoderStatus == AMEDIACODEC_INFO_TRY_AGAIN_LATER)
				{
					break;
				}
				else if(encoderStatus == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED)
				{
					// not expected for an encoder
				}
				else if(encoderStatus == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
				{
					if(isMediaSinkActiveForAudio)
					{
						RNWarning("ERROR: format changed twice");
					}
					AMediaFormat *newFormat = AMediaCodec_getOutputFormat(encoderAudio);
					HandleMediaSinkFormat(false, newFormat);
					if(newFormat) AMediaFormat_delete(newFormat);
				}
				else if(encoderStatus < 0)
				{
					RNWarning("Unexpected result from encoder.dequeueOutputBuffer: " << encoderStatus);
				}
				else
				{
					size_t out_size;
					uint8_t *encodedData = AMediaCodec_getOutputBuffer(encoderAudio, encoderStatus, &out_size);

					if(out_size <= 0)
					{
						RNWarning("Encoded data of size 0.");
					}

					if(encodedData == nullptr)
					{
						RNWarning("encoderOutputBuffer " << encoderStatus << " was null");
					}

					if((bufferInfo.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG) != 0)
					{
						RNDebug("Ignoring BUFFER_FLAG_CODEC_CONFIG");
						bufferInfo.size = 0;
					}

					if(bufferInfo.size != 0)
					{
						if(isMediaSinkReady)
						{
							if(!hasFirstAudioPresentationTime)
							{
								firstAudioPresentationTimeUs = bufferInfo.presentationTimeUs;
								hasFirstAudioPresentationTime = true;
								RNDebug("First audio encoder PTS: " << firstAudioPresentationTimeUs);
							}
							bufferInfo.presentationTimeUs -= firstAudioPresentationTimeUs;
							LogOutputPresentationTime("Audio", bufferInfo.presentationTimeUs, hasLastAudioOutputPresentationTime, lastAudioOutputPresentationTimeUs, 100000);
							if(mediaSink) mediaSink->WriteAudioSample(GetEncodedSample(encodedData, bufferInfo));
						}
						else
						{
							LogMediaSinkNotReady(false);
						}
					}
					else
					{
						RNWarning("mBufferInfo empty " << bufferInfo.size);
					}

					AMediaCodec_releaseOutputBuffer(encoderAudio, encoderStatus, false);
					if((bufferInfo.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0)
					{
						if(!endOfStream)
						{
							RNWarning("Reached end of stream unexpectedly");
						}
						else
						{
							RNDebug("End of stream reached");
						}

						isMediaSinkActiveForAudio = false;
						hasReachedAudioEndOfStream = true;

						break;
					}
				}
			}
		}

		StrongRef<PostProcessingAPIStage> retainedResolvePass;
		StrongRef<Framebuffer> retainedRestoreFramebuffer;
		StrongRef<const String> retainedFilename;
		StrongRef<const String> retainedStreamURL;
		Configuration configuration;
		std::atomic<bool> isRecording;
		std::atomic<bool> isWaitingToEndRecording;

		std::atomic<bool> isReleasingRecordingResources;

		std::atomic<bool> isDrainThreadRunning;

		Window *window;
		ANativeWindow *surfaceWindow;

		AMediaCodec *encoderVideo;
		AMediaCodec *encoderAudio;

		CaptureEncodedSink *mediaSink;

		uint64 audioSubmittedSamples;
		uint64 audioInputBufferMissCount;
		uint64 audioSkippedSamples;
		uint64 audioSkipWarningCount;
		std::atomic<uint64> scheduledVideoFrameCount;
		uint64 writtenVideoFrameCount;
		std::atomic<bool> hasStartedMediaTimeline;
		std::atomic<bool> hasQueuedAudioEndOfStream;
		std::atomic<bool> hasReachedVideoEndOfStream;
		std::atomic<bool> hasReachedAudioEndOfStream;
		int64 firstVideoPresentationTimeUs;
		int64 firstAudioPresentationTimeUs;
		int64 lastVideoOutputPresentationTimeUs;
		int64 lastAudioOutputPresentationTimeUs;
		int64 recordingStartTimeUs;
		int64 lastScheduledPresentationTimeUs;
		bool hasFirstVideoPresentationTime;
		bool hasFirstAudioPresentationTime;
		bool hasLastVideoOutputPresentationTime;
		bool hasLastAudioOutputPresentationTime;
		uint64 videoMediaSinkNotReadyCount;
		uint64 audioMediaSinkNotReadyCount;
		std::atomic<bool> isMediaSinkActiveForVideo;
		std::atomic<bool> isMediaSinkActiveForAudio;
		std::atomic<bool> isMediaSinkReady;

		Lockable audioPendingDurationLock;
		int64 audioPendingDurationUs;
		size_t audioPendingSamples;

		double videoFrameAccumulator;
		std::shared_ptr<CaptureAudioSource> audioSource;
		std::shared_ptr<std::atomic<bool>> callbackEnabled = std::make_shared<std::atomic<bool>>(true);
	};

	CaptureAndroidSession::CaptureAndroidSession() :
		_state(std::make_shared<State>())
	{
		_state->audioSource = GetAudioSource();
	}

	CaptureAndroidSession::~CaptureAndroidSession()
	{
		*_state->callbackEnabled = false;
		_state->Stop();
	}

	bool CaptureAndroidSession::Start(const Configuration &configuration)
	{
		if(!configuration.IsValid() || (configuration.streamURL && !RN_CAPTURE_ENABLE_RTMP) || _state->GetIsRecording()) return false;
		if(static_cast<bool>(configuration.openFileCallback) != static_cast<bool>(configuration.finishFileCallback)) return false;
		GetAudioSource()->Configure(configuration);
		Configuration safeConfiguration = configuration;
		auto enabled = _state->callbackEnabled;
		safeConfiguration.completionCallback = [enabled, callback = configuration.completionCallback] {
			if(*enabled && callback) callback();
		};
		safeConfiguration.errorCallback = [enabled, callback = configuration.errorCallback](const String *error) {
			if(*enabled && callback) callback(error);
		};
		return _state->Start(safeConfiguration);
	}

	void CaptureAndroidSession::Stop()
	{
		_state->Stop();
	}

	void CaptureAndroidSession::Update(float delta)
	{
		_state->Update(delta);
	}

	bool CaptureAndroidSession::GetIsRecording() const
	{
		return _state->isRecording && !_state->isWaitingToEndRecording;
	}

	bool CaptureAndroidSession::GetIsFinishing() const
	{
		return _state->isWaitingToEndRecording || _state->isReleasingRecordingResources || (!_state->isRecording && _state->isDrainThreadRunning);
	}

	Framebuffer *CaptureAndroidSession::GetFramebuffer() const
	{
		return _state->window ? _state->window->GetFramebuffer() : nullptr;
	}

	CaptureSession *CaptureSessionWithAndroidPlatform()
	{
		return new CaptureAndroidSession();
	}
} // namespace RN

#endif
