//
//  RNCaptureAppleSession.mm
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCaptureSession.h"
#include <RNMetalRenderer.h>
#include <RNMetalSwapChain.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>
#if RN_PLATFORM_IOS
	#import <UIKit/UIKit.h>
#endif

namespace RN
{
	struct CaptureAppleCallbacks
	{
		std::mutex mutex;
		std::atomic<bool> enabled {true};

		void Disable()
		{
			std::lock_guard<std::mutex> lock(mutex);
			enabled = false;
		}
	};

	struct CaptureAppleState : public std::enable_shared_from_this<CaptureAppleState>
	{
		AVAssetWriter *writer = nil;
		AVAssetWriterInput *video = nil;
		AVAssetWriterInput *audio = nil;
		AVAssetWriterInputPixelBufferAdaptor *adaptor = nil;
		CVPixelBufferPoolRef pool = nullptr;
		CVMetalTextureCacheRef cache = nullptr;

		dispatch_queue_t queue = dispatch_queue_create("net.rayne.recording.apple", DISPATCH_QUEUE_SERIAL);

		std::mutex mutex;
		std::atomic<bool> recording {false};
		std::atomic<bool> ready {false};
		std::atomic<bool> finishing {false};
		std::atomic<bool> active {true};
		dispatch_group_t finishGroup = dispatch_group_create();
		size_t videoPending = 0;
		size_t audioPending = 0;
		std::atomic<bool> errorReported {false};
		std::shared_ptr<CaptureAppleCallbacks> callbacksEnabled;
		bool finishQueued = false;
		bool sessionStarted = false; // Accessed only on the writer queue

		uint64 audioPosition = 0;
		std::shared_ptr<CaptureAudioSource> audioSource;

		std::function<void()> completion;
		std::function<void(const String *)> error;
#if RN_PLATFORM_IOS
		id resignObserver = nil;
		UIBackgroundTaskIdentifier backgroundTask = UIBackgroundTaskInvalid;
#endif

		~CaptureAppleState()
		{
#if RN_PLATFORM_IOS
			if(resignObserver) [[NSNotificationCenter defaultCenter] removeObserver:resignObserver];
			[resignObserver release];
#endif
			if(cache) CFRelease(cache);
			if(pool) CFRelease(pool);
			[adaptor release];
			[audio release];
			[video release];
			[writer release];
			dispatch_release(queue);
			dispatch_release(finishGroup);
		}

		void Initialize(NSString *filename, id<MTLDevice> device, int resolutionX, int resolutionY, int videoBitRate, int audioBitRate, double videoFramePeriod)
		{
			NSError *error = nil;
			NSString *path = filename;
			if(!path.isAbsolutePath)
			{
#if RN_PLATFORM_MAC_OS
				NSSearchPathDirectory directory = NSMoviesDirectory;
#else
				NSSearchPathDirectory directory = NSDocumentDirectory;
#endif
				NSString *folder = NSSearchPathForDirectoriesInDomains(directory, NSUserDomainMask, YES).firstObject;
				if(!folder)
				{
					ReportError(@"Unable to locate the recordings folder");
					return;
				}
				[[NSFileManager defaultManager] createDirectoryAtPath:folder withIntermediateDirectories:YES attributes:nil error:&error];
				path = [folder stringByAppendingPathComponent:path];
			}

			NSURL *url = [NSURL fileURLWithPath:path];
			writer = [[AVAssetWriter alloc] initWithURL:url fileType:AVFileTypeMPEG4 error:&error];
			NSDictionary *compression = @{
				AVVideoAverageBitRateKey: @(videoBitRate > 0 ? videoBitRate : 12000000),
				AVVideoExpectedSourceFrameRateKey: @(1.0 / videoFramePeriod),
				AVVideoMaxKeyFrameIntervalKey: @(MAX(1, (int)(2.0 / videoFramePeriod)))
			};
			video = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeVideo outputSettings:@{
				AVVideoCodecKey: AVVideoCodecTypeH264,
				AVVideoWidthKey: @(resolutionX),
				AVVideoHeightKey: @(resolutionY),
				AVVideoCompressionPropertiesKey: compression,
				AVVideoColorPropertiesKey: @{
					AVVideoColorPrimariesKey: AVVideoColorPrimaries_ITU_R_709_2,
					AVVideoTransferFunctionKey: AVVideoTransferFunction_ITU_R_709_2,
					AVVideoYCbCrMatrixKey: AVVideoYCbCrMatrix_ITU_R_709_2
				}
			}];
			audio = [[AVAssetWriterInput alloc] initWithMediaType:AVMediaTypeAudio outputSettings:@{
				AVFormatIDKey: @(kAudioFormatMPEG4AAC),
				AVSampleRateKey: @48000,
				AVNumberOfChannelsKey: @2,
				AVEncoderBitRateKey: @(audioBitRate > 0 ? audioBitRate : 128000)
			}];
			video.expectsMediaDataInRealTime = YES;
			audio.expectsMediaDataInRealTime = YES;

