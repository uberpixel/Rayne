//
//  RNCaptureWindowsSession.cpp
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCaptureSession.h"

#if RN_PLATFORM_WINDOWS
	#include "RNCaptureRTMPWriter.h"
	#include "RNCaptureWindowsFileWriter.h"
	#include "RNCaptureWindowsInternals.h"
	#include "RNCaptureWindowsSwapChain.h"

	#include <RNVulkanRenderer.h>

namespace RN
{
	struct CaptureWindowsState : public std::enable_shared_from_this<CaptureWindowsState>
	{
		using Configuration = CaptureSession::Configuration;

		CaptureWindowsState() :
			isRecording(false),
			isWaitingToEndRecording(false),
			isReleasingRecordingResources(false),
			recordingSwapChain(nullptr),
			nextVideoPresentationTimeUs(0),
			scheduledVideoFrameCount(0),
			recordingSession(nullptr),
			videoFrameAccumulator(0.0),
			audioSubmittedSamples(0),
			audioSkippedSamples(0),
			audioSkipWarningCount(0),
			audioPendingDurationUs(0)
		{}

		~CaptureWindowsState() = default;

		bool Start(const Configuration &newConfiguration)
		{
			if(GetIsRecording()) return false;
			return StartRecording(newConfiguration);
		}

		void Stop()
		{
			if(!isRecording || isWaitingToEndRecording) return;

			isWaitingToEndRecording = true;
			if(configuration.resolvePass) configuration.resolvePass->SetFramebuffer(configuration.restoreFramebuffer);
			configuration.resolvePass = nullptr;
			configuration.restoreFramebuffer = nullptr;
			retainedResolvePass = nullptr;
			retainedRestoreFramebuffer = nullptr;
			StopInternal(true);
		}

		void Update(float delta)
		{
			UpdateRecording(delta);
		}

		bool GetIsRecording() const
		{
			return isRecording || isReleasingRecordingResources;
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

			FlushAudio();

			isRecording = false;
			isWaitingToEndRecording = false;

			if(configuration.resolvePass) configuration.resolvePass->SetFramebuffer(configuration.restoreFramebuffer);

			CaptureWindowsSwapChain *swapChain = recordingSwapChain;
			std::shared_ptr<CaptureWindowsEncodingSession> session = recordingSession;
			recordingSwapChain = nullptr;
			recordingSession = nullptr;

			size_t canceledQueuedFrameCount = swapChain ? swapChain->ClearQueuedFrames() : 0;
			if(session && canceledQueuedFrameCount > 0)
			{
				session->CancelPendingFrames(canceledQueuedFrameCount);
			}

			RNInfo("Stopping Windows recording after scheduling " << scheduledVideoFrameCount.load() << " video frames, writing " << (session ? session->GetWrittenVideoFrameCount() : 0) << " frames, pending " << (session ? session->GetPendingVideoFrameCount() : 0) << " frames, writing " << (session ? session->GetWrittenAudioSampleCount() : 0) << " audio samples, skipped " << audioSkippedSamples << " audio samples");

			std::function<void()> completionCallback = configuration.completionCallback;
			if(session || swapChain)
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
				if(session)
				{
					session->FinishWhenReady(swapChain, releaseCompletion);
				}
				else
				{
					ReleaseCaptureWindowsSwapChainAfterRenderThreadDrain(swapChain, releaseCompletion);
				}
			}
			else if(notifyCompletion && completionCallback)
			{
				completionCallback();
			}
		}

		std::unique_ptr<CaptureEncodedSink> CreateMediaSink() const
		{
	#if RN_CAPTURE_ENABLE_RTMP
			if(configuration.streamURL) return std::make_unique<CaptureRTMPWriter>();
	#endif
			return std::make_unique<CaptureWindowsFileWriter>();
		}

