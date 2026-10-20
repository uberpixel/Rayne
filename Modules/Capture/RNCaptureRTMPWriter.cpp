//
//  RNCaptureRTMPWriter.cpp
//  Rayne-Capture
//
//  Copyright 2026 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#if defined(_WIN32)
// clang-format off
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#ifndef SECURITY_WIN32
		#define SECURITY_WIN32
	#endif
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>
	#include <security.h>
	#include <schannel.h>
// clang-format on
#endif

#include "RNCaptureRTMPWriter.h"

#if RN_PLATFORM_WINDOWS
	#ifndef far
		#define far
	#endif
	#ifndef near
		#define near
	#endif
#endif

#if RN_PLATFORM_ANDROID
	#define RN_CAPTURE_RTMP_OPENSSL_TLS_SUPPORTED 1
#else
	#define RN_CAPTURE_RTMP_OPENSSL_TLS_SUPPORTED 0
#endif

#if RN_PLATFORM_WINDOWS
	#define RN_CAPTURE_RTMP_SCHANNEL_TLS_SUPPORTED 1
#else
	#define RN_CAPTURE_RTMP_SCHANNEL_TLS_SUPPORTED 0
#endif

#if RN_CAPTURE_RTMP_OPENSSL_TLS_SUPPORTED || RN_CAPTURE_RTMP_SCHANNEL_TLS_SUPPORTED
	#define RN_CAPTURE_RTMP_TLS_SUPPORTED 1
#else
	#define RN_CAPTURE_RTMP_TLS_SUPPORTED 0
#endif

#if RN_PLATFORM_POSIX || RN_PLATFORM_WINDOWS
	#define RN_CAPTURE_RTMP_TRANSPORT_SUPPORTED 1
#else
	#define RN_CAPTURE_RTMP_TRANSPORT_SUPPORTED 0
#endif

#if RN_CAPTURE_RTMP_TRANSPORT_SUPPORTED
	#include <errno.h>
	#include <string.h>
	#include <time.h>

	#if RN_PLATFORM_POSIX
		#include <fcntl.h>
		#include <netdb.h>
		#include <sys/select.h>
		#include <sys/socket.h>
		#include <unistd.h>
	#endif

	#if RN_CAPTURE_RTMP_OPENSSL_TLS_SUPPORTED
		#include <openssl/pem.h>
		#include <openssl/ssl.h>
	#endif
#endif

namespace RN
{
	struct CaptureRTMPWriter::State
	{
		enum : uint8_t
		{
			MessageTypeSetChunkSize = 1,
			MessageTypeAudio = 8,
			MessageTypeVideo = 9,
			MessageTypeDataAMF0 = 18,
			MessageTypeCommandAMF0 = 20,
			MessageTypeCommandAMF3 = 17
		};

		enum : uint32_t
		{
			CommandChunkStreamID = 3,
			DataChunkStreamID = 5,
			AudioChunkStreamID = 6,
			VideoChunkStreamID = 7
		};

		enum : uint32_t
		{
			DefaultOutputChunkSize = 4096
		};

		enum PacketKind : uint8_t
		{
			PacketKindVideoSequenceHeader,
			PacketKindAudioSequenceHeader,
			PacketKindVideoKeyFrame,
			PacketKindVideoDelta,
			PacketKindAudioData
		};

		enum : size_t
		{
			SocketTransferLimit = 0x7fffffff,
			TLSReadBufferSize = 4096
		};

#if RN_PLATFORM_WINDOWS
		using SocketHandle = SOCKET;
		static constexpr SocketHandle InvalidSocket = INVALID_SOCKET;
#else
		using SocketHandle = int;
		static constexpr SocketHandle InvalidSocket = -1;
#endif

		struct URL
		{
			std::string host;
			std::string port;
			std::string app;
			std::string playPath;
			std::string tcURL;
			std::string redactedURL;
			bool useTLS = false;
		};

		struct Packet
		{
			uint8_t type;
			uint32_t timestamp;
			std::vector<uint8_t> payload;
			PacketKind kind = PacketKindVideoDelta;
		};

		struct InboundChunk
		{
			uint32_t timestamp = 0;
			uint32_t messageLength = 0;
			uint8_t messageType = 0;
			std::vector<uint8_t> payload;
		};

		struct Message
		{
			uint8_t type = 0;
			std::vector<uint8_t> payload;
		};

		State() :
			_socketFD(InvalidSocket),
#if RN_CAPTURE_RTMP_OPENSSL_TLS_SUPPORTED
			_sslContext(nullptr),
			_ssl(nullptr),
#endif
#if RN_CAPTURE_RTMP_SCHANNEL_TLS_SUPPORTED
			_securityCredentials(),
			_securityContext(),
			_tlsStreamSizes(),
			_tlsEncryptedBuffer(),
			_tlsDecryptedBuffer(),
			_hasSecurityCredentials(false),
			_hasSecurityContext(false),
#endif
			_outChunkSize(128),
			_inChunkSize(128),
			_streamID(1),
			_width(0),
			_height(0),
			_sampleRate(48000),
			_channelCount(2),
			_hasVideoFormat(false),
			_hasAudioFormat(false),
			_hasNetworkSession(false),
			_hasQueuedVideoSequenceHeader(false),
			_hasQueuedAudioSequenceHeader(false),
			_hasLoggedQueuePressure(false),
			_shouldStop(false),
			_workerThread(nullptr),
			_isWorkerRunning(false),
			_isReady(false)
		{}

		~State()
		{
			Stop();
		}

		bool Start(const CaptureSession::Configuration &configuration)
		{
			if(!configuration.streamURL)
			{
				RNWarning("RTMP stream URL is missing");
				return false;
			}

			_width = configuration.resolutionX;
			_height = configuration.resolutionY;
			_shouldStop = false;
			_isReady = false;
			_streamID = 1;
			_outChunkSize = 128;
			_inChunkSize = 128;
			_inboundChunks.clear();
			_packets.clear();
			_videoSequenceHeader.clear();
			_audioSequenceHeader.clear();
			_hasVideoFormat = false;
			_hasAudioFormat = false;
			_hasNetworkSession = false;
			_hasQueuedVideoSequenceHeader = false;
			_hasQueuedAudioSequenceHeader = false;
			_hasLoggedQueuePressure = false;
			_errorCallback = configuration.errorCallback;

			if(!ParseURL(configuration.streamURL->GetUTF8String(), _url))
			{
				RNWarning("Invalid RTMP stream URL");
				return false;
			}

			RNInfo("Starting stream to " << _url.redactedURL << " at " << _width << "x" << _height);
			_isWorkerRunning = true;
			_workerThread = new Thread([this] {
				AutoreleasePool pool;
				Run();
			},
									   false);
			_workerThread->SetName(RNCSTR("RN::RTMPWriter"));
			_workerThread->Start();

			return true;
		}

		void Stop()
		{
			_shouldStop = true;
			InterruptTransport();

			Thread *workerThread = _workerThread;
			_workerThread = nullptr;

			while(_isWorkerRunning.load())
			{
				SleepForMilliseconds(1);
			}

			if(workerThread) workerThread->Release();

			_lock.Lock();
			_packets.clear();
			_videoSequenceHeader.clear();
			_audioSequenceHeader.clear();
			_hasVideoFormat = false;
			_hasAudioFormat = false;
			_hasNetworkSession = false;
			_hasQueuedVideoSequenceHeader = false;
			_hasQueuedAudioSequenceHeader = false;
			_isReady = false;
			_errorCallback = nullptr;
			_lock.Unlock();
		}

		bool DidReceiveVideoFormat(const CaptureEncodedVideoFormat &format)
		{
			std::vector<uint8_t> sps;
			std::vector<uint8_t> pps;
			if(!ExtractVideoParameterSets(format, sps, pps))
			{
				RNWarning("RTMP writer did not receive H.264 SPS/PPS");
				return false;
			}

			std::vector<uint8_t> sequenceHeader;
			BuildVideoSequenceHeader(sps, pps, sequenceHeader);

			_lock.Lock();
			_width = format.width;
			_height = format.height;
			_videoSequenceHeader = sequenceHeader;
			_hasVideoFormat = true;
			UpdateReadinessLocked();
			_lock.Unlock();

			return true;
		}

		bool DidReceiveAudioFormat(const CaptureEncodedAudioFormat &format)
		{
			int32_t sampleRate = format.sampleRate > 0 ? format.sampleRate : 48000;
			int32_t channelCount = format.channelCount > 0 ? format.channelCount : 2;

			std::vector<uint8_t> config;
			if(format.audioSpecificConfig && format.audioSpecificConfigSize > 0)
			{
				config.assign(format.audioSpecificConfig, format.audioSpecificConfig + format.audioSpecificConfigSize);
			}
			else
			{
				BuildAACAudioSpecificConfig(sampleRate, channelCount, config);
			}

			std::vector<uint8_t> sequenceHeader;
			BuildAudioSequenceHeader(config, sequenceHeader);

			_lock.Lock();
			_sampleRate = sampleRate;
			_channelCount = channelCount;
			_audioSequenceHeader = sequenceHeader;
			_hasAudioFormat = true;
			UpdateReadinessLocked();
			_lock.Unlock();

			return true;
		}

		bool GetIsReady() const
		{
			return _isReady.load();
		}

		bool TestConnection(const char *urlString, String **errorMessage)
		{
			URL testURL;
			if(!ParseURL(urlString, testURL))
			{
				SetError(errorMessage, RNCSTR("Use an rtmp:// or rtmps:// URL with an app path and stream key."));
				return false;
			}

			_url = testURL;
			_shouldStop = false;

			bool result = OpenTransport(errorMessage);
			CloseTransport();
			return result;
		}

