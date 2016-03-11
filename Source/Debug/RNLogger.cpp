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

#include "../Base/RNUnistd.h"

namespace RN
{
	static Logger *__sharedLogger = nullptr;

	void LogMessage::FormatTime()
	{
		if(!formattedTime.empty())
			return;

		std::stringstream stream;
		std::time_t timet = std::chrono::system_clock::to_time_t(time);

#if RN_PLATFORM_POSIX || __GNUC__
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

		formattedTime = std::move(stream.str());
	}


	Logger::Logger()
	{
		_queue = new RN::WorkQueue(RN::WorkQueue::Priority::Background, 0, RNCSTR("net.uberpixel.rayne.queue.logger"));
		_engines = new RN::Array();
		_threadEngines = new RN::Array();

		__sharedLogger = this;
	}

	Logger::~Logger()
	{
		__FlushQueue();
		_queue->Release();

		{
			std::lock_guard<std::mutex> lock(_engineLock);

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

	void Logger::__LoadDefaultLoggers()
	{
		Array *loggers = Settings::GetSharedInstance()->GetEntryForKey<Array>(kRNSettingsLoggersKey);
		if(!loggers)
			return;

		Catalogue *catalogue = Catalogue::GetSharedInstance();

		loggers->Enumerate<Dictionary>([&](Dictionary *entry, size_t index, bool &stop) {

			String *name = entry->GetObjectForKey<String>(RNCSTR("class"));
			if(!name)
				return;

			Number *tlevel = entry->GetObjectForKey<Number>(RNCSTR("level"));
			Level level = tlevel ? static_cast<Level>(tlevel->GetUint32Value()) : Level::Debug;

			MetaClass *meta = catalogue->GetClassWithName(name->GetUTF8String());
			if(meta && meta->SupportsConstruction())
			{
				LoggingEngine *engine = static_cast<LoggingEngine *>(meta->Construct());
				engine->SetLevel(level);

				AddEngine(engine);

				engine->Autorelease();
			}
		});
	}

	void Logger::AddEngine(LoggingEngine *engine)
	{
		std::lock_guard<std::mutex> lock(_engineLock);

		bool bound = engine->IsThreadBound();

		if(bound)
			_threadEngines->AddObject(engine);
		else
			_engines->AddObject(engine);

		engine->Flush();
		engine->Open();
	}

	void Logger::RemoveEngine(LoggingEngine *engine)
	{
		std::lock_guard<std::mutex> lock(_engineLock);

		bool bound = engine->IsThreadBound();

		if(bound)
			_threadEngines->RemoveObject(engine);
		else
			_engines->RemoveObject(engine);

		engine->Flush();
		engine->Close();
	}

	void Logger::Log(Level level, LogMessage &&message)
	{
		{
			std::lock_guard<std::mutex> lock(_engineLock);

			if(_threadEngines->GetCount() > 0)
			{
				message.FormatTime();

				_threadEngines->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {
					engine->Log(level, message);
				});
			}
		}

		{
			LockGuard<SpinLock> lock(_lock);
			while(!_messages.Push(std::move(LogEntry(level, std::move(message)))))
				_queue->PerformSynchronous([this]{ __FlushQueue(); });
		}

		bool wanted = false;
		if(_queueMarked.compare_exchange_strong(wanted, true, std::memory_order_acq_rel))
			_queue->Perform([this]{ __FlushQueue(); });
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

		LogMessage message(line, file, function, buffer);

		Log(level, std::move(message));
	}

	void Logger::Flush()
	{
		std::lock_guard<std::mutex> lock(_engineLock);

		_threadEngines->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {
			engine->Flush();
		});

		_engines->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {
			engine->Flush();
		});
	}

	void Logger::__FlushQueue()
	{
		std::lock_guard<std::mutex> lock(_engineLock);
		LogEntry entry;

		while(_messages.Pop(entry))
		{
			entry.message.FormatTime();

			_engines->Enumerate<LoggingEngine>([&](LoggingEngine *engine, size_t index, bool &stop) {

				auto offset = std::chrono::duration_cast<std::chrono::seconds>(entry.message.time - _lastMessage).count();
				_lastMessage = entry.message.time;

				if(offset >= 10)
					engine->LogBreak();

				engine->Log(entry.level, entry.message);
			});
		}

		_queueMarked.store(false, std::memory_order_release);

		if(!_messages.WasEmpty())
		{
			_queueMarked.store(true, std::memory_order_release);
			_queue->Perform([this]{ __FlushQueue(); });
		}
	}


	LogBuilder::LogBuilder(size_t line, const char *file, const char *function, Logger::Level level) :
		_level(level),
		_line(line),
		_file(file),
		_function(function)
	{}

	LogBuilder::~LogBuilder()
	{
		Submit();
	}

	void LogBuilder::Submit()
	{
		if(!_stream.tellp())
			return;

		LogMessage message(_line, _file, _function, std::move(_stream.str()));
		__sharedLogger->Log(_level, std::move(message));
		_stream.str("");
	}
}