		bool StartRecording(const Configuration &newConfiguration)
		{
			if(newConfiguration.streamURL && !RN_CAPTURE_ENABLE_RTMP) return false;
			configuration = newConfiguration;
			retainedResolvePass = configuration.resolvePass;
			retainedRestoreFramebuffer = configuration.restoreFramebuffer;
			retainedFilename = configuration.filename;
			retainedStreamURL = configuration.streamURL;
			isRecording = true;
			isWaitingToEndRecording = false;
			isReleasingRecordingResources = false;
			recordingSession = nullptr;
			scheduledVideoFrameCount = 0;
			nextVideoPresentationTimeUs = 0;
			videoFrameAccumulator = configuration.videoFramePeriod;
			ResetAudioState();

			Renderer *activeRenderer = Renderer::GetActiveRenderer();
			VulkanRenderer *vulkanRenderer = activeRenderer ? activeRenderer->Downcast<VulkanRenderer>() : nullptr;
			if(!vulkanRenderer)
			{
				RNWarning("Recording recording on Windows requires the Vulkan renderer");
				StopInternal(false);
				return false;
			}

			Window::SwapChainDescriptor swapChainDescriptor;
			swapChainDescriptor.colorFormat = Texture::Format::BGRA_8_SRGB;
			swapChainDescriptor.depthStencilFormat = Texture::Format::Invalid;
			swapChainDescriptor.usageHint = Texture::UsageHint::RenderTarget;
			swapChainDescriptor.bufferCount = 3;
			swapChainDescriptor.layerCount = 1;
			swapChainDescriptor.vsync = false;

			recordingSwapChain = new CaptureWindowsSwapChain(vulkanRenderer, Vector2(configuration.resolutionX, configuration.resolutionY), swapChainDescriptor);
			if(!recordingSwapChain->GetIsValid())
			{
				RNWarning("Failed to create recording swapchain");
				StopInternal(false);
				return false;
			}

			std::unique_ptr<CaptureEncodedSink> mediaSink = CreateMediaSink();
			recordingSession = CaptureWindowsEncodingSession::WithMediaSink(configuration, recordingSwapChain->GetD3DDevice(), std::move(mediaSink));
			if(!recordingSession)
			{
				RNWarning("Failed to start Windows media sink");
				StopInternal(false);
				return false;
			}

			std::shared_ptr<CaptureWindowsEncodingSession> session = recordingSession;
			recordingSwapChain->SetFrameReadyCallback([session](ID3D11Texture2D *texture, int64 presentationTimeUs, int64 durationUs) {
				session->WriteFrame(texture, presentationTimeUs, durationUs);
			});

			configuration.resolvePass->SetFramebuffer(configuration.restoreFramebuffer);

			return true;
		}

		void UpdateRecording(float delta)
		{
			if(!isRecording || isWaitingToEndRecording) return;

			videoFrameAccumulator += delta;
			if(videoFrameAccumulator < configuration.videoFramePeriod)
			{
				configuration.resolvePass->SetFramebuffer(configuration.restoreFramebuffer);
				return;
			}

			videoFrameAccumulator -= configuration.videoFramePeriod;
			if(!recordingSwapChain || !recordingSession)
			{
				configuration.resolvePass->SetFramebuffer(configuration.restoreFramebuffer);
				return;
			}

			const int64 frameDurationUs = GetVideoFrameDurationUs();
			const int64 presentationTimeUs = nextVideoPresentationTimeUs;
			nextVideoPresentationTimeUs += frameDurationUs;

			recordingSession->AddPendingFrame();
			recordingSwapChain->QueueFrame(presentationTimeUs, frameDurationUs);
			scheduledVideoFrameCount.fetch_add(1);
			WriteAudioDuration(frameDurationUs);
			configuration.resolvePass->SetFramebuffer(recordingSwapChain->GetFramebuffer());
		}

		int64 GetVideoFrameDurationUs() const
		{
			double framePeriod = configuration.videoFramePeriod > 0.0 ? configuration.videoFramePeriod : (1.0 / 30.0);
			return static_cast<int64>(std::max(1.0, std::round(framePeriod * 1000000.0)));
		}

		void ResetAudioState()
		{
			audioSubmittedSamples = 0;
			audioSkippedSamples = 0;
			audioSkipWarningCount = 0;
			audioPendingDurationUs = 0;
			audioBuffer.clear();
		}

		int64 GetAudioPresentationTimeUs() const
		{
			return CaptureWindowsInternals::GetAudioDurationForSamples(audioSubmittedSamples + audioSkippedSamples);
		}

