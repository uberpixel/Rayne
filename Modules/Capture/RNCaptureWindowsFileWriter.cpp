//
//  RNCaptureWindowsFileWriter.cpp
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCaptureWindowsFileWriter.h"

#if RN_PLATFORM_WINDOWS

namespace RN
{
	using Microsoft::WRL::ComPtr;

	namespace CaptureFileDetails
	{
		static constexpr uint16 AACPayloadTypeRaw = 0;
		static constexpr uint16 AACProfileLevelUnknown = 0;
		static constexpr uint16 AACStructTypeAudioSpecificConfig = 0;

		void AppendU32BE(std::vector<uint8> &data, uint32 value)
		{
			data.push_back(static_cast<uint8>((value >> 24) & 0xff));
			data.push_back(static_cast<uint8>((value >> 16) & 0xff));
			data.push_back(static_cast<uint8>((value >> 8) & 0xff));
			data.push_back(static_cast<uint8>(value & 0xff));
		}

		void AppendU16LE(std::vector<uint8> &data, uint16 value)
		{
			data.push_back(static_cast<uint8>(value & 0xff));
			data.push_back(static_cast<uint8>((value >> 8) & 0xff));
		}

		void AppendU32LE(std::vector<uint8> &data, uint32 value)
		{
			data.push_back(static_cast<uint8>(value & 0xff));
			data.push_back(static_cast<uint8>((value >> 8) & 0xff));
			data.push_back(static_cast<uint8>((value >> 16) & 0xff));
			data.push_back(static_cast<uint8>((value >> 24) & 0xff));
		}

		uint32 ReadU32BE(const uint8 *data)
		{
			return (static_cast<uint32>(data[0]) << 24) | (static_cast<uint32>(data[1]) << 16) | (static_cast<uint32>(data[2]) << 8) | data[3];
		}

		bool StartsWithAnnexBStartCode(const uint8 *data, size_t size)
		{
			return size >= 4 && data[0] == 0 && data[1] == 0 && ((data[2] == 1) || (data[2] == 0 && data[3] == 1));
		}

		size_t FindStartCode(const uint8 *data, size_t size, size_t offset)
		{
			for(size_t i = offset; i + 3 < size; i++)
			{
				if(data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1) return i;
				if(i + 4 <= size && data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 0 && data[i + 3] == 1) return i;
			}

			return std::string::npos;
		}

		size_t StartCodeLength(const uint8 *data, size_t size)
		{
			if(size >= 4 && data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1) return 4;
			return 3;
		}

		void AddH264ParameterSet(const uint8 *data, size_t size, std::vector<uint8> &sps, std::vector<uint8> &pps)
		{
			if(!data || size == 0) return;

			uint8 type = data[0] & 0x1f;
			if(type == 7 && sps.empty())
			{
				sps.assign(data, data + size);
			}
			else if(type == 8 && pps.empty())
			{
				pps.assign(data, data + size);
			}
		}

		void ExtractAnnexBParameterSets(const uint8 *data, size_t size, std::vector<uint8> &sps, std::vector<uint8> &pps)
		{
			size_t offset = 0;
			while(offset + 3 < size)
			{
				size_t startCodeOffset = FindStartCode(data, size, offset);
				if(startCodeOffset == std::string::npos) break;

				size_t naluStart = startCodeOffset + StartCodeLength(data + startCodeOffset, size - startCodeOffset);
				size_t nextStartCodeOffset = FindStartCode(data, size, naluStart);
				size_t naluEnd = nextStartCodeOffset == std::string::npos ? size : nextStartCodeOffset;

				while(naluEnd > naluStart && data[naluEnd - 1] == 0) naluEnd--;
				if(naluEnd > naluStart) AddH264ParameterSet(data + naluStart, naluEnd - naluStart, sps, pps);

				if(nextStartCodeOffset == std::string::npos) break;
				offset = nextStartCodeOffset;
			}
		}