		bool TestPublishConnection(const char *urlString, String **errorMessage)
		{
			URL testURL;
			if(!ParseURL(urlString, testURL))
			{
				SetError(errorMessage, RNCSTR("Use an rtmp:// or rtmps:// URL with an app path and stream key."));
				return false;
			}

			_url = testURL;
			_shouldStop = false;

			bool result = OpenPublishSession(true, false, errorMessage);
			CloseTransport();
			return result;
		}

		void WriteVideoSample(const CaptureEncodedSample &sample)
		{
			if(!_isReady.load() || !sample.data || sample.size == 0 || sample.isCodecConfig) return;

			const uint8_t *sampleData = sample.data + sample.offset;
			size_t sampleSize = sample.size;
			bool isKeyFrame = IsKeyFrame(sampleData, sampleSize, sample.isKeyFrame);

			std::vector<uint8_t> videoPayload;
			BuildVideoPayload(sampleData, sampleSize, isKeyFrame, videoPayload);
			if(videoPayload.empty()) return;

			Packet packet;
			packet.type = MessageTypeVideo;
			packet.timestamp = static_cast<uint32_t>(sample.presentationTimeUs / 1000);
			packet.payload = videoPayload;
			packet.kind = isKeyFrame ? PacketKindVideoKeyFrame : PacketKindVideoDelta;
			QueuePacket(packet);
		}

		void WriteAudioSample(const CaptureEncodedSample &sample)
		{
			if(!_isReady.load() || !sample.data || sample.size == 0 || sample.isCodecConfig) return;

			Packet packet;
			packet.type = MessageTypeAudio;
			packet.timestamp = static_cast<uint32_t>(sample.presentationTimeUs / 1000);
			packet.payload.reserve(sample.size + 2);
			packet.payload.push_back(0xaf);
			packet.payload.push_back(1);
			packet.payload.insert(packet.payload.end(), sample.data + sample.offset, sample.data + sample.offset + sample.size);
			packet.kind = PacketKindAudioData;
			QueuePacket(packet);
		}

		void Run()
		{
			String *errorMessage = nullptr;
			bool didOpen = OpenSession(&errorMessage);
			if(didOpen)
			{
				_lock.Lock();
				_hasNetworkSession = true;
				UpdateReadinessLocked();
				_lock.Unlock();

				RunPacketLoop();
			}
			else if(!_shouldStop.load())
			{
				NotifyError(errorMessage ? errorMessage : RNCSTR("Could not start the RTMP stream."));
			}
			SafeRelease(errorMessage);

			_lock.Lock();
			_hasNetworkSession = false;
			_isReady = false;
			_lock.Unlock();

			CloseTransport();
			_isWorkerRunning = false;
		}

		bool OpenSession(String **errorMessage)
		{
			return OpenPublishSession(true, true, errorMessage);
		}

		bool OpenPublishSession(bool waitForPublishResponse, bool sendMetadata, String **errorMessage)
		{
			if(!OpenTransport(errorMessage)) return false;
			if(!PerformHandshake())
			{
				SetErrorIfMissing(errorMessage, RNCSTR("RTMP handshake failed."));
				return false;
			}

			if(!SendSetChunkSize(DefaultOutputChunkSize))
			{
				SetErrorIfMissing(errorMessage, RNCSTR("Could not configure the RTMP stream."));
				return false;
			}
			if(!SendConnect())
			{
				SetErrorIfMissing(errorMessage, RNCSTR("Could not send the RTMP connect request."));
				return false;
			}
			if(!WaitForCommandResponse(1, nullptr, errorMessage)) return false;

			if(!SendCommand("releaseStream", 2, 0, &_url.playPath, nullptr))
			{
				SetErrorIfMissing(errorMessage, RNCSTR("Could not reserve the RTMP stream."));
				return false;
			}
			if(!SendCommand("FCPublish", 3, 0, &_url.playPath, nullptr))
			{
				SetErrorIfMissing(errorMessage, RNCSTR("Could not prepare the RTMP publish request."));
				return false;
			}
			if(!SendCommand("createStream", 4, 0, nullptr, nullptr))
			{
				SetErrorIfMissing(errorMessage, RNCSTR("Could not create the RTMP stream."));
				return false;
			}
			if(!WaitForCommandResponse(4, &_streamID, errorMessage)) return false;
			if(!SendCommand("publish", 0, _streamID, &_url.playPath, "live"))
			{
				SetErrorIfMissing(errorMessage, RNCSTR("Could not send the RTMP publish request."));
				return false;
			}
			if(waitForPublishResponse && !WaitForPublishResponse(errorMessage)) return false;
			if(sendMetadata && !SendMetadata())
			{
				SetErrorIfMissing(errorMessage, RNCSTR("Could not send RTMP stream metadata."));
				return false;
			}

			RNInfo("Recording RTMP publish session opened");
			return true;
		}

		void NotifyError(const String *message)
		{
			if(!_errorCallback) return;

			std::function<void(const String *)> errorCallback = _errorCallback;
			String *messageCopy = RNSTR("" << (message ? message->GetUTF8String() : "Could not start the live stream."))->Retain();
			WorkGroup *group = new WorkGroup();
			group->Perform(WorkQueue::GetMainQueue(), [errorCallback, messageCopy] {
				AutoreleasePool pool;
				errorCallback(messageCopy);
				messageCopy->Release();
			});
			group->Release();
		}

		void RunPacketLoop()
		{
			while(!_shouldStop.load())
			{
				Packet packet;
				if(!TakePacket(packet))
				{
					SleepForMilliseconds(10);
					continue;
				}

				uint32_t chunkStreamID = packet.type == MessageTypeAudio ? AudioChunkStreamID : VideoChunkStreamID;
				if(!SendMessage(chunkStreamID, packet.type, _streamID, packet.timestamp, packet.payload.data(), packet.payload.size()))
				{
					RNWarning("RTMP writer failed while sending media packet");
					NotifyError(RNCSTR("RTMP connection was lost while streaming."));
					_shouldStop = true;
				}
			}
		}

		void QueuePacket(const Packet &packet)
		{
			_lock.Lock();
			QueuePacketLocked(packet);
			_lock.Unlock();
		}

		void QueuePacketLocked(const Packet &packet)
		{
			if(_packets.size() > 180)
			{
				if(!_hasLoggedQueuePressure)
				{
					RNWarning("RTMP writer is falling behind, dropping queued packets");
					_hasLoggedQueuePressure = true;
				}

				DropQueuedPacketsForBackpressureLocked();
			}

			_packets.push_back(packet);
		}

		bool DropOldestQueuedPacketWithKindLocked(PacketKind kind)
		{
			for(std::deque<Packet>::iterator iterator = _packets.begin(); iterator != _packets.end(); ++iterator)
			{
				if(iterator->kind == kind)
				{
					_packets.erase(iterator);
					return true;
				}
			}

			return false;
		}

		void DropQueuedPacketsForBackpressureLocked()
		{
			while(_packets.size() > 120 && DropOldestQueuedPacketWithKindLocked(PacketKindVideoDelta)) {}
			while(_packets.size() > 120 && DropOldestQueuedPacketWithKindLocked(PacketKindAudioData)) {}
			while(_packets.size() > 120 && DropOldestQueuedPacketWithKindLocked(PacketKindVideoKeyFrame)) {}
		}

		bool TakePacket(Packet &packet)
		{
			bool hasPacket = false;

			_lock.Lock();
			if(!_packets.empty())
			{
				packet = _packets.front();
				_packets.pop_front();
				hasPacket = true;
			}
			_lock.Unlock();

			return hasPacket;
		}

		void UpdateReadinessLocked()
		{
			bool ready = _hasNetworkSession && _hasVideoFormat && _hasAudioFormat;
			if(ready && !_isReady.load())
			{
				QueueSequenceHeadersLocked(0);
			}

			_isReady = ready;
		}

		void QueueSequenceHeadersLocked(uint32_t timestamp)
		{
			if(!_hasQueuedVideoSequenceHeader && !_videoSequenceHeader.empty())
			{
				Packet packet;
				packet.type = MessageTypeVideo;
				packet.timestamp = timestamp;
				packet.payload = _videoSequenceHeader;
				packet.kind = PacketKindVideoSequenceHeader;
				QueuePacketLocked(packet);
				_hasQueuedVideoSequenceHeader = true;
			}

			if(!_hasQueuedAudioSequenceHeader && !_audioSequenceHeader.empty())
			{
				Packet packet;
				packet.type = MessageTypeAudio;
				packet.timestamp = timestamp;
				packet.payload = _audioSequenceHeader;
				packet.kind = PacketKindAudioSequenceHeader;
				QueuePacketLocked(packet);
				_hasQueuedAudioSequenceHeader = true;
			}
		}

		static bool ParseURL(const char *urlString, URL &url)
		{
			if(!urlString) return false;
			url = URL();

			std::string value(urlString);
			size_t schemeEnd = value.find("://");
			if(schemeEnd == std::string::npos) return false;

			std::string scheme = value.substr(0, schemeEnd);
			url.useTLS = scheme == "rtmps";
			if(scheme != "rtmp" && scheme != "rtmps") return false;

			size_t authorityStart = schemeEnd + 3;
			size_t pathStart = value.find('/', authorityStart);
			if(pathStart == std::string::npos || pathStart + 1 >= value.length()) return false;

			std::string authority = value.substr(authorityStart, pathStart - authorityStart);
			if(authority.empty()) return false;

			size_t portStart = authority.rfind(':');
			if(portStart != std::string::npos)
			{
				url.host = authority.substr(0, portStart);
				url.port = authority.substr(portStart + 1);
			}
			else
			{
				url.host = authority;
			}

			if(url.host.empty()) return false;
			if(url.port.empty()) url.port = url.useTLS ? "443" : "1935";

			std::string path = value.substr(pathStart + 1);
			size_t appEnd = path.find('/');
			if(appEnd == std::string::npos)
			{
				url.playPath = path;
				url.tcURL = scheme + "://" + url.host;
				if((!url.useTLS && url.port != "1935") || (url.useTLS && url.port != "443")) url.tcURL += ":" + url.port;
				url.redactedURL = url.tcURL + "/<stream>";
				return !url.playPath.empty();
			}
			if(appEnd + 1 >= path.length()) return false;

			url.app = path.substr(0, appEnd);
			url.playPath = path.substr(appEnd + 1);
			if(url.app.empty() || url.playPath.empty()) return false;

			url.tcURL = scheme + "://" + url.host;
			if((!url.useTLS && url.port != "1935") || (url.useTLS && url.port != "443")) url.tcURL += ":" + url.port;
			url.tcURL += "/" + url.app;
			url.redactedURL = url.tcURL + "/<stream>";
			return true;
		}