			NSDictionary *attributes = @{
				(id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA),
				(id)kCVPixelBufferWidthKey: @(resolutionX),
				(id)kCVPixelBufferHeightKey: @(resolutionY),
				(id)kCVPixelBufferMetalCompatibilityKey: @YES,
				(id)kCVPixelBufferIOSurfacePropertiesKey: @{}
			};
			adaptor = [[AVAssetWriterInputPixelBufferAdaptor alloc] initWithAssetWriterInput:video sourcePixelBufferAttributes:attributes];
			if(!writer || ![writer canAddInput:video] || ![writer canAddInput:audio])
			{
				ReportError(error.localizedDescription ?: @"Unable to create MP4 encoder");
				return;
			}

			[writer addInput:video];
			[writer addInput:audio];
			if(CVPixelBufferPoolCreate(kCFAllocatorDefault, nullptr, (CFDictionaryRef)attributes, &pool) != kCVReturnSuccess ||
				CVMetalTextureCacheCreate(kCFAllocatorDefault, nullptr, device, nullptr, &cache) != kCVReturnSuccess ||
				![writer startWriting])
			{
				ReportError(writer.error.localizedDescription ?: @"Unable to start MP4 encoder");
				return;
			}

			std::lock_guard<std::mutex> lock(mutex);
			ready = recording && active;
		}

		void ReportError(NSString *message)
		{
			if(errorReported.exchange(true)) return;
			Stop();
			auto callback = error;
			auto enabled = callbacksEnabled;
			std::string description([message UTF8String] ?: "Apple recording failed");
			if(callback)
			{
				dispatch_async(dispatch_get_main_queue(), ^{
					if(!enabled)
					{
						return;
					}
					std::lock_guard<std::mutex> lock(enabled->mutex);
					if(!enabled->enabled)
					{
						return;
					}
					WorkQueue::GetMainQueue()->Perform([callback, description, enabled]() {
						if(enabled->enabled)
						{
							callback(String::WithString(description.c_str()));
						}
					});
				});
			}
		}

		void DidFinish()
		{
			if(writer.status == AVAssetWriterStatusFailed) ReportError(writer.error.localizedDescription);
			finishing = false;
			dispatch_group_leave(finishGroup);
			auto session = shared_from_this();
			dispatch_async(dispatch_get_main_queue(), ^{
#if RN_PLATFORM_IOS
				if(session->backgroundTask != UIBackgroundTaskInvalid)
				{
					[[UIApplication sharedApplication] endBackgroundTask:session->backgroundTask];
					session->backgroundTask = UIBackgroundTaskInvalid;
				}
#endif
				auto enabled = session->callbacksEnabled;
				if(!enabled) return;
				std::lock_guard<std::mutex> lock(enabled->mutex);
				if(!enabled->enabled) return;
				WorkQueue::GetMainQueue()->Perform([session]() {
					auto callback = session->completion;
					auto enabled = session->callbacksEnabled;
					if(callback && !session->errorReported && session->writer.status == AVAssetWriterStatusCompleted && enabled && enabled->enabled)
					{
						callback();
					}
				});
			});
		}

		void FinishLocked()
		{
			if(recording || videoPending || audioPending || finishQueued) return;
			finishQueued = true;
			auto session = shared_from_this();
			dispatch_async(queue, ^{
				if(session->writer.status != AVAssetWriterStatusWriting)
				{
					session->DidFinish();
					return;
				}
				if(!session->sessionStarted) [session->writer startSessionAtSourceTime:kCMTimeZero];
				[session->video markAsFinished];
				[session->audio markAsFinished];
				[session->writer finishWritingWithCompletionHandler:^{
					session->DidFinish();
				}];
			});
		}

		void Stop()
		{
			std::lock_guard<std::mutex> lock(mutex);
			if(!recording.exchange(false)) return;
			ready = false;
			finishing = true;
			dispatch_group_enter(finishGroup);
			FinishLocked();
		}

		bool Reserve(bool isAudio = false)
		{
			std::lock_guard<std::mutex> lock(mutex);
			size_t &count = isAudio ? audioPending : videoPending;
			if(!recording || !ready || !active || count >= (isAudio ? 4 : 3)) return false;
			count++;
			return true;
		}