		bool ParseAVCDecoderConfigurationRecord(const uint8 *data, size_t size, std::vector<uint8> &sps, std::vector<uint8> &pps)
		{
			if(size < 7 || data[0] != 1) return false;

			size_t offset = 5;
			uint8 spsCount = data[offset++] & 0x1f;
			for(uint8 i = 0; i < spsCount && offset + 2 <= size; i++)
			{
				size_t naluSize = (static_cast<size_t>(data[offset]) << 8) | data[offset + 1];
				offset += 2;
				if(offset + naluSize > size) return false;
				if(sps.empty()) sps.assign(data + offset, data + offset + naluSize);
				offset += naluSize;
			}

			if(offset >= size) return false;
			uint8 ppsCount = data[offset++];
			for(uint8 i = 0; i < ppsCount && offset + 2 <= size; i++)
			{
				size_t naluSize = (static_cast<size_t>(data[offset]) << 8) | data[offset + 1];
				offset += 2;
				if(offset + naluSize > size) return false;
				if(pps.empty()) pps.assign(data + offset, data + offset + naluSize);
				offset += naluSize;
			}

			return !sps.empty() && !pps.empty();
		}

		void ExtractLengthPrefixedParameterSets(const uint8 *data, size_t size, std::vector<uint8> &sps, std::vector<uint8> &pps)
		{
			size_t offset = 0;
			while(offset + 4 <= size)
			{
				uint32 naluSize = ReadU32BE(data + offset);
				offset += 4;
				if(naluSize == 0 || offset + naluSize > size) return;

				AddH264ParameterSet(data + offset, naluSize, sps, pps);
				offset += naluSize;
			}
		}

		void ExtractH264ParameterSets(const uint8 *data, size_t size, std::vector<uint8> &sps, std::vector<uint8> &pps)
		{
			if(!data || size == 0) return;
			if(ParseAVCDecoderConfigurationRecord(data, size, sps, pps)) return;
			if(StartsWithAnnexBStartCode(data, size))
			{
				ExtractAnnexBParameterSets(data, size, sps, pps);
				return;
			}

			ExtractLengthPrefixedParameterSets(data, size, sps, pps);
			AddH264ParameterSet(data, size, sps, pps);
		}

		void AppendAnnexBNALUnit(std::vector<uint8> &data, const std::vector<uint8> &nalUnit)
		{
			AppendU32BE(data, 1);
			data.insert(data.end(), nalUnit.begin(), nalUnit.end());
		}

		bool IsH264ParameterSet(const uint8 *data, size_t size)
		{
			if(!data || size == 0) return false;

			uint8 type = data[0] & 0x1f;
			return type == 7 || type == 8;
		}

		bool AppendAnnexBVideoSampleNALUnit(std::vector<uint8> &data, const uint8 *nalUnit, size_t size)
		{
			if(!nalUnit || size == 0 || IsH264ParameterSet(nalUnit, size)) return false;

			AppendU32BE(data, 1);
			data.insert(data.end(), nalUnit, nalUnit + size);
			return true;
		}

		bool BuildH264SequenceHeader(const CaptureEncodedVideoFormat &format, std::vector<uint8> &sequenceHeader)
		{
			std::vector<uint8> sps;
			std::vector<uint8> pps;

			ExtractH264ParameterSets(format.sequenceParameterSet, format.sequenceParameterSetSize, sps, pps);
			ExtractH264ParameterSets(format.pictureParameterSet, format.pictureParameterSetSize, sps, pps);
			ExtractH264ParameterSets(format.codecConfiguration, format.codecConfigurationSize, sps, pps);
			ExtractH264ParameterSets(format.codecConfiguration2, format.codecConfiguration2Size, sps, pps);

			if(sps.empty() || pps.empty()) return false;

			sequenceHeader.clear();
			AppendAnnexBNALUnit(sequenceHeader, sps);
			AppendAnnexBNALUnit(sequenceHeader, pps);
			return true;
		}

