//
//  RNCaptureAndroidFileWriter.cpp
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCaptureAndroidFileWriter.h"

#if RN_PLATFORM_ANDROID
	#include <fcntl.h>
	#include <media/NdkMediaCodec.h>
	#include <media/NdkMediaFormat.h>
	#include <media/NdkMediaMuxer.h>
	#include <unistd.h>
#endif

namespace RN
{
#if RN_PLATFORM_ANDROID
	CaptureAndroidFileWriter::CaptureAndroidFileWriter() :
		_videoFD(-1),
		_muxer(nullptr),
		_videoTrackIndex(-1),
		_audioTrackIndex(-1),
		_hasVideoFormat(false),
		_hasAudioFormat(false),
		_isReady(false),
		_sampleWriteFailureCount(0)
	{}

	CaptureAndroidFileWriter::~CaptureAndroidFileWriter()
	{
		Stop();
	}

	bool CaptureAndroidFileWriter::Start(const CaptureSession::Configuration &configuration)
	{
		_finishFileCallback = configuration.finishFileCallback;
		if(configuration.openFileCallback)
		{
			_videoFD = configuration.openFileCallback(configuration.filename);
		}
		else
		{
			FileManager *fileManager = FileManager::GetSharedInstance();
			String *directory = fileManager->GetPathForLocation(FileManager::Location::ExternalSaveDirectory)->StringByAppendingPathComponent(RNCSTR("videos"));
			if(!fileManager->PathExists(directory) && !fileManager->CreateDirectory(directory)) return false;
			const String *filename = configuration.filename;
			if(!filename->GetPathExtension()->IsEqual(RNCSTR("mp4"))) filename = filename->StringByAppendingPathExtension(RNCSTR("mp4"));
			_outputPath = directory->StringByAppendingPathComponent(filename)->Retain();
			_videoFD = open(_outputPath->GetUTF8String(), O_CREAT | O_TRUNC | O_RDWR, 0600);
		}
		if(_videoFD == -1)
		{
			RNWarning("Failed creating video file");
			return false;
		}

		_muxer = AMediaMuxer_new(_videoFD, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
		if(_muxer == nullptr)
		{
			RNWarning("Failed creating muxer");
			Stop();
			return false;
		}

		RNInfo("Started recording to file " << configuration.filename);
		return true;
	}

	void CaptureAndroidFileWriter::Stop()
	{
		bool keepMediaFile = false;
		if(_muxer != nullptr)
		{
			if(_isReady)
			{
				RNDebug("Stop muxer");
				media_status_t err = AMediaMuxer_stop(_muxer);
				if(err != AMEDIA_OK)
				{
					RNWarning("Android muxer stop failed: " << err);
				}
				else
				{
					keepMediaFile = true;
				}
			}

			RNDebug("Release muxer");
			AMediaMuxer_delete(_muxer);
			_muxer = nullptr;
		}

		if(_videoFD != -1)
		{
			RNDebug("Close file");
			int mediaFD = _videoFD;
			close(mediaFD);
			_videoFD = -1;
			if(_finishFileCallback)
				_finishFileCallback(mediaFD, keepMediaFile);
			else if(!keepMediaFile && _outputPath)
				unlink(_outputPath->GetUTF8String());
		}

		SafeRelease(_outputPath);
		_finishFileCallback = nullptr;
		_hasVideoFormat = false;
		_hasAudioFormat = false;
		_videoTrackIndex = -1;
		_audioTrackIndex = -1;
		_isReady = false;
		_sampleWriteFailureCount = 0;
	}

	bool CaptureAndroidFileWriter::DidReceiveVideoFormat(const CaptureEncodedVideoFormat &format)
	{
		if(!_muxer) return false;
		if(_hasVideoFormat)
		{
			RNWarning("ERROR: video format changed twice");
			return false;
		}

		AMediaFormat *mediaFormat = AMediaFormat_new();
		if(!mediaFormat) return false;

		AMediaFormat_setString(mediaFormat, AMEDIAFORMAT_KEY_MIME, "video/avc");
		AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_WIDTH, format.width);
		AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_HEIGHT, format.height);
		if(!AddBufferToFormat(mediaFormat, "csd-0", format.codecConfiguration, format.codecConfigurationSize))
		{
			AddBufferToFormat(mediaFormat, "csd-0", format.sequenceParameterSet, format.sequenceParameterSetSize);
		}
		if(!AddBufferToFormat(mediaFormat, "csd-1", format.codecConfiguration2, format.codecConfiguration2Size))
		{
			AddBufferToFormat(mediaFormat, "csd-1", format.pictureParameterSet, format.pictureParameterSetSize);
		}

