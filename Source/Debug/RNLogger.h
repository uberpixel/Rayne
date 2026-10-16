//
//  RNLogging.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LOGGING_H__
#define __RAYNE_LOGGING_H__

#include "../Base/RNBase.h"
#include "../Objects/RNArray.h"
#include "../Objects/RNString.h"
#include "../Threads/RNWorkQueue.h"

namespace RN
{
	class Logger;
	struct LogMessage
	{
		friend class Logger;

		LogMessage() :
			message(nullptr), file(nullptr)
		{}

		LogMessage(size_t tline, const char *tfile, const char *tfunction, const String *string) :
			line(tline),
			file(tfile),
			function(tfunction),
			time(std::chrono::system_clock::now()),
			message(SafeCopy(string))
		{}


		~LogMessage()
		{
			SafeRelease(message);
		}

		LogMessage(LogMessage &&other) :
			line(other.line),
			file(other.file),
			function(other.function),
			time(other.time),
			message(SafeRetain(other.message))
		{}

		LogMessage &operator=(LogMessage &&other)
		{
			line = other.line;
			file = other.file;
			function = other.function;
			time = other.time;
			message = SafeRetain(other.message);

			return *this;
		}

		const char *GetFileName() const
		{
			int32 length = static_cast<int32>(strlen(file));
			while(length > 0)
			{
#if RN_PLATFORM_WINDOWS
				if(file[length] == '/' || file[length] == '\\')
					return file + length + 1;
#else
				if(file[length] == '/')
					return file + length + 1;
#endif
				length--;
			}

			return file;
		}

		size_t line;
		const char *file;
		const char *function;
		std::chrono::system_clock::time_point time;
		String *message;
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

		// Engine changes from sink callbacks throw InconsistencyException.
		RNAPI void AddEngine(LoggingEngine *engine, Level level = Level::Debug);
		RNAPI void RemoveEngine(LoggingEngine *engine);

		// Recursive logging and flushing from sink callbacks are ignored.
		RNAPI void Log(Level level, LogMessage &&message);
		RNAPI void Log(Level level, size_t line, const char *file, const char *function, const char *format, ...);
		RNAPI void Log(Level level, size_t line, const char *file, const char *function, const String *message);

		RNAPI void Flush(bool synchronous = true);

	private:
		Logger();
		~Logger();

		struct CallbackScope
		{
			CallbackScope();
			~CallbackScope();

			bool previous;
		};

		static thread_local bool _callback;
		void Perform(Function &&command, bool synchronous);
		void Enqueue(Function &&command);
		void Drain();

		Lockable _lock;
		std::deque<Function> _commands;
		bool _draining;

		Lockable _engineLock;
		// Bound engines use _engineLock; asynchronous engines belong to Drain().
		std::vector<StrongRef<LoggingEngine>> _threadEngines;
		std::vector<StrongRef<LoggingEngine>> _engines;
		std::atomic<bool> _hasAsyncEngines;

		WorkQueue *_queue;
		std::chrono::system_clock::time_point _lastMessage;
	};
} // namespace RN

#define RNDebugf(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Debug, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, __VA_ARGS__)
#define RNInfof(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Info, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, __VA_ARGS__)
#define RNWarningf(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Warning, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, __VA_ARGS__)
#define RNErrorf(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Error, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, __VA_ARGS__)

#define RNDebug(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Debug, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, RNSTR(__VA_ARGS__))
#define RNInfo(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Info, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, RNSTR(__VA_ARGS__))
#define RNWarning(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Warning, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, RNSTR(__VA_ARGS__))
#define RNError(...) RN::Logger::GetSharedInstance()->Log(RN::Logger::Level::Error, __LINE__, __FILE__, RN_FUNCTION_SIGNATURE, RNSTR(__VA_ARGS__))

#endif /* __RAYNE_LOGGING_H__ */
