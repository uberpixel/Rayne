//
//  RNLogging.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LOGGING_H__
#define __RAYNE_LOGGING_H__

#include "RNBase.h"
#include "RNArray.h"
#include "RNRingbuffer.h"

namespace RN
{
	namespace Log
	{
		class LoggingEngine;
		class Logger;
		
		enum class Level
		{
			Debug,
			Info,
			Warning,
			Error
		};
		
		class Message
		{
		public:
			RNAPI Message();
			RNAPI Message(Level level, const std::string& message);
			RNAPI Message(Level level, const std::string& title, const std::string& message);
			RNAPI Message(Level level, std::string&& message);
			RNAPI Message(Level level, std::string&& title, std::string&& message);
			
			RNAPI void SetTitle(const std::string& title);
			RNAPI void SetTitle(std::string&& title);
			
			RNAPI bool HasTitle() const { return !_title.empty(); }
			
			RNAPI Level GetLevel() const { return _level; }
			RNAPI const std::string& GetTitle() const { return _title; }
			RNAPI const std::string& GetMessage() const { return _message; }
			RNAPI const std::string& GetFormattedTime() const;
			RNAPI std::chrono::system_clock::time_point GetTime() { return _time; }
			
		private:
			Level _level;
			std::string _title;
			std::string _message;
			mutable std::string _formattedTime;
			std::chrono::system_clock::time_point _time;
		};
		
		class Loggable
		{
		public:
			RNAPI Loggable(Level level = Level::Info);
			RNAPI ~Loggable();
			
			RNAPI void Submit();
			
			Loggable& operator << (const std::string& val) { _stream << val; return *this; }
			Loggable& operator << (const char *val) { _stream << val; return *this; }
			Loggable& operator << (bool val) { _stream << val; return *this; }
			Loggable& operator << (short val) { _stream << val; return *this; }
			Loggable& operator << (unsigned short val) { _stream << val; return *this; }
			Loggable& operator << (int val) { _stream << val; return *this; }
			Loggable& operator << (unsigned int val) { _stream << val; return *this; }
			Loggable& operator << (long val) { _stream << val; return *this; }
			Loggable& operator << (unsigned long val) { _stream << val; return *this; }
			Loggable& operator << (long long val) { _stream << val; return *this; }
			Loggable& operator << (unsigned long long val) { _stream << val; return *this; }
			Loggable& operator << (const void *val) { _stream << val; return *this; }
			Loggable& operator << (std::ostream& (*pf)(std::ostream&)) { _stream << pf; return *this; }
			Loggable& operator << (std::ios& (*pf)(std::ios&)) { _stream << pf; return *this; };
			Loggable& operator << (std::ios_base& (*pf)(std::ios_base&)) { _stream << pf; return *this; }
			
		private:
			Level _level;
			std::stringstream _stream;
		};
		
		class Logger : public ISingleton<Logger>
		{
		public:
			RNAPI Logger();
			RNAPI ~Logger();
			
			RNAPI void AddLoggingEngine(LoggingEngine *engine);
			RNAPI void RemoveLoggingEngine(LoggingEngine *engine);
			
			RNAPI void Log(Level level, const std::string& message);
			RNAPI void Log(Level level, std::string&& message);
			RNAPI void Log(Level level, const char *message, ...);
			RNAPI void Log(const Message& message);
			RNAPI void Log(Message&& message);
			
			RNAPI void Flush(bool force = false);
			
		private:
			void FlushBuffer();
			void FlushRunLoop();
			
			SpinLock _lock;
			SpinLock _enginesLock;
			
			Thread *_flushThread;
			
			Array _engines;
			stl::lock_free_ring_buffer<Message, 512> _buffer;
			
			std::mutex _signalLock;
			std::mutex _writeLock;
			std::condition_variable _signal;
			std::condition_variable _writeSignal;
			
			std::chrono::system_clock::time_point _lastMessage;
			
			RNDeclareSingleton(Logger)
		};
	}
}

#define RNDebug(...) RN::Log::Logger::GetSharedInstance()->Log(RN::Log::Level::Debug, __VA_ARGS__)
#define RNInfo(...) RN::Log::Logger::GetSharedInstance()->Log(RN::Log::Level::Info, __VA_ARGS__)
#define RNWarning(...) RN::Log::Logger::GetSharedInstance()->Log(RN::Log::Level::Warning, __VA_ARGS__)
#define RNError(...) RN::Log::Logger::GetSharedInstance()->Log(RN::Log::Level::Error, __VA_ARGS__)

#endif /* __RAYNE_LOGGING_H__ */