		bool BuildAACUserData(const CaptureEncodedAudioFormat &format, std::vector<uint8> &userData)
		{
			if(!format.audioSpecificConfig || format.audioSpecificConfigSize == 0) return false;

			userData.clear();
			AppendU16LE(userData, AACPayloadTypeRaw);
			AppendU16LE(userData, AACProfileLevelUnknown);
			AppendU16LE(userData, AACStructTypeAudioSpecificConfig);
			AppendU16LE(userData, 0); // wReserved1
			AppendU32LE(userData, 0); // dwReserved2
			userData.insert(userData.end(), format.audioSpecificConfig, format.audioSpecificConfig + format.audioSpecificConfigSize);
			return true;
		}
	} // namespace CaptureFileDetails

	using namespace CaptureFileDetails;

	CaptureWindowsFileWriter::CaptureWindowsFileWriter() :
		_videoStreamIndex(0),
		_audioStreamIndex(0),
		_videoFrameRate(0),
		_videoBitRate(0),
		_audioBitRate(0),
		_outputPath(nullptr),
		_didInitializeCOM(false),
		_didStartMediaFoundation(false),
		_isStarted(false),
		_hasVideoFormat(false),
		_hasAudioFormat(false),
		_didWriteVideoSample(false),
		_didWriteAudioSample(false),
		_isReady(false)
	{}

	CaptureWindowsFileWriter::~CaptureWindowsFileWriter()
	{
		Stop();
		SafeRelease(_outputPath);
	}

	bool CaptureWindowsFileWriter::Start(const CaptureSession::Configuration &configuration)
	{
		if(!CreateOutputPath(configuration)) return false;

		_videoFrameRate = CaptureWindowsInternals::GetVideoFrameRate(configuration);
		_videoBitRate = static_cast<UINT32>(configuration.videoBitRate > 0 ? configuration.videoBitRate : CaptureWindowsInternals::DefaultFileVideoBitRate);
		_audioBitRate = static_cast<UINT32>(configuration.audioBitRate > 0 ? configuration.audioBitRate : CaptureWindowsInternals::DefaultAudioBitRate);

		HRESULT result = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		if(SUCCEEDED(result))
		{
			_didInitializeCOM = true;
		}
		else if(result != RPC_E_CHANGED_MODE)
		{
			CaptureWindowsInternals::LogMediaFailure("COM initialization", result);
			return false;
		}

		result = MFStartup(MF_VERSION);
		if(CaptureWindowsInternals::LogMediaFailure("startup", result)) return false;
		_didStartMediaFoundation = true;

		_isStarted = true;
		RNInfo("Started recording to file " << _outputPath);
		return true;
	}

	void CaptureWindowsFileWriter::Stop()
	{
		LockGuard<Lockable> lock(_lock);
		if(_sinkWriter && _isReady)
		{
			HRESULT result = _sinkWriter->Finalize();
			if(CaptureWindowsInternals::LogMediaFailure("finalize", result))
			{
				RNWarning("Recording Windows file finalize state: video format: " << _hasVideoFormat << ", audio format: " << _hasAudioFormat << ", video sample: " << _didWriteVideoSample << ", audio sample: " << _didWriteAudioSample);
			}
		}

		_isReady = false;
		_isStarted = false;
		_hasVideoFormat = false;
		_hasAudioFormat = false;
		_didWriteVideoSample = false;
		_didWriteAudioSample = false;
		_videoSequenceHeader.clear();
		_videoStreamIndex = 0;
		_audioStreamIndex = 0;
		_videoFrameRate = 0;
		_videoBitRate = 0;
		_audioBitRate = 0;
		_sinkWriter.Reset();
		_mediaSink.Reset();
		_byteStream.Reset();
		_videoMediaType.Reset();
		_audioMediaType.Reset();

		if(_didStartMediaFoundation)
		{
			MFShutdown();
			_didStartMediaFoundation = false;
		}

		if(_didInitializeCOM)
		{
			::CoUninitialize();
			_didInitializeCOM = false;
		}
	}

