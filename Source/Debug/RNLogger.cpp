//
//  RNLogging.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLogger.h"
#include "RNLoggingEngine.h"
#include "../Base/RNSettings.h"
#include "../Threads/RNWorkGroup.h"
#include "../Base/RNUnistd.h"

namespace RN
{
	static Logger *__sharedLogger = nullptr;

	static std::string FormatTime(std::chrono::system_clock::time_point &time)
	{
		std::stringstream stream;
		std::time_t timet = std::chrono::system_clock::to_time_t(time);

#if RN_PLATFORM_POSIX || RN_COMPILER_GCC
		std::tm tm = std::tm{0};
		localtime_r(&timet, &tm);
#else
		std::tm tm = *localtime(&timet);
#endif

		std::chrono::duration<double> sec = time - std::chrono::system_clock::from_time_t(timet) + std::chrono::seconds(tm.tm_sec);


		stream.precision(3);
		stream << tm.tm_mon + 1 << '/' << tm.tm_mday << '/' << tm.tm_year - 100 << ' ';

#define PutTwoDigitNumber(n) if(n < 10) stream << '0'; stream << std::fixed << n

		PutTwoDigitNumber(tm.tm_hour) << ':';
		PutTwoDigitNumber(tm.tm_min) << ':';
		PutTwoDigitNumber(sec.count());
#undef PutTwoDigitNumber

		return std::move(stream.str());
	}



	Logger::Logger()
	{
		_queue = new WorkQueue(WorkQueue::Priority::Background, WorkQueue::Flags::Serial, RNCSTR("net.uberpixel.rayne.queue.logger"));
		_engines = new Array();
		_threadEngines = new Array();
		_flag.clear(std::memory_order_release);

		__sharedLogger = this;
	}

	Logger::~Logger()
	{
		__FlushQueue();
		_queue->Release();

		{
			LockGuard<Lockable> lock(_engineLock);

			_threadEngines->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {
				engine->Close();
			});

			_engines->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {
				engine->Close();
			});

			_threadEngines->RemoveAllObjects();
			_engines->RemoveAllObjects();
		}

		__sharedLogger = nullptr;
	}

	Logger *Logger::GetSharedInstance()
	{
		return __sharedLogger;
	}

	void Logger::AddEngine(LoggingEngine *engine)
	{
		LockGuard<Lockable> lock(_engineLock);

		bool bound = engine->IsThreadBound();

		if(bound)
			_threadEngines->AddObject(engine);
		else
			_engines->AddObject(engine);

		engine->Open();
	}

	void Logger::RemoveEngine(LoggingEngine *engine)
	{
		Flush(true);

		LockGuard<Lockable> lock(_engineLock);
		engine->Close();

		bool bound = engine->IsThreadBound();

		if(bound)
			_threadEngines->RemoveObject(engine);
		else
			_engines->RemoveObject(engine);
	}

	void Logger::Log(Level level, LogMessage &&message)
	{
		{
			LockGuard<Lockable> lock(_engineLock);

			if(_threadEngines->GetCount() > 0)
			{
				std::string time = FormatTime(message.time);

				_threadEngines->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {
					engine->Log(level, message, time);
				});
			}

			if(_engines->GetCount() == 0)
				return;
		}

		_queue->Perform([message{std::move(message)}, this, level]() mutable {

			{
				LogContainer container(std::move(message), level);

				LockGuard<Lockable> lock(_lock);

				while(!_messages.Push(std::move(container)))
				{
					lock.Unlock();
					__FlushQueue();
					lock.Lock();
				}
			}

			if(!_flag.test_and_set(std::memory_order_acq_rel))
				_queue->Perform([=]{ __FlushQueue(); });

		});
	}

	void Logger::Log(Level level, size_t line, const char *file, const char *function, const char *format, ...)
	{
		va_list args;
		va_start(args, format);
		ssize_t size = vsnprintf(nullptr, 0, format, args);
		va_end(args);

		char *buffer = new char[size + 1];

		va_start(args, format);
		vsnprintf(buffer, static_cast<size_t>(size + 1), format, args);
		va_end(args);

		LogMessage message(line, file, function, RNSTR(buffer));
		Log(level, std::move(message));
	}

	void Logger::Log(Level level, size_t line, const char *file, const char *function, const String *string)
	{
		LogMessage message(line, file, function, string);
		Log(level, std::move(message));
	}

	void Logger::Flush(bool synchronous)
	{
		Array *engines;

		{
			LockGuard<Lockable> lock(_engineLock);
			engines = _engines->Copy();
		}

		Function work([=]() {

			engines->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {
				engine->Flush();
			});

			engines->Release();

		});

		synchronous ? _queue->PerformSynchronousBarrier(std::move(work)) : _queue->PerformBarrier(std::move(work));
	}

	void Logger::__FlushQueue()
	{
		if(RN_EXPECT_FALSE(_messages.WasEmpty()))
			return;

		Array *engines;

		{
			LockGuard<Lockable> lock(_engineLock);
			engines = _engines->Copy();
		}

		std::vector<LogContainer> *messages = new std::vector<LogContainer>();
		{
			LogContainer message;

			while(_messages.Pop(message))
				messages->push_back(std::move(message));
		}

		WorkGroup *group = new WorkGroup();

		group->Perform(_queue, [messages]() {

			for(auto iterator = messages->begin(); iterator != messages->end(); iterator ++)
			{
				LogContainer &container = *iterator;
				container.FormatTime();
			}

		});


		engines->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {

			group->Perform(_queue, [engine, messages]() {

				for(auto iterator = messages->begin(); iterator != messages->end(); iterator ++)
				{
					LogContainer &container = *iterator;
					engine->Log(container.level, container.message, container.formattedTime);
				}

			});

		});


		group->Notify(_queue, [=]() {

			delete messages;
			engines->Release();

			_flag.clear(std::memory_order_release);

			if(!_messages.WasEmpty())
				__FlushQueue();

		});

		group->Release();
	}



	void Logger::LogContainer::FormatTime()
	{
		if(!formattedTime.empty())
			return;

		formattedTime = std::move(RN::FormatTime(message.time));
	}
}