		bool OpenTransport(String **errorMessage = nullptr)
		{
#if !RN_CAPTURE_RTMP_TRANSPORT_SUPPORTED
			SetError(errorMessage, RNCSTR("RTMP streaming is not supported on this platform yet."));
			return false;
#else
			if(!EnsureSocketSystem())
			{
				SetError(errorMessage, RNCSTR("Could not initialize the RTMP socket system."));
				return false;
			}
			if(_url.useTLS && !GetIsTLSSupported())
			{
				SetError(errorMessage, RNCSTR("RTMPS is not supported on this platform yet."));
				return false;
			}

			struct addrinfo hints;
			memset(&hints, 0, sizeof(hints));
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_family = AF_UNSPEC;

			struct addrinfo *addresses = nullptr;
			int result = getaddrinfo(_url.host.c_str(), _url.port.c_str(), &hints, &addresses);
			if(result != 0)
			{
				RNWarning("RTMP DNS lookup failed: " << GetAddressInfoErrorString(result));
				SetError(errorMessage, RNSTR("Could not resolve RTMP host: " << GetAddressInfoErrorString(result)));
				return false;
			}

			for(struct addrinfo *address = addresses; address; address = address->ai_next)
			{
				if(_shouldStop.load()) break;

				_socketFD = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
				if(_socketFD == InvalidSocket) continue;

				ConfigureSocketTimeouts();
				if(ConnectSocket(address))
				{
					break;
				}

				CloseSocket(_socketFD);
				_socketFD = InvalidSocket;
			}

			freeaddrinfo(addresses);

			if(_socketFD == InvalidSocket)
			{
				RNWarning("RTMP TCP connection failed");
				SetError(errorMessage, RNCSTR("Could not connect to the RTMP server."));
				return false;
			}

			if(_url.useTLS && !StartTLS(errorMessage))
			{
				return false;
			}

			return true;
#endif
		}

		bool ConnectSocket(struct addrinfo *address)
		{
#if RN_PLATFORM_WINDOWS
			u_long nonBlocking = 1;
			ioctlsocket(_socketFD, FIONBIO, &nonBlocking);

			int result = connect(_socketFD, address->ai_addr, static_cast<int>(address->ai_addrlen));
			if(result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
			{
				nonBlocking = 0;
				ioctlsocket(_socketFD, FIONBIO, &nonBlocking);
				return false;
			}

			if(result == SOCKET_ERROR)
			{
				fd_set writeSet;
				FD_ZERO(&writeSet);
				FD_SET(_socketFD, &writeSet);

				struct timeval timeout;
				timeout.tv_sec = 10;
				timeout.tv_usec = 0;

				result = select(0, nullptr, &writeSet, nullptr, &timeout);
				if(result <= 0)
				{
					nonBlocking = 0;
					ioctlsocket(_socketFD, FIONBIO, &nonBlocking);
					return false;
				}

				int error = 0;
				int errorLength = sizeof(error);
				if(getsockopt(_socketFD, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&error), &errorLength) != 0 || error != 0)
				{
					nonBlocking = 0;
					ioctlsocket(_socketFD, FIONBIO, &nonBlocking);
					return false;
				}
			}

			nonBlocking = 0;
			ioctlsocket(_socketFD, FIONBIO, &nonBlocking);
			return true;
#elif RN_PLATFORM_POSIX
			int flags = fcntl(_socketFD, F_GETFL, 0);
			if(flags >= 0) fcntl(_socketFD, F_SETFL, flags | O_NONBLOCK);

			int result = connect(_socketFD, address->ai_addr, address->ai_addrlen);
			if(result == -1 && errno != EINPROGRESS)
			{
				if(flags >= 0) fcntl(_socketFD, F_SETFL, flags);
				return false;
			}

			if(result == -1)
			{
				fd_set writeSet;
				FD_ZERO(&writeSet);
				FD_SET(_socketFD, &writeSet);

				struct timeval timeout;
				timeout.tv_sec = 10;
				timeout.tv_usec = 0;

				result = select(_socketFD + 1, nullptr, &writeSet, nullptr, &timeout);
				if(result <= 0)
				{
					if(flags >= 0) fcntl(_socketFD, F_SETFL, flags);
					return false;
				}

				int error = 0;
				socklen_t errorLength = sizeof(error);
				if(getsockopt(_socketFD, SOL_SOCKET, SO_ERROR, &error, &errorLength) != 0 || error != 0)
				{
					if(flags >= 0) fcntl(_socketFD, F_SETFL, flags);
					return false;
				}
			}

			if(flags >= 0) fcntl(_socketFD, F_SETFL, flags);
			return true;
#else
			(void)address;
			return false;
#endif
		}