	bool CaptureWindowsFileWriter::DidReceiveVideoFormat(const CaptureEncodedVideoFormat &format)
	{
		LockGuard<Lockable> lock(_lock);
		if(!_isStarted) return false;
		if(_hasVideoFormat)
		{
			return true;
		}

		if(!ConfigureVideoStream(format)) return false;
		_hasVideoFormat = true;
		bool didStartWriting = StartWritingIfReady();
		if(!didStartWriting)
		{
			_hasVideoFormat = false;
			_videoMediaType.Reset();
			_videoSequenceHeader.clear();
		}
		return didStartWriting;
	}

	bool CaptureWindowsFileWriter::DidReceiveAudioFormat(const CaptureEncodedAudioFormat &format)
	{
		LockGuard<Lockable> lock(_lock);
		if(!_isStarted) return false;
		if(_hasAudioFormat)
		{
			return true;
		}

		if(!ConfigureAudioStream(format)) return false;
		_hasAudioFormat = true;
		bool didStartWriting = StartWritingIfReady();
		if(!didStartWriting)
		{
			_hasAudioFormat = false;
			_audioMediaType.Reset();
		}
		return didStartWriting;
	}

	bool CaptureWindowsFileWriter::GetIsReady() const
	{
		return _isReady;
	}

	void CaptureWindowsFileWriter::WriteVideoSample(const CaptureEncodedSample &sample)
	{
		std::vector<uint8> sequenceHeader;
		{
			LockGuard<Lockable> lock(_lock);
			if(!_didWriteVideoSample) sequenceHeader = _videoSequenceHeader;
		}

		std::vector<uint8> data;
		if(!BuildVideoSample(sample, sequenceHeader, data)) return;

		WriteSample(_videoStreamIndex, sample, data.data(), data.size(), true);
	}

	void CaptureWindowsFileWriter::WriteAudioSample(const CaptureEncodedSample &sample)
	{
		if(!sample.data || sample.size == 0 || sample.isCodecConfig) return;

		WriteSample(_audioStreamIndex, sample, sample.data + sample.offset, sample.size, false);
	}

	bool CaptureWindowsFileWriter::CreateOutputPath(const CaptureSession::Configuration &configuration)
	{
		FileManager *fileManager = FileManager::GetSharedInstance();
		String *directory = fileManager->GetPathForLocation(FileManager::Location::ExternalSaveDirectory)->Copy();
		directory->AppendPathComponent(RNCSTR("videos"));
		if(!fileManager->PathExists(directory))
		{
			if(!fileManager->CreateDirectory(directory))
			{
				directory->Release();
				RNWarning("Failed creating video directory");
				return false;
			}
		}

		const String *filename = configuration.filename ? configuration.filename : RNCSTR("recording.mp4");
		if(!filename->GetPathExtension()->IsEqual(RNCSTR("mp4"))) filename = filename->StringByAppendingPathExtension(RNCSTR("mp4"));
		SafeRelease(_outputPath);
		_outputPath = directory->StringByAppendingPathComponent(filename)->Retain();
		directory->Release();
		return true;
	}

	bool CaptureWindowsFileWriter::SetBlobAttribute(IMFMediaType *mediaType, const GUID &key, const uint8 *data, size_t size)
	{
		if(!mediaType || !data || size == 0) return false;
		if(size > static_cast<size_t>(std::numeric_limits<UINT32>::max())) return false;

		HRESULT result = mediaType->SetBlob(key, data, static_cast<UINT32>(size));
		return !CaptureWindowsInternals::LogMediaFailure("encoded media type blob setup", result);
	}