		_videoTrackIndex = AMediaMuxer_addTrack(_muxer, mediaFormat);
		AMediaFormat_delete(mediaFormat);
		if(_videoTrackIndex < 0) return false;
		_hasVideoFormat = true;
		return StartMuxerIfReady();
	}

	bool CaptureAndroidFileWriter::DidReceiveAudioFormat(const CaptureEncodedAudioFormat &format)
	{
		if(!_muxer) return false;
		if(_hasAudioFormat)
		{
			RNWarning("ERROR: audio format changed twice");
			return false;
		}

		AMediaFormat *mediaFormat = AMediaFormat_new();
		if(!mediaFormat) return false;

		AMediaFormat_setString(mediaFormat, AMEDIAFORMAT_KEY_MIME, "audio/mp4a-latm");
		AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_SAMPLE_RATE, format.sampleRate);
		AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_CHANNEL_COUNT, format.channelCount);
		AddBufferToFormat(mediaFormat, "csd-0", format.audioSpecificConfig, format.audioSpecificConfigSize);

		_audioTrackIndex = AMediaMuxer_addTrack(_muxer, mediaFormat);
		AMediaFormat_delete(mediaFormat);
		if(_audioTrackIndex < 0) return false;
		_hasAudioFormat = true;
		return StartMuxerIfReady();
	}

	bool CaptureAndroidFileWriter::GetIsReady() const
	{
		return _isReady;
	}

	void CaptureAndroidFileWriter::WriteVideoSample(const CaptureEncodedSample &sample)
	{
		WriteSample(_videoTrackIndex, sample);
	}

	void CaptureAndroidFileWriter::WriteAudioSample(const CaptureEncodedSample &sample)
	{
		WriteSample(_audioTrackIndex, sample);
	}

	bool CaptureAndroidFileWriter::AddBufferToFormat(AMediaFormat *mediaFormat, const char *name, const uint8 *data, size_t size)
	{
		if(!mediaFormat || !data || size == 0) return false;
		AMediaFormat_setBuffer(mediaFormat, name, const_cast<uint8 *>(data), size);
		return true;
	}

	void CaptureAndroidFileWriter::WriteSample(ssize_t trackIndex, const CaptureEncodedSample &sample)
	{
		if(!_isReady || trackIndex < 0 || !sample.data || sample.size == 0 || sample.isCodecConfig) return;

		AMediaCodecBufferInfo bufferInfo;
		bufferInfo.offset = static_cast<int32_t>(sample.offset);
		bufferInfo.size = static_cast<int32_t>(sample.size);
		bufferInfo.presentationTimeUs = sample.presentationTimeUs;
		bufferInfo.flags = 0;
		if(sample.isKeyFrame) bufferInfo.flags |= AndroidCodecBufferFlagKeyFrame;

		media_status_t err = AMediaMuxer_writeSampleData(_muxer, trackIndex, const_cast<uint8 *>(sample.data), &bufferInfo);
		if(err != AMEDIA_OK)
		{
			_sampleWriteFailureCount += 1;
			if(_sampleWriteFailureCount <= 3 || _sampleWriteFailureCount % 300 == 0)
			{
				RNWarning("Android muxer writeSampleData failed for track " << trackIndex << ", size: " << sample.size << ", PTS: " << sample.presentationTimeUs << ", keyframe: " << sample.isKeyFrame << ", err: " << err);
			}
		}
	}

	bool CaptureAndroidFileWriter::StartMuxerIfReady()
	{
		if(_isReady) return true;
		if(!_hasVideoFormat || !_hasAudioFormat) return true;

		media_status_t err = AMediaMuxer_start(_muxer);
		if(err != AMEDIA_OK)
		{
			_hasVideoFormat = false;
			_hasAudioFormat = false;
			RNWarning("Error occurred: " << err);
			return false;
		}

		_isReady = true;
		return true;
	}
#endif
} // namespace RN