		void ConfigureSocketTimeouts()
		{
#if RN_PLATFORM_WINDOWS
			DWORD timeout = 5000;
			setsockopt(_socketFD, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout));
			setsockopt(_socketFD, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char *>(&timeout), sizeof(timeout));
#elif RN_PLATFORM_POSIX
			struct timeval timeout;
			timeout.tv_sec = 5;
			timeout.tv_usec = 0;
			setsockopt(_socketFD, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
			setsockopt(_socketFD, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
#endif
		}

		bool StartTLS(String **errorMessage = nullptr)
		{
#if RN_CAPTURE_RTMP_OPENSSL_TLS_SUPPORTED
			_sslContext = SSL_CTX_new(TLS_client_method());
			if(!_sslContext)
			{
				RNWarning("RTMPS SSL context creation failed");
				SetError(errorMessage, RNCSTR("Could not create RTMPS TLS context."));
				return false;
			}

			SSL_CTX_set_verify(_sslContext, SSL_VERIFY_PEER, nullptr);
			Data *caData = Data::WithContentsOfFile(RNCSTR("cacert.pem"));
			bool loadedCertificate = false;
			BIO *certificateBuffer = caData ? BIO_new_mem_buf(caData->GetBytes<char>(), static_cast<int>(caData->GetLength())) : nullptr;
			if(certificateBuffer)
			{
				while(X509 *certificate = PEM_read_bio_X509(certificateBuffer, nullptr, nullptr, nullptr))
				{
					if(X509_STORE_add_cert(SSL_CTX_get_cert_store(_sslContext), certificate) == 1) loadedCertificate = true;
					X509_free(certificate);
				}
				BIO_free(certificateBuffer);
			}
			if(!loadedCertificate) SSL_CTX_set_default_verify_paths(_sslContext);

			_ssl = SSL_new(_sslContext);
			if(!_ssl)
			{
				RNWarning("RTMPS SSL session creation failed");
				SetError(errorMessage, RNCSTR("Could not create RTMPS TLS session."));
				return false;
			}

			SSL_set_tlsext_host_name(_ssl, _url.host.c_str());
			X509_VERIFY_PARAM_set1_host(SSL_get0_param(_ssl), _url.host.c_str(), 0);
			SSL_set_fd(_ssl, _socketFD);

			if(SSL_connect(_ssl) <= 0)
			{
				RNWarning("RTMPS handshake failed");
				SetError(errorMessage, RNCSTR("RTMPS TLS handshake failed."));
				return false;
			}

			if(SSL_get_verify_result(_ssl) != X509_V_OK)
			{
				RNWarning("RTMPS certificate verification failed");
				SetError(errorMessage, RNCSTR("RTMPS certificate verification failed."));
				return false;
			}

			return true;
#elif RN_CAPTURE_RTMP_SCHANNEL_TLS_SUPPORTED
			return StartSchannelTLS(errorMessage);
#else
			SetError(errorMessage, RNCSTR("RTMPS is not supported on this platform yet."));
			return false;
#endif
		}

		static void SetError(String **errorMessage, const String *message)
		{
			if(errorMessage)
			{
				SafeRelease(*errorMessage);
				*errorMessage = message ? RNSTR(message->GetUTF8String())->Retain() : nullptr;
			}
		}

		static void SetErrorIfMissing(String **errorMessage, const String *message)
		{
			if(!errorMessage || *errorMessage) return;
			SetError(errorMessage, message);
		}

		void InterruptTransport()
		{
			if(_socketFD != InvalidSocket) ShutdownSocket(_socketFD);
		}

		void CloseTransport()
		{
#if RN_CAPTURE_RTMP_OPENSSL_TLS_SUPPORTED
			if(_ssl)
			{
				// The stream may already have failed or been interrupted; avoid re-entering OpenSSL I/O during teardown.
				SSL_free(_ssl);
				_ssl = nullptr;
			}

			if(_sslContext)
			{
				SSL_CTX_free(_sslContext);
				_sslContext = nullptr;
			}
#endif
#if RN_CAPTURE_RTMP_SCHANNEL_TLS_SUPPORTED
			if(_hasSecurityContext)
			{
				DeleteSecurityContext(&_securityContext);
				_hasSecurityContext = false;
			}

			if(_hasSecurityCredentials)
			{
				FreeCredentialsHandle(&_securityCredentials);
				_hasSecurityCredentials = false;
			}

			_tlsEncryptedBuffer.clear();
			_tlsDecryptedBuffer.clear();
#endif

			if(_socketFD != InvalidSocket)
			{
				CloseSocket(_socketFD);
				_socketFD = InvalidSocket;
			}
		}

		static bool EnsureSocketSystem()
		{
#if RN_PLATFORM_WINDOWS
			static bool didInitialize = false;
			if(didInitialize) return true;

			WSADATA data;
			if(WSAStartup(MAKEWORD(2, 2), &data) != 0) return false;
			didInitialize = true;
#endif
			return true;
		}

		static const char *GetAddressInfoErrorString(int error)
		{
#if RN_PLATFORM_WINDOWS
			return gai_strerrorA(error);
#elif RN_PLATFORM_POSIX
			return gai_strerror(error);
#else
			(void)error;
			return "unsupported platform";
#endif
		}

		static bool GetIsTLSSupported()
		{
			return RN_CAPTURE_RTMP_TLS_SUPPORTED != 0;
		}

		static void CloseSocket(SocketHandle socket)
		{
#if RN_PLATFORM_WINDOWS
			closesocket(socket);
#elif RN_PLATFORM_POSIX
			close(socket);
#else
			(void)socket;
#endif
		}

		static void ShutdownSocket(SocketHandle socket)
		{
#if RN_PLATFORM_WINDOWS
			shutdown(socket, SD_BOTH);
#elif RN_PLATFORM_POSIX
			shutdown(socket, SHUT_RDWR);
#else
			(void)socket;
#endif
		}

		int ReadSocket(uint8_t *buffer, size_t size)
		{
#if RN_PLATFORM_WINDOWS
			size_t readSize = size > SocketTransferLimit ? SocketTransferLimit : size;
			return recv(_socketFD, reinterpret_cast<char *>(buffer), static_cast<int>(readSize), 0);
#elif RN_PLATFORM_POSIX
			size_t readSize = size > SocketTransferLimit ? SocketTransferLimit : size;
			ssize_t result = recv(_socketFD, buffer, readSize, 0);
			return static_cast<int>(result);
#else
			(void)buffer;
			(void)size;
			return -1;
#endif
		}

		int WriteSocket(const uint8_t *buffer, size_t size)
		{
#if RN_PLATFORM_WINDOWS
			size_t writeSize = size > SocketTransferLimit ? SocketTransferLimit : size;
			return send(_socketFD, reinterpret_cast<const char *>(buffer), static_cast<int>(writeSize), 0);
#elif RN_PLATFORM_POSIX
			size_t writeSize = size > SocketTransferLimit ? SocketTransferLimit : size;
			ssize_t result = send(_socketFD, buffer, writeSize, 0);
			return static_cast<int>(result);
#else
			(void)buffer;
			(void)size;
			return -1;
#endif
		}

		bool WriteSocketAll(const uint8_t *buffer, size_t size)
		{
			size_t offset = 0;
			while(offset < size && !_shouldStop.load())
			{
				int result = WriteSocket(buffer + offset, size - offset);
				if(result <= 0) return false;
				offset += static_cast<size_t>(result);
			}

			return offset == size;
		}

#if RN_CAPTURE_RTMP_SCHANNEL_TLS_SUPPORTED
		static DWORD GetSchannelContextRequirements()
		{
			return ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_REQ_EXTENDED_ERROR | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;
		}

		bool StartSchannelTLS(String **errorMessage)
		{
			SCHANNEL_CRED credentials;
			memset(&credentials, 0, sizeof(credentials));
			credentials.dwVersion = SCHANNEL_CRED_VERSION;
	#ifdef SCH_USE_STRONG_CRYPTO
			credentials.dwFlags |= SCH_USE_STRONG_CRYPTO;
	#endif

			TimeStamp credentialExpiry;
			SECURITY_STATUS status = AcquireCredentialsHandleA(nullptr, const_cast<SEC_CHAR *>(UNISP_NAME_A), SECPKG_CRED_OUTBOUND, nullptr, &credentials, nullptr, nullptr, &_securityCredentials, &credentialExpiry);
			if(status != SEC_E_OK)
			{
				RNWarning("RTMPS Schannel credential creation failed: " << status);
				SetError(errorMessage, RNCSTR("Could not create RTMPS TLS credentials."));
				return false;
			}
			_hasSecurityCredentials = true;

			DWORD contextAttributes = 0;
			TimeStamp contextExpiry;
			while(!_shouldStop.load())
			{
				SecBuffer outputBuffer;
				outputBuffer.BufferType = SECBUFFER_TOKEN;
				outputBuffer.cbBuffer = 0;
				outputBuffer.pvBuffer = nullptr;

				SecBufferDesc outputDescription;
				outputDescription.ulVersion = SECBUFFER_VERSION;
				outputDescription.cBuffers = 1;
				outputDescription.pBuffers = &outputBuffer;

				SecBuffer inputBuffers[2];
				SecBufferDesc inputDescription;
				SecBufferDesc *inputDescriptionPointer = nullptr;
				if(!_tlsEncryptedBuffer.empty())
				{
					inputBuffers[0].BufferType = SECBUFFER_TOKEN;
					inputBuffers[0].cbBuffer = static_cast<unsigned long>(_tlsEncryptedBuffer.size());
					inputBuffers[0].pvBuffer = _tlsEncryptedBuffer.data();
					inputBuffers[1].BufferType = SECBUFFER_EMPTY;
					inputBuffers[1].cbBuffer = 0;
					inputBuffers[1].pvBuffer = nullptr;

					inputDescription.ulVersion = SECBUFFER_VERSION;
					inputDescription.cBuffers = 2;
					inputDescription.pBuffers = inputBuffers;
					inputDescriptionPointer = &inputDescription;
				}

				status = InitializeSecurityContextA(&_securityCredentials, _hasSecurityContext ? &_securityContext : nullptr, const_cast<SEC_CHAR *>(_url.host.c_str()), GetSchannelContextRequirements(), 0, SECURITY_NATIVE_DREP, inputDescriptionPointer, 0, &_securityContext, &outputDescription, &contextAttributes, &contextExpiry);
				if(status == SEC_E_OK || status == SEC_I_CONTINUE_NEEDED)
				{
					_hasSecurityContext = true;
				}

				if(outputBuffer.cbBuffer > 0 && outputBuffer.pvBuffer)
				{
					bool didWrite = WriteSocketAll(static_cast<const uint8_t *>(outputBuffer.pvBuffer), outputBuffer.cbBuffer);
					FreeContextBuffer(outputBuffer.pvBuffer);
					if(!didWrite)
					{
						SetError(errorMessage, RNCSTR("Could not send the RTMPS TLS handshake."));
						return false;
					}
				}

				if(status == SEC_E_OK)
				{
					if(inputDescriptionPointer) PreserveSchannelExtraEncryptedData(inputBuffers, 2);

					status = QueryContextAttributes(&_securityContext, SECPKG_ATTR_STREAM_SIZES, &_tlsStreamSizes);
					if(status != SEC_E_OK)
					{
						RNWarning("RTMPS Schannel stream size query failed: " << status);
						SetError(errorMessage, RNCSTR("Could not configure the RTMPS TLS stream."));
						return false;
					}

					return true;
				}

				if(status == SEC_E_INCOMPLETE_MESSAGE)
				{
					if(!ReadSocketIntoTLSEncryptedBuffer(errorMessage)) return false;
					continue;
				}

				if(status == SEC_I_CONTINUE_NEEDED)
				{
					if(inputDescriptionPointer) PreserveSchannelExtraEncryptedData(inputBuffers, 2);
					if(_tlsEncryptedBuffer.empty() && !ReadSocketIntoTLSEncryptedBuffer(errorMessage)) return false;
					continue;
				}

				RNWarning("RTMPS Schannel handshake failed: " << status);
				SetError(errorMessage, RNCSTR("RTMPS TLS handshake failed."));
				return false;
			}

			SetError(errorMessage, RNCSTR("RTMPS TLS handshake was interrupted."));
			return false;
		}

		bool ReadSocketIntoTLSEncryptedBuffer(String **errorMessage)
		{
			uint8_t buffer[TLSReadBufferSize];
			int result = ReadSocket(buffer, sizeof(buffer));
			if(result <= 0)
			{
				SetErrorIfMissing(errorMessage, RNCSTR("Could not read the RTMPS TLS handshake."));
				return false;
			}

			_tlsEncryptedBuffer.insert(_tlsEncryptedBuffer.end(), buffer, buffer + result);
			return true;
		}

		void PreserveSchannelExtraEncryptedData(SecBuffer *buffers, size_t count)
		{
			for(size_t i = 0; i < count; i++)
			{
				if(buffers[i].BufferType == SECBUFFER_EXTRA && buffers[i].cbBuffer > 0)
				{
					uint8_t *extraData = static_cast<uint8_t *>(buffers[i].pvBuffer);
					if(extraData)
					{
						std::vector<uint8_t> extraBuffer;
						extraBuffer.assign(extraData, extraData + buffers[i].cbBuffer);
						_tlsEncryptedBuffer = extraBuffer;
					}
					else if(_tlsEncryptedBuffer.size() >= buffers[i].cbBuffer)
					{
						_tlsEncryptedBuffer.erase(_tlsEncryptedBuffer.begin(), _tlsEncryptedBuffer.end() - buffers[i].cbBuffer);
					}
					else
					{
						_tlsEncryptedBuffer.clear();
					}
					return;
				}
			}

			_tlsEncryptedBuffer.clear();
		}

		int ConsumeSchannelPlaintext(uint8_t *buffer, size_t size)
		{
			if(_tlsDecryptedBuffer.empty()) return 0;

			size_t copySize = _tlsDecryptedBuffer.size() < size ? _tlsDecryptedBuffer.size() : size;
			memcpy(buffer, _tlsDecryptedBuffer.data(), copySize);
			_tlsDecryptedBuffer.erase(_tlsDecryptedBuffer.begin(), _tlsDecryptedBuffer.begin() + copySize);
			return static_cast<int>(copySize);
		}

		int ReadSchannelTransport(uint8_t *buffer, size_t size)
		{
			if(size == 0) return 0;

			int bufferedResult = ConsumeSchannelPlaintext(buffer, size);
			if(bufferedResult > 0) return bufferedResult;

			while(!_shouldStop.load())
			{
				if(_tlsEncryptedBuffer.empty())
				{
					uint8_t rawBuffer[TLSReadBufferSize];
					int rawResult = ReadSocket(rawBuffer, sizeof(rawBuffer));
					if(rawResult <= 0) return rawResult;
					_tlsEncryptedBuffer.insert(_tlsEncryptedBuffer.end(), rawBuffer, rawBuffer + rawResult);
				}

				SecBuffer buffers[4];
				buffers[0].BufferType = SECBUFFER_DATA;
				buffers[0].cbBuffer = static_cast<unsigned long>(_tlsEncryptedBuffer.size());
				buffers[0].pvBuffer = _tlsEncryptedBuffer.data();
				for(size_t i = 1; i < 4; i++)
				{
					buffers[i].BufferType = SECBUFFER_EMPTY;
					buffers[i].cbBuffer = 0;
					buffers[i].pvBuffer = nullptr;
				}

				SecBufferDesc bufferDescription;
				bufferDescription.ulVersion = SECBUFFER_VERSION;
				bufferDescription.cBuffers = 4;
				bufferDescription.pBuffers = buffers;

				SECURITY_STATUS status = DecryptMessage(&_securityContext, &bufferDescription, 0, nullptr);
				if(status == SEC_E_INCOMPLETE_MESSAGE)
				{
					uint8_t rawBuffer[TLSReadBufferSize];
					int rawResult = ReadSocket(rawBuffer, sizeof(rawBuffer));
					if(rawResult <= 0) return rawResult;
					_tlsEncryptedBuffer.insert(_tlsEncryptedBuffer.end(), rawBuffer, rawBuffer + rawResult);
					continue;
				}
				if(status == SEC_I_CONTEXT_EXPIRED) return 0;
				if(status != SEC_E_OK)
				{
					RNWarning("RTMPS Schannel decrypt failed: " << status);
					return -1;
				}

				SecBuffer *dataBuffer = nullptr;
				std::vector<uint8_t> extraBuffer;
				for(size_t i = 0; i < 4; i++)
				{
					if(buffers[i].BufferType == SECBUFFER_DATA && buffers[i].cbBuffer > 0)
					{
						dataBuffer = &buffers[i];
					}
					else if(buffers[i].BufferType == SECBUFFER_EXTRA && buffers[i].cbBuffer > 0)
					{
						uint8_t *extraData = static_cast<uint8_t *>(buffers[i].pvBuffer);
						if(extraData)
						{
							extraBuffer.assign(extraData, extraData + buffers[i].cbBuffer);
						}
						else if(_tlsEncryptedBuffer.size() >= buffers[i].cbBuffer)
						{
							extraBuffer.assign(_tlsEncryptedBuffer.end() - buffers[i].cbBuffer, _tlsEncryptedBuffer.end());
						}
					}
				}

				int result = 0;
				if(dataBuffer && dataBuffer->pvBuffer)
				{
					size_t copySize = dataBuffer->cbBuffer < size ? dataBuffer->cbBuffer : size;
					memcpy(buffer, dataBuffer->pvBuffer, copySize);
					result = static_cast<int>(copySize);

					if(copySize < dataBuffer->cbBuffer)
					{
						uint8_t *plainData = static_cast<uint8_t *>(dataBuffer->pvBuffer);
						_tlsDecryptedBuffer.insert(_tlsDecryptedBuffer.end(), plainData + copySize, plainData + dataBuffer->cbBuffer);
					}
				}

				_tlsEncryptedBuffer = extraBuffer;
				if(result > 0) return result;
			}

			return -1;
		}

		int WriteSchannelTransport(const uint8_t *buffer, size_t size)
		{
			if(size == 0) return 0;

			size_t maximumMessageSize = _tlsStreamSizes.cbMaximumMessage > 0 ? _tlsStreamSizes.cbMaximumMessage : size;
			size_t messageSize = size < maximumMessageSize ? size : maximumMessageSize;
			size_t packetSize = _tlsStreamSizes.cbHeader + messageSize + _tlsStreamSizes.cbTrailer;
			std::vector<uint8_t> packet(packetSize);
			memcpy(packet.data() + _tlsStreamSizes.cbHeader, buffer, messageSize);

			SecBuffer buffers[4];
			buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
			buffers[0].cbBuffer = _tlsStreamSizes.cbHeader;
			buffers[0].pvBuffer = packet.data();
			buffers[1].BufferType = SECBUFFER_DATA;
			buffers[1].cbBuffer = static_cast<unsigned long>(messageSize);
			buffers[1].pvBuffer = packet.data() + _tlsStreamSizes.cbHeader;
			buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
			buffers[2].cbBuffer = _tlsStreamSizes.cbTrailer;
			buffers[2].pvBuffer = packet.data() + _tlsStreamSizes.cbHeader + messageSize;
			buffers[3].BufferType = SECBUFFER_EMPTY;
			buffers[3].cbBuffer = 0;
			buffers[3].pvBuffer = nullptr;

			SecBufferDesc bufferDescription;
			bufferDescription.ulVersion = SECBUFFER_VERSION;
			bufferDescription.cBuffers = 4;
			bufferDescription.pBuffers = buffers;

			SECURITY_STATUS status = EncryptMessage(&_securityContext, 0, &bufferDescription, 0);
			if(status != SEC_E_OK)
			{
				RNWarning("RTMPS Schannel encrypt failed: " << status);
				return -1;
			}

			for(size_t i = 0; i < 3; i++)
			{
				if(buffers[i].cbBuffer == 0) continue;
				if(!buffers[i].pvBuffer || !WriteSocketAll(static_cast<const uint8_t *>(buffers[i].pvBuffer), buffers[i].cbBuffer))
				{
					return -1;
				}
			}

			return static_cast<int>(messageSize);
		}
#endif

		int ReadTransport(uint8_t *buffer, size_t size)
		{
#if RN_CAPTURE_RTMP_OPENSSL_TLS_SUPPORTED
			if(_ssl) return SSL_read(_ssl, buffer, static_cast<int>(size));
#endif
#if RN_CAPTURE_RTMP_SCHANNEL_TLS_SUPPORTED
			if(_hasSecurityContext) return ReadSchannelTransport(buffer, size);
#endif
			return ReadSocket(buffer, size);
		}

		int WriteTransport(const uint8_t *buffer, size_t size)
		{
#if RN_CAPTURE_RTMP_OPENSSL_TLS_SUPPORTED
			if(_ssl) return SSL_write(_ssl, buffer, static_cast<int>(size));
#endif
#if RN_CAPTURE_RTMP_SCHANNEL_TLS_SUPPORTED
			if(_hasSecurityContext) return WriteSchannelTransport(buffer, size);
#endif
			return WriteSocket(buffer, size);
		}

		bool PerformHandshake()
		{
			std::vector<uint8_t> c0c1(1537);
			c0c1[0] = 3;
			WriteU32BE(c0c1, 1, static_cast<uint32_t>(GetMonotonicTimeMs()));
			for(size_t i = 9; i < c0c1.size(); i++)
			{
				c0c1[i] = static_cast<uint8_t>((i * 37) & 0xff);
			}

			if(!WriteAll(c0c1.data(), c0c1.size())) return false;

			std::vector<uint8_t> s0s1s2(3073);
			if(!ReadFully(s0s1s2.data(), s0s1s2.size())) return false;
			if(s0s1s2[0] != 3)
			{
				RNWarning("RTMP server returned unsupported handshake version " << static_cast<int>(s0s1s2[0]));
				return false;
			}

			return WriteAll(s0s1s2.data() + 1, 1536);
		}

		bool ReadFully(uint8_t *buffer, size_t size)
		{
			size_t offset = 0;
			while(offset < size && !_shouldStop.load())
			{
				int result = ReadTransport(buffer + offset, size - offset);
				if(result <= 0) return false;
				offset += static_cast<size_t>(result);
			}

			return offset == size;
		}

		bool WriteAll(const uint8_t *buffer, size_t size)
		{
			size_t offset = 0;
			while(offset < size && !_shouldStop.load())
			{
				int result = WriteTransport(buffer + offset, size - offset);
				if(result <= 0) return false;
				offset += static_cast<size_t>(result);
			}

			return offset == size;
		}

		bool SendSetChunkSize(uint32_t chunkSize)
		{
			std::vector<uint8_t> payload;
			AppendU32BE(payload, chunkSize);
			bool result = SendMessage(2, MessageTypeSetChunkSize, 0, 0, payload.data(), payload.size());
			if(result) _outChunkSize = chunkSize;
			return result;
		}

		bool SendConnect()
		{
			std::vector<uint8_t> payload;
			AppendAMF0String(payload, "connect");
			AppendAMF0Number(payload, 1);
			AppendAMF0ObjectStart(payload);
			AppendAMF0PropertyString(payload, "app", _url.app);
			AppendAMF0PropertyString(payload, "type", "nonprivate");
			AppendAMF0PropertyString(payload, "flashVer", "FMLE/3.0");
			AppendAMF0PropertyString(payload, "tcUrl", _url.tcURL);
			AppendAMF0PropertyBoolean(payload, "fpad", false);
			AppendAMF0PropertyNumber(payload, "capabilities", 15);
			AppendAMF0PropertyNumber(payload, "audioCodecs", 3575);
			AppendAMF0PropertyNumber(payload, "videoCodecs", 252);
			AppendAMF0PropertyNumber(payload, "videoFunction", 1);
			AppendAMF0ObjectEnd(payload);

			return SendMessage(CommandChunkStreamID, MessageTypeCommandAMF0, 0, 0, payload.data(), payload.size());
		}

		bool SendCommand(const char *command, double transactionID, uint32_t messageStreamID, const std::string *streamName, const char *trailingString)
		{
			std::vector<uint8_t> payload;
			AppendAMF0String(payload, command);
			AppendAMF0Number(payload, transactionID);
			AppendAMF0Null(payload);
			if(streamName) AppendAMF0String(payload, *streamName);
			if(trailingString) AppendAMF0String(payload, trailingString);
			return SendMessage(CommandChunkStreamID, MessageTypeCommandAMF0, messageStreamID, 0, payload.data(), payload.size());
		}

		bool SendMetadata()
		{
			std::vector<uint8_t> payload;
			AppendAMF0String(payload, "@setDataFrame");
			AppendAMF0String(payload, "onMetaData");
			AppendAMF0ECMAArrayStart(payload, 8);
			AppendAMF0PropertyNumber(payload, "width", _width);
			AppendAMF0PropertyNumber(payload, "height", _height);
			AppendAMF0PropertyNumber(payload, "videocodecid", 7);
			AppendAMF0PropertyNumber(payload, "framerate", 30);
			AppendAMF0PropertyNumber(payload, "audiocodecid", 10);
			AppendAMF0PropertyNumber(payload, "audiosamplerate", _sampleRate);
			AppendAMF0PropertyNumber(payload, "audiosamplesize", 16);
			AppendAMF0PropertyBoolean(payload, "stereo", _channelCount > 1);
			AppendAMF0ObjectEnd(payload);
			return SendMessage(DataChunkStreamID, MessageTypeDataAMF0, _streamID, 0, payload.data(), payload.size());
		}

		bool SendMessage(uint32_t chunkStreamID, uint8_t messageType, uint32_t messageStreamID, uint32_t timestamp, const uint8_t *payload, size_t payloadSize)
		{
			size_t offset = 0;
			bool isFirstChunk = true;
			uint32_t timestampHeader = timestamp >= 0xffffff ? 0xffffff : timestamp;

			while(offset < payloadSize || (payloadSize == 0 && isFirstChunk))
			{
				std::vector<uint8_t> chunk;
				if(isFirstChunk)
				{
					AppendBasicHeader(chunk, 0, chunkStreamID);
					AppendU24BE(chunk, timestampHeader);
					AppendU24BE(chunk, static_cast<uint32_t>(payloadSize));
					chunk.push_back(messageType);
					AppendU32LE(chunk, messageStreamID);
				}
				else
				{
					AppendBasicHeader(chunk, 3, chunkStreamID);
				}

				if(timestampHeader == 0xffffff) AppendU32BE(chunk, timestamp);

				size_t remaining = payloadSize - offset;
				size_t chunkPayloadSize = remaining > _outChunkSize ? _outChunkSize : remaining;
				if(chunkPayloadSize > 0)
				{
					chunk.insert(chunk.end(), payload + offset, payload + offset + chunkPayloadSize);
				}

				if(!WriteAll(chunk.data(), chunk.size())) return false;
				offset += chunkPayloadSize;
				isFirstChunk = false;
			}

			return true;
		}

		bool WaitForCommandResponse(double transactionID, uint32_t *streamID, String **errorMessage)
		{
			uint64_t deadline = GetMonotonicTimeMs() + 10000;
			while(!_shouldStop.load() && GetMonotonicTimeMs() < deadline)
			{
				Message message;
				if(!ReadMessage(message))
				{
					SetErrorIfMissing(errorMessage, RNCSTR("RTMP server closed the connection."));
					return false;
				}
				HandleInboundControlMessage(message);

				if(message.type != MessageTypeCommandAMF0 && message.type != MessageTypeCommandAMF3) continue;

				size_t offset = message.type == MessageTypeCommandAMF3 ? 1 : 0;
				std::string command;
				double responseTransactionID = 0.0;
				if(!ReadAMF0String(message.payload, offset, command)) continue;
				if(!ReadAMF0Number(message.payload, offset, responseTransactionID)) continue;

				if((command == "_result" || command == "_error") && static_cast<int>(responseTransactionID) == static_cast<int>(transactionID))
				{
					if(command == "_error")
					{
						RNWarning("RTMP command returned error for transaction " << transactionID);
						SetErrorIfMissing(errorMessage, RNCSTR("RTMP server rejected the connection."));
						return false;
					}

					if(streamID)
					{
						SkipAMF0Value(message.payload, offset);
						double streamIDValue = 0.0;
						if(!ReadAMF0Number(message.payload, offset, streamIDValue)) return false;
						*streamID = static_cast<uint32_t>(streamIDValue);
					}

					return true;
				}
			}

			RNWarning("Timed out waiting for RTMP command response " << transactionID);
			SetErrorIfMissing(errorMessage, RNCSTR("Timed out waiting for the RTMP server."));
			return false;
		}

		bool WaitForPublishResponse(String **errorMessage)
		{
			uint64_t deadline = GetMonotonicTimeMs() + 10000;
			while(!_shouldStop.load() && GetMonotonicTimeMs() < deadline)
			{
				Message message;
				if(!ReadMessage(message))
				{
					SetErrorIfMissing(errorMessage, RNCSTR("RTMP server closed the connection before accepting the stream."));
					return false;
				}
				HandleInboundControlMessage(message);

				if(message.type != MessageTypeCommandAMF0 && message.type != MessageTypeCommandAMF3) continue;

				size_t offset = message.type == MessageTypeCommandAMF3 ? 1 : 0;
				std::string command;
				double transactionID = 0.0;
				if(!ReadAMF0String(message.payload, offset, command)) continue;
				if(!ReadAMF0Number(message.payload, offset, transactionID)) continue;

				if(command == "_error")
				{
					SetError(errorMessage, RNCSTR("RTMP server rejected the publish request."));
					return false;
				}

				if(command != "onStatus") continue;
				SkipAMF0Value(message.payload, offset);

				std::string code;
				std::string description;
				ReadAMF0ObjectStringProperty(message.payload, offset, "code", code);
				ReadAMF0ObjectStringProperty(message.payload, offset, "description", description);
				RNInfo("RTMP publish status: " << (code.empty() ? "<none>" : code) << ", description: " << (description.empty() ? "<none>" : description));

				if(code == "NetStream.Publish.Start")
				{
					return true;
				}

				if(code.find("NetStream.Publish.") != std::string::npos)
				{
					SetError(errorMessage, RNSTR("" << (description.empty() ? code : description)));
					return false;
				}
			}

			SetErrorIfMissing(errorMessage, RNCSTR("Timed out waiting for the RTMP publish response."));
			return false;
		}

		bool ReadMessage(Message &message)
		{
			while(!_shouldStop.load())
			{
				uint8_t basicHeaderByte = 0;
				if(!ReadFully(&basicHeaderByte, 1)) return false;

				uint32_t format = basicHeaderByte >> 6;
				uint32_t chunkStreamID = basicHeaderByte & 0x3f;
				if(chunkStreamID == 0)
				{
					uint8_t next = 0;
					if(!ReadFully(&next, 1)) return false;
					chunkStreamID = static_cast<uint32_t>(next) + 64;
				}
				else if(chunkStreamID == 1)
				{
					uint8_t extra[2];
					if(!ReadFully(extra, 2)) return false;
					chunkStreamID = static_cast<uint32_t>(extra[0]) + static_cast<uint32_t>(extra[1]) * 256 + 64;
				}

				InboundChunk &chunk = _inboundChunks[chunkStreamID];
				uint32_t timestamp = chunk.timestamp;
				uint32_t messageLength = chunk.messageLength;
				uint8_t messageType = chunk.messageType;

				if(format <= 2)
				{
					uint8_t timestampBytes[3];
					if(!ReadFully(timestampBytes, 3)) return false;
					timestamp = ReadU24BE(timestampBytes);
				}

				if(format <= 1)
				{
					uint8_t header[4];
					if(!ReadFully(header, 4)) return false;
					messageLength = ReadU24BE(header);
					messageType = header[3];
				}

				if(format == 0)
				{
					uint8_t streamIDBytes[4];
					if(!ReadFully(streamIDBytes, 4)) return false;
					chunk.payload.clear();
				}
				else if(format < 3 && chunk.payload.size() >= messageLength)
				{
					chunk.payload.clear();
				}

				if(timestamp == 0xffffff)
				{
					uint8_t extendedTimestamp[4];
					if(!ReadFully(extendedTimestamp, 4)) return false;
					timestamp = ReadU32BE(extendedTimestamp);
				}

				chunk.timestamp = timestamp;
				chunk.messageLength = messageLength;
				chunk.messageType = messageType;

				size_t remaining = messageLength - chunk.payload.size();
				size_t readSize = remaining > _inChunkSize ? _inChunkSize : remaining;
				if(readSize > 0)
				{
					size_t oldSize = chunk.payload.size();
					chunk.payload.resize(oldSize + readSize);
					if(!ReadFully(chunk.payload.data() + oldSize, readSize)) return false;
				}

				if(chunk.payload.size() == messageLength)
				{
					message.type = chunk.messageType;
					message.payload = chunk.payload;
					chunk.payload.clear();
					return true;
				}
			}

			return false;
		}

		void HandleInboundControlMessage(const Message &message)
		{
			if(message.type == MessageTypeSetChunkSize && message.payload.size() >= 4)
			{
				_inChunkSize = ReadU32BE(message.payload.data());
			}
		}

		static bool ExtractVideoParameterSets(const CaptureEncodedVideoFormat &format, std::vector<uint8_t> &sps, std::vector<uint8_t> &pps)
		{
			if(format.sequenceParameterSet && format.sequenceParameterSetSize > 0)
			{
				sps.assign(format.sequenceParameterSet, format.sequenceParameterSet + format.sequenceParameterSetSize);
			}
			if(format.pictureParameterSet && format.pictureParameterSetSize > 0)
			{
				pps.assign(format.pictureParameterSet, format.pictureParameterSet + format.pictureParameterSetSize);
			}
			if(!sps.empty() && !pps.empty()) return true;

			if(format.codecConfiguration && format.codecConfigurationSize > 0)
			{
				if(ParseAVCDecoderConfigurationRecord(format.codecConfiguration, format.codecConfigurationSize, sps, pps)) return true;
				ExtractH264ParameterSets(format.codecConfiguration, format.codecConfigurationSize, sps, pps);
			}

			if(format.codecConfiguration2 && format.codecConfiguration2Size > 0)
			{
				ExtractH264ParameterSets(format.codecConfiguration2, format.codecConfiguration2Size, sps, pps);
			}

			return !sps.empty() && !pps.empty();
		}

		static bool ParseAVCDecoderConfigurationRecord(const uint8_t *data, size_t size, std::vector<uint8_t> &sps, std::vector<uint8_t> &pps)
		{
			if(size < 7 || data[0] != 1) return false;

			size_t offset = 5;
			uint8_t spsCount = data[offset++] & 0x1f;
			for(uint8_t i = 0; i < spsCount && offset + 2 <= size; i++)
			{
				size_t naluSize = (static_cast<size_t>(data[offset]) << 8) | data[offset + 1];
				offset += 2;
				if(offset + naluSize > size) return false;
				if(sps.empty()) sps.assign(data + offset, data + offset + naluSize);
				offset += naluSize;
			}

			if(offset >= size) return false;
			uint8_t ppsCount = data[offset++];
			for(uint8_t i = 0; i < ppsCount && offset + 2 <= size; i++)
			{
				size_t naluSize = (static_cast<size_t>(data[offset]) << 8) | data[offset + 1];
				offset += 2;
				if(offset + naluSize > size) return false;
				if(pps.empty()) pps.assign(data + offset, data + offset + naluSize);
				offset += naluSize;
			}

			return !sps.empty() && !pps.empty();
		}

		static void ExtractH264ParameterSets(const uint8_t *data, size_t size, std::vector<uint8_t> &sps, std::vector<uint8_t> &pps)
		{
			std::vector<std::pair<const uint8_t *, size_t>> nalUnits;
			FindAnnexBNALUnits(data, size, nalUnits);
			if(nalUnits.empty()) nalUnits.push_back(std::make_pair(data, size));

			for(const auto &unit : nalUnits)
			{
				if(unit.second == 0) continue;
				uint8_t type = unit.first[0] & 0x1f;
				if(type == 7 && sps.empty())
					sps.assign(unit.first, unit.first + unit.second);
				else if(type == 8 && pps.empty())
					pps.assign(unit.first, unit.first + unit.second);
			}
		}

		static void BuildVideoSequenceHeader(const std::vector<uint8_t> &sps, const std::vector<uint8_t> &pps, std::vector<uint8_t> &payload)
		{
			payload.clear();
			payload.push_back(0x17);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);

			payload.push_back(1);
			payload.push_back(sps.size() > 1 ? sps[1] : 66);
			payload.push_back(sps.size() > 2 ? sps[2] : 0);
			payload.push_back(sps.size() > 3 ? sps[3] : 31);
			payload.push_back(0xff);
			payload.push_back(0xe1);
			AppendU16BE(payload, static_cast<uint16_t>(sps.size()));
			payload.insert(payload.end(), sps.begin(), sps.end());
			payload.push_back(1);
			AppendU16BE(payload, static_cast<uint16_t>(pps.size()));
			payload.insert(payload.end(), pps.begin(), pps.end());
		}