		void RecordSkippedAudioSamples(size_t skippedSamples, const char *reason)
		{
			if(skippedSamples == 0) return;

			audioSkippedSamples += skippedSamples;
			audioSkipWarningCount += 1;
			if(audioSkipWarningCount <= 10 || audioSkipWarningCount % 60 == 0)
			{
				RNWarning("Recording Windows recorder skipped " << skippedSamples << " audio samples (" << CaptureWindowsInternals::GetAudioDurationForSamples(skippedSamples) << "us) " << reason << ". Total skipped: " << audioSkippedSamples << " samples (" << CaptureWindowsInternals::GetAudioDurationForSamples(audioSkippedSamples) << "us), next audio PTS: " << GetAudioPresentationTimeUs());
			}
		}

		void FlushAudio()
		{
			if(audioPendingDurationUs <= 0) return;

			std::shared_ptr<CaptureWindowsEncodingSession> session = recordingSession;
			if(!session)
			{
				audioPendingDurationUs = 0;
				return;
			}

			const int64 pendingDurationUs = audioPendingDurationUs;
			audioPendingDurationUs = 0;

			const size_t sampleCount = audioSource->GetFrameCount(static_cast<float>(static_cast<double>(pendingDurationUs) / 1000000.0));
			if(sampleCount == 0) return;

			audioBuffer.resize(sampleCount * CaptureWindowsInternals::AudioFrameBytes);
			audioSource->ReadFrames(sampleCount, audioBuffer.data());

			const int64 presentationTimeUs = GetAudioPresentationTimeUs();
			if(session->WriteAudioSamples(audioBuffer.data(), sampleCount, presentationTimeUs))
			{
				audioSubmittedSamples += sampleCount;
			}
			else
			{
				RecordSkippedAudioSamples(sampleCount, "after Media Foundation audio sample write failed");
			}
		}

		void WriteAudioDuration(int64 durationUs)
		{
			if(durationUs <= 0) return;

			audioPendingDurationUs += durationUs;
			FlushAudio();
		}

		StrongRef<PostProcessingAPIStage> retainedResolvePass;
		StrongRef<Framebuffer> retainedRestoreFramebuffer;
		StrongRef<const String> retainedFilename;
		StrongRef<const String> retainedStreamURL;
		Configuration configuration;
		std::atomic<bool> isRecording;
		std::atomic<bool> isWaitingToEndRecording;
		std::atomic<bool> isReleasingRecordingResources;
		CaptureWindowsSwapChain *recordingSwapChain;
		int64 nextVideoPresentationTimeUs;
		std::atomic<uint64> scheduledVideoFrameCount;
		std::shared_ptr<CaptureWindowsEncodingSession> recordingSession;
		double videoFrameAccumulator;
		uint64 audioSubmittedSamples;
		uint64 audioSkippedSamples;
		uint64 audioSkipWarningCount;
		int64 audioPendingDurationUs;
		std::vector<uint8> audioBuffer;
		std::shared_ptr<CaptureAudioSource> audioSource;
		std::shared_ptr<std::atomic<bool>> callbackEnabled = std::make_shared<std::atomic<bool>>(true);
	};

	class CaptureWindowsSession final : public CaptureSession
	{
	public:
		CaptureWindowsSession() :
			_state(std::make_shared<CaptureWindowsState>()) { _state->audioSource = GetAudioSource(); }
		~CaptureWindowsSession() override
		{
			*_state->callbackEnabled = false;
			_state->Stop();
		}
		bool Start(const Configuration &configuration) override
		{
			if(!configuration.IsValid() || (configuration.streamURL && !RN_CAPTURE_ENABLE_RTMP) || _state->GetIsRecording()) return false;
			if(configuration.openFileCallback || configuration.finishFileCallback) return false;
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
		void Stop() override { _state->Stop(); }
		void Update(float delta) override { _state->Update(delta); }
		bool GetIsRecording() const override { return _state->isRecording && !_state->isWaitingToEndRecording; }
		bool GetIsFinishing() const override { return _state->isWaitingToEndRecording || _state->isReleasingRecordingResources; }
		Framebuffer *GetFramebuffer() const override { return _state->recordingSwapChain ? _state->recordingSwapChain->GetFramebuffer() : nullptr; }

	private:
		std::shared_ptr<CaptureWindowsState> _state;
	};

	CaptureSession *CaptureSessionWithWindowsPlatform()
	{
		return new CaptureWindowsSession();
	}
} // namespace RN

#endif
