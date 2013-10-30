//
//  RNLogging.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LOGGING_H__
#define __RAYNE_LOGGING_H__

#include "RNBase.h"
#include "RNArray.h"

namespace RN
{
	namespace Log
	{
		class LoggingEngine;
		
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
			Message(Level level, const std::string& message);
			Message(Level level, const std::string& title, const std::string& message);
			Message(Level level, std::string&& message);
			Message(Level level, std::string&& title, std::string&& message);
			
			void SetTitle(const std::string& title);
			void SetTitle(std::string&& title);
			
			bool HasTitle() const { return !_title.empty(); }
			
			Level GetLevel() const { return _level; }
			const std::string& GetTitle() const { return _title; }
			const std::string& GetMessage() const { return _message; }
			const std::string& GetFormattedTime() const;
			std::chrono::system_clock::time_point GetTime() { return _time; }
			
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
			Loggable(Level level = Level::Info);
			~Loggable();
			
			void Submit();
			
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
		
		class Logger : public Singleton<Logger>
		{
		public:
			Logger();
			~Logger();
			
			void SetLogLevel(Level level);
			
			Level GetLogLevel();
			
			void AddLoggingEngine(LoggingEngine *engine);
			void RemoveLoggingEngine(LoggingEngine *engine);
			
			void Log(Level level, const std::string& message);
			void Log(Level level, std::string&& message);
			void Log(Level level, const char *message, ...);
			void Log(const Message& message);
			void Log(Message&& message);
			
			void Flush(bool force = false);
			
		private:
			void FlushBuffer();
			void FlushRunLoop();
			
			SpinLock _lock;
			SpinLock _enginesLock;
			
			Level _level;
			Thread *_flushThread;
			
			Array _engines;
			std::vector<Message> _buffer;
			
			std::mutex _signalLock;
			std::condition_variable _signal;
			
			std::chrono::system_clock::time_point _lastMessage;
		};
	}
}

#define RNDebug(...) RN::Log::Logger::GetSharedInstance()->Log(RN::Log::Level::Debug, __VA_ARGS__)
#define RNInfo(...) RN::Log::Logger::GetSharedInstance()->Log(RN::Log::Level::Info, __VA_ARGS__)
#define RNWarning(...) RN::Log::Logger::GetSharedInstance()->Log(RN::Log::Level::Warning, __VA_ARGS__)
#define RNError(...) RN::Log::Logger::GetSharedInstance()->Log(RN::Log::Level::Error, __VA_ARGS__)

#endif /* __RAYNE_LOGGING_H__ */