		static void BuildVideoPayload(const uint8_t *data, size_t size, bool isKeyFrame, std::vector<uint8_t> &payload)
		{
			payload.clear();
			payload.push_back(isKeyFrame ? 0x17 : 0x27);
			payload.push_back(1);
			payload.push_back(0);
			payload.push_back(0);
			payload.push_back(0);

			if(StartsWithAnnexBStartCode(data, size))
			{
				std::vector<std::pair<const uint8_t *, size_t>> nalUnits;
				FindAnnexBNALUnits(data, size, nalUnits);
				for(const auto &unit : nalUnits)
				{
					if(unit.second == 0) continue;
					uint8_t type = unit.first[0] & 0x1f;
					if(type == 7 || type == 8) continue;
					AppendU32BE(payload, static_cast<uint32_t>(unit.second));
					payload.insert(payload.end(), unit.first, unit.first + unit.second);
				}
			}
			else if(LooksLikeLengthPrefixedH264(data, size))
			{
				payload.insert(payload.end(), data, data + size);
			}
			else
			{
				AppendU32BE(payload, static_cast<uint32_t>(size));
				payload.insert(payload.end(), data, data + size);
			}

			if(payload.size() == 5) payload.clear();
		}

		static void BuildAACAudioSpecificConfig(int32_t sampleRate, int32_t channelCount, std::vector<uint8_t> &config)
		{
			static const int32_t sampleRates[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};
			uint8_t frequencyIndex = 3;
			for(uint8_t i = 0; i < 13; i++)
			{
				if(sampleRates[i] == sampleRate)
				{
					frequencyIndex = i;
					break;
				}
			}

			uint8_t audioObjectType = 2;
			uint8_t channels = static_cast<uint8_t>(channelCount);
			config.clear();
			config.push_back(static_cast<uint8_t>((audioObjectType << 3) | (frequencyIndex >> 1)));
			config.push_back(static_cast<uint8_t>(((frequencyIndex & 1) << 7) | (channels << 3)));
		}