		void ReleasePending(bool isAudio = false)
		{
			std::lock_guard<std::mutex> lock(mutex);
			(isAudio ? audioPending : videoPending)--;
			FinishLocked();
		}

		void CompleteVideo(CVPixelBufferRef buffer, double time, bool success)
		{
			auto session = shared_from_this();
			if(!buffer)
			{
				ReleasePending();
				return;
			}
			dispatch_async(queue, ^{
				if(session->writer.status == AVAssetWriterStatusFailed)
				{
					session->ReportError(session->writer.error.localizedDescription);
				}
				if(success && session->writer.status == AVAssetWriterStatusWriting && session->video.readyForMoreMediaData)
				{
					if(!session->sessionStarted)
					{
						[session->writer startSessionAtSourceTime:CMTimeMakeWithSeconds(time, 1000000)];
						session->sessionStarted = true;
					}
					if(![session->adaptor appendPixelBuffer:buffer withPresentationTime:CMTimeMakeWithSeconds(time, 1000000)])
					{
						session->ReportError(session->writer.error.localizedDescription);
					}
				}
				CFRelease(buffer);
				session->ReleasePending();
			});
		}

		void PushAudio(float delta)
		{
			size_t count = audioSource->GetFrameCount(delta);
			if(!count) return;
			uint64 position = audioPosition;
			audioPosition += count;
			// Bound audio blocks and queued work while preserving elapsed sample time
			if(count > 4800)
			{
				size_t skipped = count - 4800;
				audioSource->ReadFrames(skipped, nullptr);
				position += skipped;
				count = 4800;
			}
			if(!Reserve(true))
			{
				audioSource->ReadFrames(count, nullptr);
				return;
			}
			CMBlockBufferRef block = nullptr;
			OSStatus result = CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault, nullptr, count * 4, kCFAllocatorDefault, nullptr, 0, count * 4, kCMBlockBufferAssureMemoryNowFlag, &block);
			if(result == noErr)
			{
				char *bytes = nullptr;
				result = CMBlockBufferGetDataPointer(block, 0, nullptr, nullptr, &bytes);
				if(result == noErr) audioSource->ReadFrames(count, bytes);
			}
			if(result != noErr)
			{
				if(block) CFRelease(block);
				audioSource->ReadFrames(count, nullptr);
				ReleasePending(true);
				return;
			}
			auto session = shared_from_this();
			dispatch_async(queue, ^{
				if(!session->sessionStarted)
				{
					CFRelease(block);
					session->ReleasePending(true);
					return;
				}
				AudioStreamBasicDescription format = {};
				format.mSampleRate = 48000;
				format.mFormatID = kAudioFormatLinearPCM;
				format.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
				format.mBytesPerPacket = format.mBytesPerFrame = 4;
				format.mFramesPerPacket = 1;
				format.mChannelsPerFrame = 2;
				format.mBitsPerChannel = 16;
				CMAudioFormatDescriptionRef description = nullptr;
				CMSampleBufferRef samples = nullptr;
				size_t sampleSize = 4;
				CMSampleTimingInfo timing = {CMTimeMake(1, 48000), CMTimeMake(position, 48000), kCMTimeInvalid};
				if(CMAudioFormatDescriptionCreate(kCFAllocatorDefault, &format, 0, nullptr, 0, nullptr, nullptr, &description) == noErr)
				{
					CMSampleBufferCreateReady(kCFAllocatorDefault, block, description, count, 1, &timing, 1, &sampleSize, &samples);
				}
				if(!samples) session->ReportError(@"Unable to prepare PCM audio samples");
				if(session->writer.status == AVAssetWriterStatusFailed)
				{
					session->ReportError(session->writer.error.localizedDescription);
				}
				if(samples && session->writer.status == AVAssetWriterStatusWriting && session->audio.readyForMoreMediaData)
				{
					if(![session->audio appendSampleBuffer:samples])
					{
						session->ReportError(session->writer.error.localizedDescription);
					}
				}
				if(samples) CFRelease(samples);
				if(description) CFRelease(description);
				CFRelease(block);
				session->ReleasePending(true);
			});
		}
	};

	class CaptureAppleSwapChain : public MetalSwapChain
	{
	public:
		CaptureAppleSwapChain(std::shared_ptr<CaptureAppleState> session, const Vector2 &size) :
			_session(session)
		{
			_renderer = Renderer::GetActiveRenderer()->Downcast<MetalRenderer>();
			_size = size;
			_descriptor.colorFormat = Texture::Format::BGRA_8_SRGB;
			_descriptor.depthStencilFormat = Texture::Format::Invalid;
			_framebuffer = CreateSwapChainFramebuffer();
		}

		~CaptureAppleSwapChain()
		{
			DiscardBuffer();
			_framebuffer->Release();
		}

		void SetTime(double time) { _time = time; }
		void SetRenderTime(double time) { _renderTime = time; }
		RenderFramePresentationState *TakeRenderFramePresentationState(uint64 frameID) override;

		void AcquireBackBuffer() override
		{
			if(!_session->Reserve()) return;
			NSDictionary *options = @{(id)kCVPixelBufferPoolAllocationThresholdKey: @3};
			if(CVPixelBufferPoolCreatePixelBufferWithAuxAttributes(kCFAllocatorDefault, _session->pool, (CFDictionaryRef)options, &_buffer) != kCVReturnSuccess)
			{
				_session->CompleteVideo(nullptr, 0.0, false);
				return;
			}
			CVBufferSetAttachment(_buffer, kCVImageBufferColorPrimariesKey, kCVImageBufferColorPrimaries_ITU_R_709_2, kCVAttachmentMode_ShouldPropagate);
			CVBufferSetAttachment(_buffer, kCVImageBufferTransferFunctionKey, kCVImageBufferTransferFunction_sRGB, kCVAttachmentMode_ShouldPropagate);
			CVBufferSetAttachment(_buffer, kCVImageBufferYCbCrMatrixKey, kCVImageBufferYCbCrMatrix_ITU_R_709_2, kCVAttachmentMode_ShouldPropagate);
			if(CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault, _session->cache, _buffer, nullptr, MTLPixelFormatBGRA8Unorm_sRGB, _size.x, _size.y, 0, &_texture) != kCVReturnSuccess)
			{
				DiscardBuffer();
			}
		}

		id GetMetalColorTexture() const override { return _texture ? CVMetalTextureGetTexture(_texture) : nil; }
		id GetMetalDepthTexture() const override { return nil; }

		void PresentBackBuffer(id<MTLCommandBuffer> commandBuffer) override
		{
			if(!_buffer) return;
			auto session = _session;
			CVPixelBufferRef buffer = _buffer;
			CVMetalTextureRef texture = _texture;
			double time = _renderTime;
			_buffer = nullptr;
			_texture = nullptr;
			[commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> completed) {
				CFRelease(texture);
				session->CompleteVideo(buffer, time, completed.status == MTLCommandBufferStatusCompleted);
			}];
		}

	private:
		void DiscardBuffer()
		{
			if(_texture) CFRelease(_texture);
			_texture = nullptr;
			if(_buffer)
			{
				CFRelease(_buffer);
				_buffer = nullptr;
				_session->CompleteVideo(nullptr, 0.0, false);
			}
		}

		std::shared_ptr<CaptureAppleState> _session;
		CVPixelBufferRef _buffer = nullptr;
		CVMetalTextureRef _texture = nullptr;
		double _time = 0.0;
		double _renderTime = 0.0;
	};

	class CaptureApplePresentation : public RenderFramePresentationState
	{
	public:
		CaptureApplePresentation(CaptureAppleSwapChain *swapChain, double time) :
			_swapChain(swapChain),
			_time(time)
		{}

		bool BeginFrameOnRenderThread() override
		{
			_swapChain->SetRenderTime(_time);
			return true;
		}

	private:
		StrongRef<CaptureAppleSwapChain> _swapChain;
		double _time;
	};

	RenderFramePresentationState *CaptureAppleSwapChain::TakeRenderFramePresentationState(uint64 frameID)
	{
		return static_cast<RenderFramePresentationState *>((new CaptureApplePresentation(this, _time))->Autorelease());
	}

	class CaptureAppleSession : public CaptureSession
	{
	public:
		~CaptureAppleSession() override
		{
			_callbacksEnabled->Disable();
			Stop();
		}

		bool Start(const Configuration &configuration) override
		{
			if(GetIsRecording() || GetIsFinishing()) return false;
			if(!configuration.IsValid() || !configuration.filename || configuration.streamURL || configuration.openFileCallback || configuration.finishFileCallback)
			{
				return false;
			}

			auto activeRenderer = Renderer::GetActiveRenderer();
			auto renderer = activeRenderer ? activeRenderer->Downcast<MetalRenderer>() : nullptr;
			if(!renderer) return false;

			Stop();
			_callbacksEnabled->Disable();
			_callbacksEnabled = std::make_shared<CaptureAppleCallbacks>();
			auto session = std::make_shared<CaptureAppleState>();
			session->callbacksEnabled = _callbacksEnabled;
			session->completion = configuration.completionCallback;
			session->error = configuration.errorCallback;
			session->audioSource = GetAudioSource();
			session->audioSource->Configure(configuration);

			NSString *filename = [NSString stringWithUTF8String:configuration.filename->GetUTF8String()];
			id<MTLDevice> device = [(id<MTLCommandQueue>)renderer->GetCommandQueue() device];
			int resolutionX = configuration.resolutionX;
			int resolutionY = configuration.resolutionY;
			int videoBitRate = configuration.videoBitRate;
			int audioBitRate = configuration.audioBitRate;
			double videoFramePeriod = configuration.videoFramePeriod;
			session->recording = true;
#if RN_PLATFORM_IOS
			session->active = [UIApplication sharedApplication].applicationState == UIApplicationStateActive;
#endif
			dispatch_async(session->queue, ^{
				@autoreleasepool
				{
					session->Initialize(filename, device, resolutionX, resolutionY, videoBitRate, audioBitRate, videoFramePeriod);
				}
			});
#if RN_PLATFORM_IOS
			std::weak_ptr<CaptureAppleState> weakSession = session;
			session->resignObserver = [[[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationWillResignActiveNotification object:nil queue:nil usingBlock:^(NSNotification *) {
				if(auto current = weakSession.lock())
				{
					current->active = false;
					if((!current->recording && !current->finishing) || current->backgroundTask != UIBackgroundTaskInvalid)
					{
						return;
					}
					current->backgroundTask = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
						if(auto expired = weakSession.lock())
						{
							expired->ReportError(@"Recording interrupted because background time expired");
							dispatch_async(expired->queue, ^{
								[expired->writer cancelWriting];
							});
							if(expired->backgroundTask != UIBackgroundTaskInvalid)
							{
								[[UIApplication sharedApplication] endBackgroundTask:expired->backgroundTask];
								expired->backgroundTask = UIBackgroundTaskInvalid;
							}
						}
					}];
					current->Stop();
				}
			}] retain];
#endif

			_session = session;
			_swapChain = new CaptureAppleSwapChain(session, Vector2(configuration.resolutionX, configuration.resolutionY));
			_pass = SafeRetain(configuration.resolvePass);
			_restore = SafeRetain(configuration.restoreFramebuffer);
			_startTime = CACurrentMediaTime();
			_audioTime = 0.0;
			_nextFrame = 0.0;
			_period = configuration.videoFramePeriod;
			return true;
		}

		void Stop() override
		{
			if(_pass) _pass->SetFramebuffer(_restore);
			SafeRelease(_pass);
			SafeRelease(_restore);
			if(_session) _session->Stop();
			SafeRelease(_swapChain);
		}

		void PrepareForShutdown() override
		{
			_callbacksEnabled->Disable();
			Stop();
			if(!_session || !_session->finishing) return;
			if(dispatch_group_wait(_session->finishGroup, dispatch_time(DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC)) != 0)
			{
				auto session = _session;
				dispatch_async(session->queue, ^{
					[session->writer cancelWriting];
				});
			}
		}

		void Update(float delta) override
		{
			if(!GetIsRecording())
			{
				if(_pass) Stop();
				return;
			}
			_pass->SetFramebuffer(_restore);
			if(delta < 0.0f || !std::isfinite(delta)) return;

			double elapsed = CACurrentMediaTime() - _startTime;
			_session->PushAudio(static_cast<float>(elapsed - _audioTime));
			_audioTime = elapsed;
			if(_session->ready && elapsed >= _nextFrame)
			{
				_swapChain->SetTime(elapsed);
				_pass->SetFramebuffer(_swapChain->GetFramebuffer());
				_nextFrame = (std::floor(elapsed / _period) + 1.0) * _period;
			}
		}

		bool GetIsRecording() const override { return _session && _session->recording; }
		bool GetIsFinishing() const override { return _session && _session->finishing; }
		Framebuffer *GetFramebuffer() const override { return _swapChain ? _swapChain->GetFramebuffer() : nullptr; }

	private:
		std::shared_ptr<CaptureAppleState> _session;
		std::shared_ptr<CaptureAppleCallbacks> _callbacksEnabled = std::make_shared<CaptureAppleCallbacks>();
		CaptureAppleSwapChain *_swapChain = nullptr;
		PostProcessingAPIStage *_pass = nullptr;
		Framebuffer *_restore = nullptr;
		double _startTime = 0.0;
		double _audioTime = 0.0;
		double _nextFrame = 0.0;
		double _period = 1.0 / 30.0;
	};

	CaptureSession *CaptureSessionWithApplePlatform()
	{
		return new CaptureAppleSession();
	}
} // namespace RN