	bool CaptureWindowsFileWriter::ConfigureVideoStream(const CaptureEncodedVideoFormat &format)
	{
		ComPtr<IMFMediaType> mediaType;
		HRESULT result = MFCreateMediaType(&mediaType);
		if(CaptureWindowsInternals::LogMediaFailure("video media type creation", result)) return false;

		mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
		mediaType->SetUINT32(MF_MT_AVG_BITRATE, _videoBitRate);
		mediaType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
		mediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, FALSE);
		MFSetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, format.width, format.height);
		MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE, _videoFrameRate, 1);
		MFSetAttributeRatio(mediaType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

		std::vector<uint8> sequenceHeader;
		if(!BuildH264SequenceHeader(format, sequenceHeader))
		{
			RNWarning("Recording Windows file writer did not receive H.264 SPS/PPS");
			return false;
		}
		if(!SetBlobAttribute(mediaType.Get(), MF_MT_MPEG_SEQUENCE_HEADER, sequenceHeader.data(), sequenceHeader.size())) return false;
		_videoSequenceHeader = sequenceHeader;
		_videoMediaType = mediaType;
		RNInfo("Configured Windows file video format " << format.width << "x" << format.height << "@" << _videoFrameRate << "Hz, SPS/PPS bytes: " << sequenceHeader.size());

		return true;
	}

	bool CaptureWindowsFileWriter::ConfigureAudioStream(const CaptureEncodedAudioFormat &format)
	{
		ComPtr<IMFMediaType> mediaType;
		HRESULT result = MFCreateMediaType(&mediaType);
		if(CaptureWindowsInternals::LogMediaFailure("audio media type creation", result)) return false;

		mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC);
		mediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, format.channelCount);
		mediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, format.sampleRate);
		mediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, CaptureWindowsInternals::AudioBitsPerSample);
		mediaType->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1);
		mediaType->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, std::max<UINT32>(1, _audioBitRate / 8));
		mediaType->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE, AACPayloadTypeRaw);
		mediaType->SetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, AACProfileLevelUnknown);
		mediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);

		std::vector<uint8> userData;
		if(!BuildAACUserData(format, userData))
		{
			RNWarning("Recording Windows file writer did not receive AAC AudioSpecificConfig");
			return false;
		}
		if(!SetBlobAttribute(mediaType.Get(), MF_MT_USER_DATA, userData.data(), userData.size())) return false;

		_audioMediaType = mediaType;
		RNInfo("Configured Windows file audio format " << format.sampleRate << "Hz, channels: " << format.channelCount << ", AudioSpecificConfig bytes: " << format.audioSpecificConfigSize << ", user data bytes: " << userData.size());

		return true;
	}

	bool CaptureWindowsFileWriter::StartWritingIfReady()
	{
		if(_isReady) return true;
		if(!_hasVideoFormat || !_hasAudioFormat) return true;
		if(!_videoMediaType || !_audioMediaType) return false;

		std::wstring outputPath = CaptureWindowsInternals::CreateWindowsWideString(_outputPath);
		if(outputPath.empty()) return false;

		HRESULT result = MFCreateFile(MF_ACCESSMODE_READWRITE, MF_OPENMODE_DELETE_IF_EXIST, MF_FILEFLAGS_NONE, outputPath.c_str(), &_byteStream);
		if(CaptureWindowsInternals::LogMediaFailure("byte stream creation", result)) return false;

		result = MFCreateMPEG4MediaSink(_byteStream.Get(), _videoMediaType.Get(), _audioMediaType.Get(), &_mediaSink);
		if(CaptureWindowsInternals::LogMediaFailure("MPEG-4 media sink creation", result)) return false;

		_videoStreamIndex = 0;
		_audioStreamIndex = 1;

		ComPtr<IMFAttributes> attributes;
		result = MFCreateAttributes(&attributes, 1);
		if(CaptureWindowsInternals::LogMediaFailure("sink writer attribute creation", result)) return false;

		attributes->SetUINT32(MF_LOW_LATENCY, TRUE);

		result = MFCreateSinkWriterFromMediaSink(_mediaSink.Get(), attributes.Get(), &_sinkWriter);
		if(CaptureWindowsInternals::LogMediaFailure("sink writer creation", result)) return false;

		result = _sinkWriter->BeginWriting();
		if(CaptureWindowsInternals::LogMediaFailure("begin writing", result)) return false;

		_isReady = true;
		RNInfo("Started Windows MPEG-4 sink writer with video stream " << _videoStreamIndex << " and audio stream " << _audioStreamIndex);
		return true;
	}

	bool CaptureWindowsFileWriter::BuildVideoSample(const CaptureEncodedSample &sample, const std::vector<uint8> &sequenceHeader, std::vector<uint8> &data)
	{
		data.clear();
		if(!sample.data || sample.size == 0 || sample.isCodecConfig) return false;

		const uint8 *sampleData = sample.data + sample.offset;
		const size_t sampleSize = sample.size;
		bool didAppendSampleNALUnit = false;

		data.insert(data.end(), sequenceHeader.begin(), sequenceHeader.end());

		if(StartsWithAnnexBStartCode(sampleData, sampleSize))
		{
			size_t offset = 0;
			while(offset + 3 < sampleSize)
			{
				size_t startCodeOffset = FindStartCode(sampleData, sampleSize, offset);
				if(startCodeOffset == std::string::npos) break;

				size_t naluStart = startCodeOffset + StartCodeLength(sampleData + startCodeOffset, sampleSize - startCodeOffset);
				size_t nextStartCodeOffset = FindStartCode(sampleData, sampleSize, naluStart);
				size_t naluEnd = nextStartCodeOffset == std::string::npos ? sampleSize : nextStartCodeOffset;

				while(naluEnd > naluStart && sampleData[naluEnd - 1] == 0) naluEnd--;
				if(naluEnd > naluStart) didAppendSampleNALUnit |= AppendAnnexBVideoSampleNALUnit(data, sampleData + naluStart, naluEnd - naluStart);

				if(nextStartCodeOffset == std::string::npos) break;
				offset = nextStartCodeOffset;
			}

			return didAppendSampleNALUnit;
		}

		std::vector<uint8> lengthPrefixedData;
		bool parsedLengthPrefixedData = false;
		size_t offset = 0;
		while(offset + 4 <= sampleSize)
		{
			uint32 naluSize = ReadU32BE(sampleData + offset);
			offset += 4;
			if(naluSize == 0 || offset + naluSize > sampleSize)
			{
				lengthPrefixedData.clear();
				break;
			}

			parsedLengthPrefixedData = true;
			didAppendSampleNALUnit |= AppendAnnexBVideoSampleNALUnit(lengthPrefixedData, sampleData + offset, naluSize);
			offset += naluSize;
		}

		if(offset == sampleSize && parsedLengthPrefixedData)
		{
			if(!didAppendSampleNALUnit) return false;
			data.insert(data.end(), lengthPrefixedData.begin(), lengthPrefixedData.end());
			return true;
		}

		didAppendSampleNALUnit = AppendAnnexBVideoSampleNALUnit(data, sampleData, sampleSize);
		return didAppendSampleNALUnit;
	}

	bool CaptureWindowsFileWriter::WriteSample(DWORD streamIndex, const CaptureEncodedSample &sample, const uint8 *data, size_t size, bool isVideo)
	{
		LockGuard<Lockable> lock(_lock);
		if(!_isReady || !_sinkWriter || !data || size == 0) return false;

		ComPtr<IMFSample> mediaSample;
		if(!CaptureWindowsInternals::CreateMemorySample(data, size, sample.presentationTimeUs, sample.durationUs, mediaSample)) return false;
		if(isVideo) mediaSample->SetUINT32(MFSampleExtension_CleanPoint, sample.isKeyFrame ? TRUE : FALSE);

		HRESULT result = _sinkWriter->WriteSample(streamIndex, mediaSample.Get());
		if(CaptureWindowsInternals::LogMediaFailure("encoded sample write", result)) return false;

		if(isVideo)
			_didWriteVideoSample = true;
		else
			_didWriteAudioSample = true;
		return true;
	}
} // namespace RN

#endif