		static void BuildAudioSequenceHeader(const std::vector<uint8_t> &config, std::vector<uint8_t> &payload)
		{
			payload.clear();
			payload.push_back(0xaf);
			payload.push_back(0);
			payload.insert(payload.end(), config.begin(), config.end());
		}

		static bool IsKeyFrame(const uint8_t *data, size_t size, bool isMarkedKeyFrame)
		{
			if(isMarkedKeyFrame) return true;

			if(StartsWithAnnexBStartCode(data, size))
			{
				std::vector<std::pair<const uint8_t *, size_t>> nalUnits;
				FindAnnexBNALUnits(data, size, nalUnits);
				for(const auto &unit : nalUnits)
				{
					if(unit.second > 0 && (unit.first[0] & 0x1f) == 5) return true;
				}
			}
			else if(size > 0)
			{
				return (data[0] & 0x1f) == 5;
			}

			return false;
		}

		static bool StartsWithAnnexBStartCode(const uint8_t *data, size_t size)
		{
			return size >= 4 && data[0] == 0 && data[1] == 0 && ((data[2] == 1) || (data[2] == 0 && data[3] == 1));
		}

		static void FindAnnexBNALUnits(const uint8_t *data, size_t size, std::vector<std::pair<const uint8_t *, size_t>> &nalUnits)
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
				if(naluEnd > naluStart) nalUnits.push_back(std::make_pair(data + naluStart, naluEnd - naluStart));

