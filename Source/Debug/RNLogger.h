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

		LogMessage() :
			_cmessage(nullptr)
		{}

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
		struct LogEntry
		{
			LogEntry() = default;

			LogEntry(Level tlevel, LogMessage &&tmessage) :
				level(tlevel),
				message(std::move(tmessage))
			{}

			LogEntry(LogEntry &&other) :
				level(other.level),
				message(std::move(other.message))
			{}

			LogEntry &operator= (LogEntry &&other)
			{
				level = other.level;
				message = std::move(other.message);

				return *this;
			}

			LogMessage message;
			Level level;
		};

		Logger();
		~Logger();

		void __LoadDefaultLoggers();
		void __FlushQueue();

		SpinLock _lock;
		AtomicRingBuffer<LogEntry, 128> _messages;

		std::mutex _engineLock;
		Array *_threadEngines;
		Array *_engines;

		std::atomic<bool> _queueMarked;
		WorkQueue *_queue;
		std::chrono::system_clock::time_point _lastMessage;
	};

	class LogBuilder
	{
	public:
		RNAPI LogBuilder(size_t line, const char *file, const char *function, Logger::Level level = Logger::Level::Info);
		RNAPI ~LogBuilder();

		RNAPI void Submit();

		LogBuilder &operator << (const Object *object) { _stream << object->GetDescription()->GetUTF8String(); return *this; }
		LogBuilder &operator << (const std::exception &e) { _stream << e.what(); return *this; }
		LogBuilder &operator << (const std::string &val) { _stream << val; return *this; }
		LogBuilder &operator << (const char *val) { _stream << val; return *this; }
		LogBuilder &operator << (bool val) { _stream << val; return *this; }
		LogBuilder &operator << (short val) { _stream << val; return *this; }
		LogBuilder &operator << (unsigned short val) { _stream << val; return *this; }
		LogBuilder &operator << (int val) { _stream << val; return *this; }
		LogBuilder &operator << (unsigned int val) { _stream << val; return *this; }
		LogBuilder &operator << (long val) { _stream << val; return *this; }
		LogBuilder &operator << (unsigned long val) { _stream << val; return *this; }
		LogBuilder &operator << (long long val) { _stream << val; return *this; }
		LogBuilder &operator << (unsigned long long val) { _stream << val; return *this; }
		LogBuilder &operator << (float val) { _stream << val; return *this; }
		LogBuilder &operator << (double val) { _stream << val; return *this; }
		LogBuilder &operator << (long double val) { _stream << val; return *this; }
		LogBuilder &operator << (const void *val) { _stream << val; return *this; }
		LogBuilder &operator << (std::ostream &(*pf)(std::ostream &)) { _stream << pf; return *this; }
		LogBuilder &operator << (std::ios &(*pf)(std::ios &)) { _stream << pf; return *this; };
		LogBuilder &operator << (std::ios_base &(*pf)(std::ios_base &)) { _stream << pf; return *this; }

	private:
		Logger::Level _level;
		size_t _line;
		const char *_file;
		const char *_function;
		std::stringstream _stream;
	};
}

#define RNDebugf(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Debug, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, __VA_ARGS__)
#define RNInfof(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Info, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, __VA_ARGS__)
#define RNWarningf(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Warning, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, __VA_ARGS__)
#define RNErrorf(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Error, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, __VA_ARGS__)

#define RNDebug(exp) do { RN::LogBuilder builder(__LINE__, __FILE__, RN_FUNCTION_SIGNATURE, RN::Logger::Level::Debug); builder << exp; } while(0)
#define RNInfo(exp) do { RN::LogBuilder builder(__LINE__, __FILE__, RN_FUNCTION_SIGNATURE, RN::Logger::Level::Info); builder << exp; } while(0)
#define RNWarning(exp) do { RN::LogBuilder builder(__LINE__, __FILE__, RN_FUNCTION_SIGNATURE, RN::Logger::Level::Warning); builder << exp; } while(0)
#define RNError(exp)  do { RN::LogBuilder builder(__LINE__, __FILE__, RN_FUNCTION_SIGNATURE, RN::Logger::Level::Error); builder << exp; } while(0)

#endif /* __RAYNE_LOGGING_H_ */
