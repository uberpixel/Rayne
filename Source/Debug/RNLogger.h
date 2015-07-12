//
//  RNLogging.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_LOGGING_H_
#define __RAYNE_LOGGING_H_

#include "../Base/RNBase.h"
#include "../Data/RNRingBuffer.h"
#include "../Objects/RNArray.h"
#include "../Threads/RNWorkQueue.h"

namespace RN
{
	class Logger;
	struct LogMessage
	{
		friend class Logger;

		LogMessage() = default;
		LogMessage(size_t tline, const char *tfile, const char *tfunction, std::string &&tmessage) :
			line(tline),
			file(tfile),
			function(tfunction),
			time(std::chrono::system_clock::now()),
			_strmessage(std::move(tmessage)),
			_cmessage(nullptr)
		{}

		LogMessage(size_t tline, const char *tfile, const char *tfunction, const char *tmessage) :
			line(tline),
			file(tfile),
			function(tfunction),
			time(std::chrono::system_clock::now()),
			_cmessage(tmessage)
		{}

		~LogMessage()
		{
			delete[] _cmessage;
		}

		LogMessage(LogMessage &&other) :
			line(other.line),
			file(other.file),
			function(other.function),
			time(other.time),
			formattedTime(std::move(other.formattedTime)),
			_strmessage(std::move(other._strmessage)),
			_cmessage(other._cmessage)
		{
			other._cmessage = nullptr;
		}

		LogMessage &operator= (LogMessage &&other)
		{
			line = other.line;
			file = other.file;
			function = other.function;
			time = other.time;
			formattedTime = std::move(formattedTime);
			_strmessage = std::move(_strmessage);
			_cmessage = other._cmessage;

			other._cmessage = nullptr;

			return *this;
		}

		const char *GetMessage() const { return _cmessage ? _cmessage : _strmessage.c_str(); }

		size_t line;
		const char *file;
		const char *function;
		std::chrono::system_clock::time_point time;
		std::string formattedTime;

	private:
		void FormatTime();

		std::string _strmessage;
		const char *_cmessage;
	};

	class LoggingEngine;
	class Kernel;

	class Logger
	{
	public:
		friend class Kernel;

		enum class Level
		{
			Debug,
			Info,
			Warning,
			Error
		};

		RNAPI static Logger *GetSharedInstance();

		RNAPI void AddEngine(LoggingEngine *engine);
		RNAPI void RemoveEngine(LoggingEngine *engine);

		RNAPI void Log(Level level, LogMessage &&message);
		RNAPI void Log(Level level, size_t line, const char *file, const char *function, const char *format, ...);

		RNAPI void Flush();

	private:
		Logger();
		~Logger();

		void __LoadDefaultLoggers();
		void __FlushQueue();

		SpinLock _lock;
		AtomicRingBuffer<LogMessage, 128> _messages;

		std::mutex _engineLock;
		Array *_threadEngines;
		Array *_engines;

		std::atomic<bool> _queueMarked;
		WorkQueue *_queue;
		std::chrono::system_clock::time_point _lastMessage;
	};
}

#define RNDebug(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Debug, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, __VA_ARGS__)
#define RNInfo(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Info, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, __VA_ARGS__)
#define RNWarning(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Warning, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, __VA_ARGS__)
#define RNError(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Error, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, __VA_ARGS__)

#endif /* __RAYNE_LOGGING_H_ */
