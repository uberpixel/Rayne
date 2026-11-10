//
//  RNCaptureSession.cpp
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCaptureSession.h"

namespace RN
{
#if RN_PLATFORM_ANDROID
	CaptureSession *CaptureSessionWithAndroidPlatform();
#elif RN_PLATFORM_WINDOWS
	CaptureSession *CaptureSessionWithWindowsPlatform();
#elif RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS || RN_PLATFORM_VISIONOS
	CaptureSession *CaptureSessionWithApplePlatform();
#endif

	bool CaptureSession::Configuration::IsValid() const
	{
		return resolvePass && restoreFramebuffer &&
			resolutionX > 0 && resolutionY > 0 &&
			std::isfinite(videoFramePeriod) && videoFramePeriod > 0.0 &&
			(filename || streamURL) &&
			(static_cast<bool>(audioFrameCountCallback) == static_cast<bool>(audioSamplesCallback));
	}

	CaptureSession::CaptureSession() :
		_audioSource(std::make_shared<CaptureAudioSource>())
	{}

	CaptureSession *CaptureSession::WithPlatform()
	{
#if RN_PLATFORM_ANDROID
		return CaptureSessionWithAndroidPlatform();
#elif RN_PLATFORM_WINDOWS
		return CaptureSessionWithWindowsPlatform();
#elif RN_PLATFORM_MAC_OS || RN_PLATFORM_IOS || RN_PLATFORM_VISIONOS
		return CaptureSessionWithApplePlatform();
#else
		return nullptr;
#endif
	}

	bool CaptureSession::SubmitAudioSamples(const float *samples, size_t frames, uint32 sampleRate)
	{
		return _audioSource->SubmitSamples(samples, frames, sampleRate);
	}

	CaptureAudioSource::CaptureAudioSource() :
		_readFrame(0),
		_writeFrame(0),
		_fractionalFrames(0.0)
	{}

	void CaptureAudioSource::Configure(const CaptureSession::Configuration &configuration)
	{
		_readFrame = 0;
		_writeFrame = 0;
		_fractionalFrames = 0.0;
		_frameCountCallback = configuration.audioFrameCountCallback;
		_samplesCallback = configuration.audioSamplesCallback;
	}

	bool CaptureAudioSource::SubmitSamples(const float *samples, size_t frames, uint32 sampleRate)
	{
		if(!samples || sampleRate != 48000) return false;
		const uint64 start = _writeFrame.load(std::memory_order_relaxed);
		const size_t offset = frames > Capacity ? frames - Capacity : 0;
		for(size_t frame = offset; frame < frames; ++frame)
		{
			const uint64 position = start + frame;
			// Publish the stamp and both channels together so an overwrite cannot tear a frame.
			uint64 packed = static_cast<uint64>(static_cast<uint32>(position)) << 32;
			for(size_t channel = 0; channel < 2; ++channel)
			{
				float value = samples[frame * 2 + channel];
				if(!std::isfinite(value)) value = 0.0f;
				const auto pcm = static_cast<int16>(std::clamp(value, -1.0f, 1.0f) * 32767.0f);
				packed |= static_cast<uint64>(static_cast<uint16>(pcm)) << (channel * 16);
			}
			_samples[position % Capacity].store(packed, std::memory_order_relaxed);
		}
		_writeFrame.store(start + frames, std::memory_order_release);
		return true;
	}

	size_t CaptureAudioSource::GetFrameCount(float duration)
	{
		if(_frameCountCallback) return _frameCountCallback(duration);
		_fractionalFrames += std::max(0.0, static_cast<double>(duration)) * 48000.0;
		size_t frames = static_cast<size_t>(_fractionalFrames);
		_fractionalFrames -= frames;
		return frames;
	}

	void CaptureAudioSource::ReadFrames(size_t frames, void *destination)
	{
		if(_samplesCallback)
		{
			_samplesCallback(frames, destination);
			return;
		}
		const uint64 end = _writeFrame.load(std::memory_order_acquire);
		const uint64 oldest = end > Capacity ? end - Capacity : 0;
		// The ring cursor counts submitted frames. Silence on underrun advances only
		// the encoder's timeline, so new input resumes after the silence.
		const size_t count = static_cast<size_t>(std::min<uint64>(frames, end - _readFrame));
		if(destination)
		{
			auto *output = static_cast<int16 *>(destination);
			for(size_t frame = 0; frame < count; ++frame)
			{
				const uint64 position = _readFrame + frame;
				uint64 packed = 0;
				if(position >= oldest)
				{
					packed = _samples[position % Capacity].load(std::memory_order_relaxed);
					if(static_cast<uint32>(packed >> 32) != static_cast<uint32>(position)) packed = 0;
				}
				output[frame * 2] = static_cast<int16>(packed);
				output[frame * 2 + 1] = static_cast<int16>(packed >> 16);
			}
			std::fill(output + count * 2, output + frames * 2, 0);
		}
		_readFrame += count;
	}
} // namespace RN
