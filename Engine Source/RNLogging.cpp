//
//  RNLogging.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		
		Message::Message() :
			_level(Level::Debug),
			_time(std::chrono::system_clock::now())
		{}
		
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
		
		void Message::SetMessage(const std::string& message)
		{
			_message = message;
		}
		void Message::SetMessage(std::string&& message)
		{
			_message = std::move(message);
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
		
		RNDefineSingleton(Logger)
		
		Logger::Logger()
		{
			AddLoggingEngine(StdoutLoggingEngine::GetSharedInstance());
			AddLoggingEngine(HTMLLoggingEngine::GetSharedInstance());
				
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
			
			_engines.Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {
				if(engine->IsOpen())
					engine->Close();
			});
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
			Message temp(level, message);
			Log(std::move(temp));
		}
		
		void Logger::Log(Level level, std::string&& message)
		{
			Message temp(level, std::move(message));
			Log(std::move(temp));
		}
		
		void Logger::Log(Level level, const char *message, ...)
		{
			char buffer[1024];
			va_list args;
			
			va_start(args, message);
			vsnprintf(buffer, 1024, message, args);
			va_end(args);
			
			
			Message temp(level, std::string(buffer));
			Log(std::move(temp));
		}
		
		void Logger::Log(const Message &message)
		{
			LockGuard<SpinLock> lock(_lock);
			bool knocked = false;
			
			while(!_buffer.push(message))
			{
				if(!knocked)
				{
					knocked = true;
					_signal.notify_one();
				}
			}
		}
		
		void Logger::Log(Message&& message)
		{
			LockGuard<SpinLock> lock(_lock);
			bool knocked = false;
			
			while(!_buffer.push(std::move(message)))
			{
				if(!knocked)
				{
					knocked = true;
					_signal.notify_one();
				}
			}
		}
		
		
		
		
		
		void Logger::Flush(bool force)
		{
			if(force)
			{
				LockGuard<SpinLock> temp(_lock);
				if(_buffer.was_empty())
					return;
				
				std::unique_lock<std::mutex> lock(_writeLock);
				
				_signal.notify_one();
				_writeSignal.wait(lock);
			}
			else
			{
				_signal.notify_one();
			}
		}
		
		
		void Logger::FlushRunLoop()
		{
			while(!_flushThread->IsCancelled())
			{
				std::unique_lock<std::mutex> lock(_signalLock);
				_signal.wait(lock);
				
				FlushBuffer();
			}
		}
		
		void Logger::FlushBuffer()
		{
			LockGuard<SpinLock> elock(_enginesLock);
			
			size_t engines = _engines.GetCount();
			Message message;
			
			while(_buffer.pop(message))
			{
				for(size_t i = 0; i < engines; i ++)
				{
					LoggingEngine *engine = static_cast<LoggingEngine *>(_engines[i]);
					
					long offset = std::chrono::duration_cast<std::chrono::seconds>(message.GetTime() - _lastMessage).count();
					_lastMessage = message.GetTime();
					
					if(offset >= 10)
						engine->CutOff();
					
					if(message.GetMessage().length() == 0)
					{
						engine->CutOff();
						continue;
					}
					
					if(message.GetLevel() >= engine->GetLevel())
						engine->Write(message);
				}
			}
			
			
			std::lock_guard<std::mutex> lock(_writeLock);
			_writeSignal.notify_one();
		}
		
		// ---------------------
		// MARK: -
		// MARK: Loggable
		// ---------------------
		
		Loggable::Loggable(Level level) :
			_level(level),
			_message(level, "")
		{}
		
		Loggable::~Loggable()
		{
			Submit();
		}
		
		void Loggable::Submit()
		{
			if(!_stream.tellp())
				return;
			
			_message.SetMessage(std::move(_stream.str()));
			_stream.str("");
			
			Logger::GetSharedInstance()->Log(std::move(_message));
			_message = Message(_level, "");
		}
	}
}