				if(nextStartCodeOffset == std::string::npos) break;
				offset = nextStartCodeOffset;
			}
		}

		static size_t FindStartCode(const uint8_t *data, size_t size, size_t offset)
		{
			for(size_t i = offset; i + 3 < size; i++)
			{
				if(data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1) return i;
				if(i + 4 <= size && data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 0 && data[i + 3] == 1) return i;
			}

			return std::string::npos;
		}

		static size_t StartCodeLength(const uint8_t *data, size_t size)
		{
			if(size >= 4 && data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 1) return 4;
			return 3;
		}

		static bool LooksLikeLengthPrefixedH264(const uint8_t *data, size_t size)
		{
			size_t offset = 0;
			size_t count = 0;
			while(offset + 4 <= size)
			{
				uint32_t naluSize = ReadU32BE(data + offset);
				offset += 4;
				if(naluSize == 0 || offset + naluSize > size) return false;
				offset += naluSize;
				count++;
			}

			return count > 0 && offset == size;
		}

		static void AppendBasicHeader(std::vector<uint8_t> &data, uint8_t format, uint32_t chunkStreamID)
		{
			if(chunkStreamID < 64)
			{
				data.push_back(static_cast<uint8_t>((format << 6) | chunkStreamID));
			}
			else if(chunkStreamID < 320)
			{
				data.push_back(static_cast<uint8_t>(format << 6));
				data.push_back(static_cast<uint8_t>(chunkStreamID - 64));
			}
			else
			{
				data.push_back(static_cast<uint8_t>((format << 6) | 1));
				uint32_t encoded = chunkStreamID - 64;
				data.push_back(static_cast<uint8_t>(encoded & 0xff));
				data.push_back(static_cast<uint8_t>((encoded >> 8) & 0xff));
			}
		}

		static void AppendAMF0String(std::vector<uint8_t> &data, const std::string &value)
		{
			data.push_back(2);
			AppendU16BE(data, static_cast<uint16_t>(value.length()));
			data.insert(data.end(), value.begin(), value.end());
		}

		static void AppendAMF0Number(std::vector<uint8_t> &data, double value)
		{
			data.push_back(0);
			uint64_t bits = 0;
			memcpy(&bits, &value, sizeof(bits));
			for(int i = 7; i >= 0; i--)
			{
				data.push_back(static_cast<uint8_t>((bits >> (i * 8)) & 0xff));
			}
		}

		static void AppendAMF0Boolean(std::vector<uint8_t> &data, bool value)
		{
			data.push_back(1);
			data.push_back(value ? 1 : 0);
		}

		static void AppendAMF0Null(std::vector<uint8_t> &data)
		{
			data.push_back(5);
		}

		static void AppendAMF0ObjectStart(std::vector<uint8_t> &data)
		{
			data.push_back(3);
		}

		static void AppendAMF0ECMAArrayStart(std::vector<uint8_t> &data, uint32_t count)
		{
			data.push_back(8);
			AppendU32BE(data, count);
		}

		static void AppendAMF0ObjectEnd(std::vector<uint8_t> &data)
		{
			data.push_back(0);
			data.push_back(0);
			data.push_back(9);
		}

		static void AppendAMF0PropertyName(std::vector<uint8_t> &data, const char *name)
		{
			size_t length = strlen(name);
			AppendU16BE(data, static_cast<uint16_t>(length));
			data.insert(data.end(), name, name + length);
		}

		static void AppendAMF0PropertyString(std::vector<uint8_t> &data, const char *name, const std::string &value)
		{
			AppendAMF0PropertyName(data, name);
			AppendAMF0String(data, value);
		}

		static void AppendAMF0PropertyNumber(std::vector<uint8_t> &data, const char *name, double value)
		{
			AppendAMF0PropertyName(data, name);
			AppendAMF0Number(data, value);
		}

		static void AppendAMF0PropertyBoolean(std::vector<uint8_t> &data, const char *name, bool value)
		{
			AppendAMF0PropertyName(data, name);
			AppendAMF0Boolean(data, value);
		}

		static bool ReadAMF0String(const std::vector<uint8_t> &data, size_t &offset, std::string &value)
		{
			if(offset + 3 > data.size() || data[offset] != 2) return false;
			offset++;
			uint16_t length = ReadU16BE(data.data() + offset);
			offset += 2;
			if(offset + length > data.size()) return false;
			value.assign(reinterpret_cast<const char *>(data.data() + offset), length);
			offset += length;
			return true;
		}

		static bool ReadAMF0Number(const std::vector<uint8_t> &data, size_t &offset, double &value)
		{
			if(offset + 9 > data.size() || data[offset] != 0) return false;
			offset++;

			uint64_t bits = 0;
			for(size_t i = 0; i < 8; i++)
			{
				bits = (bits << 8) | data[offset + i];
			}
			offset += 8;
			memcpy(&value, &bits, sizeof(value));
			return true;
		}

		static bool SkipAMF0Value(const std::vector<uint8_t> &data, size_t &offset)
		{
			if(offset >= data.size()) return false;

			uint8_t type = data[offset++];
			if(type == 0)
			{
				if(offset + 8 > data.size()) return false;
				offset += 8;
				return true;
			}
			if(type == 1)
			{
				if(offset + 1 > data.size()) return false;
				offset += 1;
				return true;
			}
			if(type == 2)
			{
				if(offset + 2 > data.size()) return false;
				uint16_t length = ReadU16BE(data.data() + offset);
				offset += 2 + length;
				return offset <= data.size();
			}
			if(type == 3 || type == 8)
			{
				if(type == 8)
				{
					if(offset + 4 > data.size()) return false;
					offset += 4;
				}

				while(offset + 3 <= data.size())
				{
					uint16_t nameLength = ReadU16BE(data.data() + offset);
					offset += 2;
					if(nameLength == 0 && offset < data.size() && data[offset] == 9)
					{
						offset++;
						return true;
					}
					offset += nameLength;
					if(offset > data.size()) return false;
					if(!SkipAMF0Value(data, offset)) return false;
				}
				return false;
			}
			if(type == 5 || type == 6) return true;

			return false;
		}

		static bool ReadAMF0ObjectStringProperty(const std::vector<uint8_t> &data, size_t objectOffset, const char *propertyName, std::string &value)
		{
			if(objectOffset >= data.size()) return false;

			size_t offset = objectOffset;
			uint8_t type = data[offset++];
			if(type == 8)
			{
				if(offset + 4 > data.size()) return false;
				offset += 4;
			}
			else if(type != 3)
			{
				return false;
			}

			while(offset + 3 <= data.size())
			{
				uint16_t nameLength = ReadU16BE(data.data() + offset);
				offset += 2;
				if(nameLength == 0 && offset < data.size() && data[offset] == 9) return false;
				if(offset + nameLength > data.size()) return false;

				std::string name(reinterpret_cast<const char *>(data.data() + offset), nameLength);
				offset += nameLength;

				if(name == propertyName)
				{
					return ReadAMF0String(data, offset, value);
				}

				if(!SkipAMF0Value(data, offset)) return false;
			}

			return false;
		}

		static void AppendU16BE(std::vector<uint8_t> &data, uint16_t value)
		{
			data.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
			data.push_back(static_cast<uint8_t>(value & 0xff));
		}

		static void AppendU24BE(std::vector<uint8_t> &data, uint32_t value)
		{
			data.push_back(static_cast<uint8_t>((value >> 16) & 0xff));
			data.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
			data.push_back(static_cast<uint8_t>(value & 0xff));
		}

		static void AppendU32BE(std::vector<uint8_t> &data, uint32_t value)
		{
			data.push_back(static_cast<uint8_t>((value >> 24) & 0xff));
			data.push_back(static_cast<uint8_t>((value >> 16) & 0xff));
			data.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
			data.push_back(static_cast<uint8_t>(value & 0xff));
		}

		static void AppendU32LE(std::vector<uint8_t> &data, uint32_t value)
		{
			data.push_back(static_cast<uint8_t>(value & 0xff));
			data.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
			data.push_back(static_cast<uint8_t>((value >> 16) & 0xff));
			data.push_back(static_cast<uint8_t>((value >> 24) & 0xff));
		}

		static void WriteU32BE(std::vector<uint8_t> &data, size_t offset, uint32_t value)
		{
			data[offset] = static_cast<uint8_t>((value >> 24) & 0xff);
			data[offset + 1] = static_cast<uint8_t>((value >> 16) & 0xff);
			data[offset + 2] = static_cast<uint8_t>((value >> 8) & 0xff);
			data[offset + 3] = static_cast<uint8_t>(value & 0xff);
		}

		static uint16_t ReadU16BE(const uint8_t *data)
		{
			return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);
		}

		static uint32_t ReadU24BE(const uint8_t *data)
		{
			return (static_cast<uint32_t>(data[0]) << 16) | (static_cast<uint32_t>(data[1]) << 8) | data[2];
		}

		static uint32_t ReadU32BE(const uint8_t *data)
		{
			return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) | (static_cast<uint32_t>(data[2]) << 8) | data[3];
		}

		static uint64_t GetMonotonicTimeMs()
		{
#if RN_PLATFORM_WINDOWS
			return static_cast<uint64_t>(GetTickCount64());
#elif RN_PLATFORM_POSIX
			struct timespec time;
			clock_gettime(CLOCK_MONOTONIC, &time);
			return static_cast<uint64_t>(time.tv_sec) * 1000 + static_cast<uint64_t>(time.tv_nsec / 1000000);
#else
			return 0;
#endif
		}

		static void SleepForMilliseconds(uint32_t milliseconds)
		{
#if RN_PLATFORM_WINDOWS
			Sleep(milliseconds);
#elif RN_PLATFORM_POSIX
			usleep(milliseconds * 1000);
#else
			(void)milliseconds;
#endif
		}

		URL _url;
		SocketHandle _socketFD;
