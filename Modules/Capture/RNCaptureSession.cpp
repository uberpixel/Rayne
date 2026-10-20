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
		_droppedFrames(0),
		_readFrame(0),
		_writeFrame(0),
		_fractionalFrames(0.0)
	{}

	void CaptureAudioSource::Configure(const CaptureSession::Configuration &configuration)
	{
		_lock.Lock();
		_readFrame = 0;
		_writeFrame = 0;
		_fractionalFrames = 0.0;
		_droppedFrames = 0;
		memset(_sampleFrames, 0, sizeof(_sampleFrames));
		_frameCountCallback = configuration.audioFrameCountCallback;
		_samplesCallback = configuration.audioSamplesCallback;
		_lock.Unlock();
	}

	bool CaptureAudioSource::SubmitSamples(const float *samples, size_t frames, uint32 sampleRate)
	{
		if(!samples || sampleRate != 48000) return false;
		if(!_lock.TryLock())
		{
			_droppedFrames.fetch_add(frames);
			return false;
		}
		_writeFrame += _droppedFrames.exchange(0);
		size_t offset = frames > Capacity ? frames - Capacity : 0;
		for(size_t frame = offset; frame < frames; frame++)
		{
			size_t index = ((_writeFrame + frame) % Capacity) * 2;
			_sampleFrames[(_writeFrame + frame) % Capacity] = _writeFrame + frame + 1;
			for(size_t channel = 0; channel < 2; channel++)
			{
				float value = samples[frame * 2 + channel];
				if(!std::isfinite(value)) value = 0.0f;
				value = std::max(-1.0f, std::min(1.0f, value));
				_samples[index + channel] = static_cast<int16>(value * 32767.0f);
			}
		}
		_writeFrame += frames;
		_lock.Unlock();
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
		int16 *output = static_cast<int16 *>(destination);
		while(frames > 0)
		{
			size_t chunkFrames = std::min(frames, static_cast<size_t>(1024));
			_lock.Lock();
			_writeFrame += _droppedFrames.exchange(0);
			uint64 oldest = _writeFrame > Capacity ? _writeFrame - Capacity : 0;
			for(size_t frame = 0; frame < chunkFrames; frame++)
			{
				uint64 position = _readFrame + frame;
				bool available = position >= oldest && position < _writeFrame && _sampleFrames[position % Capacity] == position + 1;
				size_t index = (position % Capacity) * 2;
				if(output)
				{
					output[frame * 2] = available ? _samples[index] : 0;
					output[frame * 2 + 1] = available ? _samples[index + 1] : 0;
				}
			}
			_readFrame += chunkFrames;
			if(_writeFrame < _readFrame) _writeFrame = _readFrame;
			_lock.Unlock();
			if(output) output += chunkFrames * 2;
			frames -= chunkFrames;
		}
	}
} // namespace RN
