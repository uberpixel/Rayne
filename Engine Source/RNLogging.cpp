//
//  RNLogging.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLogging.h"
#include "RNLoggingEngine.h"
#include "RNThread.h"

namespace RN
{
	namespace Log
	{
		// ---------------------
		// MARK: -
		// MARK: Message
		// ---------------------
		
		Message::Message(Level level, const std::string& message) :
			_level(level),
			_message(message),
			_time(std::chrono::system_clock::now())
		{}
		
		Message::Message(Level level, std::string&& message) :
			_level(level),
			_message(std::move(message)),
			_time(std::chrono::system_clock::now())
		{}
		
		Message::Message(Level level, const std::string& title, const std::string& message) :
			_level(level),
			_title(title),
			_message(message),
			_time(std::chrono::system_clock::now())
		{}
		
		Message::Message(Level level, std::string&& title, std::string&& message) :
			_level(level),
			_title(std::move(title)),
			_message(std::move(message)),
			_time(std::chrono::system_clock::now())
		{}
		
		void Message::SetTitle(const std::string& title)
		{
			_title = title;
		}
		
		void Message::SetTitle(std::string&& title)
		{
			_title = std::move(title);
		}
		
		const std::string& Message::GetFormattedTime() const
		{
			if(!_formattedTime.empty())
				return _formattedTime;
			
			std::stringstream stream;
			std::time_t time = std::chrono::system_clock::to_time_t(_time);
			
#if RN_PLATFORM_POSIX
			std::tm tm = std::tm{0};
			localtime_r(&time, &tm);
#else
			std::tm tm = *localtime(&time);
#endif
			
			std::chrono::duration<double> sec = _time - std::chrono::system_clock::from_time_t(time) + std::chrono::seconds(tm.tm_sec);
			
			
			stream.precision(3);
			stream << tm.tm_mon + 1 << '/' << tm.tm_mday << '/' << tm.tm_year - 100 << ' ';
			
#define PutTwoDigitNumber(n) if(n < 10) stream << '0'; stream << std::fixed << n
			
			PutTwoDigitNumber(tm.tm_hour) << ':';
			PutTwoDigitNumber(tm.tm_min) << ':';
			PutTwoDigitNumber(sec.count());
			
			_formattedTime = stream.str();
			return _formattedTime;
		}
		
		// ---------------------
		// MARK: -
		// MARK: Logger
		// ---------------------
		
		Logger::Logger()
		{
			AddLoggingEngine(StdoutLoggingEngine::GetSharedInstance());
			AddLoggingEngine(HTMLLoggingEngine::GetSharedInstance());
			AddLoggingEngine(SimpleLoggingEngine::GetSharedInstance());
			
#ifndef NDEBUG
			_level = Level::Debug;
#else
			_level = Level::Info;
#endif
			
			_lastMessage = std::chrono::system_clock::now();
			
			_flushThread = new Thread(std::bind(&Logger::FlushRunLoop, this), false);
			_flushThread->SetName("RN::Logger");
			_flushThread->Detach();
		}
		
		Logger::~Logger()
		{
			_flushThread->Cancel();
			_signal.notify_all();
			
			_flushThread->WaitForExit();
			_flushThread->Release();
			
			FlushBuffer();
			
			_engines.Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool *stop) {
				if(engine->IsOpen())
					engine->Close();
			});
		}
		
		void Logger::SetLogLevel(Level level)
		{
			LockGuard<SpinLock> lock(_lock);
			_level = level;
		}
		 
		Level Logger::GetLogLevel()
		{
			LockGuard<SpinLock> lock(_lock);
			Level level = _level;
			
			return level;
		}
		
		
		
		void Logger::AddLoggingEngine(LoggingEngine *engine)
		{
			LockGuard<SpinLock> elock(_enginesLock);
			
			_engines.AddObject(engine);
			
			if(!engine->IsOpen())
				engine->Open();
		}
		
		void Logger::RemoveLoggingEngine(LoggingEngine *engine)
		{
			LockGuard<SpinLock> elock(_enginesLock);
			
			size_t index;
			
			if((index =_engines.GetIndexOfObject(engine)) != k::NotFound)
			{
				engine->Close();
				_engines.RemoveObjectAtIndex(index);
			}
		}
		
		
		
		void Logger::Log(Level level, const std::string& message)
		{
			LockGuard<SpinLock> lock(_lock);
			_buffer.emplace_back(Message(level, message));
		}
		
		void Logger::Log(Level level, std::string&& message)
		{
			LockGuard<SpinLock> lock(_lock);
			_buffer.emplace_back(Message(level, std::move(message)));
		}
		
		void Logger::Log(Level level, const char *message, ...)
		{
			char buffer[1024];
			va_list args;
			
			va_start(args, message);
			vsnprintf(buffer, 1024, message, args);
			va_end(args);
			
			
			LockGuard<SpinLock> lock(_lock);
			_buffer.emplace_back(Message(level, std::string(buffer)));
		}
		
		void Logger::Log(const Message& message)
		{
			LockGuard<SpinLock> lock(_lock);
			_buffer.push_back(message);
		}
		
		void Logger::Log(Message&& message)
		{
			LockGuard<SpinLock> lock(_lock);
			_buffer.push_back(std::move(message));
		}
		
		
		
		void Logger::FlushRunLoop()
		{
			while(!_flushThread->IsCancelled())
			{
				std::unique_lock<std::mutex> lock(_signalLock);
				_signal.wait_for(lock, std::chrono::milliseconds(250));
				
				FlushBuffer();
			}
		}
		
		void Logger::Flush(bool force)
		{
			force ? FlushBuffer() : _signal.notify_all();
		}
		
		void Logger::FlushBuffer()
		{
			LockGuard<SpinLock> lock(_lock);
			std::vector<Message> buffer;
			Level level = _level;
			
			std::swap(buffer, _buffer);
			lock.Unlock();
			
			LockGuard<SpinLock> elock(_enginesLock);
			
			std::chrono::system_clock::time_point temp = _lastMessage;
			
			_engines.Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool *stop) {
				
				_lastMessage = temp;
				
				for(Message& message : buffer)
				{
					long offset = std::chrono::duration_cast<std::chrono::seconds>(message.GetTime() - _lastMessage).count();
					_lastMessage = message.GetTime();
					
					
					if(offset >= 10)
						engine->CutOff();
					
					if(message.GetMessage().length() == 0)
					{
						engine->CutOff();
						continue;
					}
					
					if(message.GetLevel() >= level)
						engine->Write(message);
				}
			});
		}
		
		// ---------------------
		// MARK: -
		// MARK: Loggable
		// ---------------------
		
		Loggable::Loggable(Level level) :
			_level(level)
		{}
		
		Loggable::~Loggable()
		{
			Submit();
		}
		
		void Loggable::Submit()
		{
			std::string string = std::move(_stream.str());
			_stream.clear();
			
			Logger::GetSharedInstance()->Log(_level, std::move(string));
		}
	}
}