#if RN_CAPTURE_RTMP_OPENSSL_TLS_SUPPORTED
		SSL_CTX *_sslContext;
		SSL *_ssl;
#endif
#if RN_CAPTURE_RTMP_SCHANNEL_TLS_SUPPORTED
		CredHandle _securityCredentials;
		CtxtHandle _securityContext;
		SecPkgContext_StreamSizes _tlsStreamSizes;
		std::vector<uint8_t> _tlsEncryptedBuffer;
		std::vector<uint8_t> _tlsDecryptedBuffer;
		bool _hasSecurityCredentials;
		bool _hasSecurityContext;
#endif

		uint32_t _outChunkSize;
		uint32_t _inChunkSize;
		uint32_t _streamID;
		int32_t _width;
		int32_t _height;
		int32_t _sampleRate;
		int32_t _channelCount;

		Lockable _lock;
		std::deque<Packet> _packets;
		std::map<uint32_t, InboundChunk> _inboundChunks;
		std::vector<uint8_t> _videoSequenceHeader;
		std::vector<uint8_t> _audioSequenceHeader;
		std::function<void(const String *)> _errorCallback;
		bool _hasVideoFormat;
		bool _hasAudioFormat;
		bool _hasNetworkSession;
		bool _hasQueuedVideoSequenceHeader;
		bool _hasQueuedAudioSequenceHeader;
		bool _hasLoggedQueuePressure;
		std::atomic<bool> _shouldStop;
		Thread *_workerThread;
		std::atomic<bool> _isWorkerRunning;
		std::atomic<bool> _isReady;
	};

	CaptureRTMPWriter::CaptureRTMPWriter() :
		_state(new State())
	{}

	CaptureRTMPWriter::~CaptureRTMPWriter()
	{
		delete _state;
	}

	bool CaptureRTMPWriter::Start(const CaptureSession::Configuration &configuration)
	{
		return _state->Start(configuration);
	}

	void CaptureRTMPWriter::Stop()
	{
		_state->Stop();
	}

	bool CaptureRTMPWriter::DidReceiveVideoFormat(const CaptureEncodedVideoFormat &format)
	{
		return _state->DidReceiveVideoFormat(format);
	}

	bool CaptureRTMPWriter::DidReceiveAudioFormat(const CaptureEncodedAudioFormat &format)
	{
		return _state->DidReceiveAudioFormat(format);
	}

	bool CaptureRTMPWriter::GetIsReady() const
	{
		return _state->GetIsReady();
	}

	void CaptureRTMPWriter::WriteVideoSample(const CaptureEncodedSample &sample)
	{
		_state->WriteVideoSample(sample);
	}

	void CaptureRTMPWriter::WriteAudioSample(const CaptureEncodedSample &sample)
	{
		_state->WriteAudioSample(sample);
	}

	bool CaptureRTMPWriter::TestConnection(const String *streamURL, String **errorMessage)
	{
		if(errorMessage) *errorMessage = nullptr;
		State state;
		return state.TestConnection(streamURL ? streamURL->GetUTF8String() : nullptr, errorMessage);
	}

	bool CaptureRTMPWriter::TestPublishConnection(const String *streamURL, String **errorMessage)
	{
		if(errorMessage) *errorMessage = nullptr;
		State state;
		return state.TestPublishConnection(streamURL ? streamURL->GetUTF8String() : nullptr, errorMessage);
	}
} // namespace RN
