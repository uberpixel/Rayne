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

	void Logger::AddEngine(LoggingEngine *engine, Level level)
	{
		LockGuard<Lockable> lock(_engineLock);

		bool bound = engine->IsThreadBound();

		if(bound)
			_threadEngines->AddObject(engine);
		else
			_engines->AddObject(engine);

		engine->_level = level;
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
				_threadEngines->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {

					const LogFormatter *formatter = engine->GetFormatter();
					String *formatted = message.message;

					if(formatter)
						formatted = formatter->FormatLogMessage(message);

					engine->Log(formatted);

				});
			}

			if(_engines->GetCount() == 0)
				return;
		}

		_queue->Perform([message{std::move(message)}, this]() mutable {

			{
				UniqueLock<Lockable> lock(_lock);

				while(!_messages.Push(std::move(message)))
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
		
		delete[] buffer;
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

		std::vector<LogMessage> *messages = new std::vector<LogMessage>();
		{
			LogMessage message;

			while(_messages.Pop(message))
				messages->push_back(std::move(message));
		}

		WorkGroup *group = new WorkGroup();

		engines->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {

			group->Perform(_queue, [engine, messages]() {

				const LogFormatter *formatter = engine->GetFormatter();

				for(auto iterator = messages->begin(); iterator != messages->end(); iterator ++)
				{
					String *formatted = iterator->message;

					if(formatter)
						formatted = formatter->FormatLogMessage(*iterator);

					engine->Log(formatted);
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
}
