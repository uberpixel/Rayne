//
//  RNLogging.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLogger.h"
#include "../Base/RNSettings.h"
#include "../Base/RNUnistd.h"
#include "RNLoggingEngine.h"

namespace RN
{
	static Logger *__sharedLogger = nullptr;

	thread_local bool Logger::_callback = false;

	Logger::CallbackScope::CallbackScope() :
		previous(_callback)
	{
		_callback = true;
	}

	Logger::CallbackScope::~CallbackScope()
	{
		_callback = previous;
	}

	Logger::Logger() :
		_draining(false), _hasAsyncEngines(false)
	{
		_queue = new WorkQueue(WorkQueue::Priority::Background, WorkQueue::Flags::Serial, RNCSTR("RN::Logger"));
		__sharedLogger = this;
	}

	Logger::~Logger()
	{
		bool needsAsyncShutdown;
		{
			LockGuard<Lockable> lock(_lock);
			needsAsyncShutdown = _draining || _hasAsyncEngines.load();
		}
		if(needsAsyncShutdown)
		{
			Perform([this] {
				for(auto &engine : _engines)
				{
					try
					{
						engine->Flush();
					}
					catch(...)
					{}
					try
					{
						engine->Close();
					}
					catch(...)
					{}
				}
				_engines.clear();
				_hasAsyncEngines.store(false);
			},
					true);
		}
		_queue->Release();
		{
			CallbackScope scope;
			LockGuard<Lockable> lock(_engineLock);
			for(auto &engine : _threadEngines)
			{
				try
				{
					engine->Close();
				}
				catch(...)
				{}
			}
			_threadEngines.clear();
		}
		__sharedLogger = nullptr;
	}

	Logger *Logger::GetSharedInstance()
	{
		return __sharedLogger;
	}

	void Logger::AddEngine(LoggingEngine *engine, Level level)
	{
		if(_callback) throw InconsistencyException("Cannot add a logging engine from a logging callback");
		CallbackScope scope;
		auto add = [this, engine = StrongRef<LoggingEngine>(engine), level] {
			auto &engines = engine->IsThreadBound() ? _threadEngines : _engines;
			for(auto &registered : engines)
			{
				if(registered.Get() == engine.Get()) return;
			}
			engines.reserve(engines.size() + 1);
			engine->_level = level;
			engine->Open();
			engines.push_back(engine);
			if(!engine->IsThreadBound()) _hasAsyncEngines.store(true);
		};
		if(engine->IsThreadBound())
		{
			LockGuard<Lockable> lock(_engineLock);
			add();
		}
		else
			Perform(std::move(add), true);
	}

	void Logger::RemoveEngine(LoggingEngine *engine)
	{
		if(_callback) throw InconsistencyException("Cannot remove a logging engine from a logging callback");
		CallbackScope scope;
		auto remove = [this, engine = StrongRef<LoggingEngine>(engine)] {
			bool bound = engine->IsThreadBound();
			auto &engines = bound ? _threadEngines : _engines;
			auto iterator = engines.begin();
			while(iterator != engines.end() && iterator->Get() != engine.Get()) ++iterator;
			if(iterator == engines.end()) return;
			engines.erase(iterator);
			if(!bound)
			{
				_hasAsyncEngines.store(!engines.empty());
				try
				{
					engine->Flush();
				}
				catch(...)
				{
					try
					{
						engine->Close();
					}
					catch(...)
					{}
					throw;
				}
			}
			engine->Close();
		};
		if(engine->IsThreadBound())
		{
			LockGuard<Lockable> lock(_engineLock);
			remove();
		}
		else
			Perform(std::move(remove), true);
	}

	void Logger::Log(Level level, LogMessage &&message)
	{
		if(_callback) return;
#if !RN_BUILD_DEBUG
		//TODO: The application should control the logging level! In general the level is just completely ignored at the moment, but shouldn't be.
		if(level == Level::Debug) //Early out if the logging level is Debug, but it isn't a debug build
		{
			return;
		}
#endif
		{
			CallbackScope scope;
			LockGuard<Lockable> lock(_engineLock);
			for(auto &engine : _threadEngines)
			{
				const LogFormatter *formatter = engine->GetFormatter();
				String *formatted = formatter ? formatter->FormatLogMessage(message) : message.message;
				engine->Log(formatted);
			}
		}
		if(!_hasAsyncEngines.load()) return;
		Enqueue([message {std::move(message)}, this]() mutable {
			for(auto &engine : _engines)
			{
				const LogFormatter *formatter = engine->GetFormatter();
				String *formatted = formatter ? formatter->FormatLogMessage(message) : message.message;
				engine->Log(formatted);
			}
		});
	}

	void Logger::Log(Level level, size_t line, const char *file, const char *function, const char *format, ...)
	{
		if(_callback) return;
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
		if(_callback) return;
		LogMessage message(line, file, function, string);
		Log(level, std::move(message));
	}

	void Logger::Flush(bool synchronous)
	{
		if(_callback) return;
		{
			LockGuard<Lockable> lock(_lock);
			if(!_draining && !_hasAsyncEngines.load()) return;
		}
		Perform([this] {
			for(auto &engine : _engines) engine->Flush();
		},
				synchronous);
	}

	void Logger::Perform(Function &&work, bool synchronous)
	{
		// Capture destruction on the logger worker must never wait for itself.
		if(!synchronous || WorkQueue::GetCurrentWorkQueue() == _queue)
		{
			Enqueue(std::move(work));
			return;
		}
		std::promise<void> promise;
		std::future<void> future = promise.get_future();
		Enqueue([work = std::move(work), promise = std::move(promise)]() mutable {
			try
			{
				work();
				promise.set_value();
			}
			catch(...)
			{
				promise.set_exception(std::current_exception());
			}
		});
		future.get();
	}

	void Logger::Enqueue(Function &&command)
	{
		UniqueLock<Lockable> lock(_lock);
		_commands.push_back(std::move(command));
		if(_draining) return;
		try
		{
			_queue->Perform([this] { Drain(); });
			_draining = true;
		}
		catch(...)
		{
			Function rejected = std::move(_commands.back());
			_commands.pop_back();
			lock.Unlock();
			throw;
		}
	}

	void Logger::Drain()
	{
		// Keep callback suppression active through command capture destruction.
		CallbackScope scope;
		while(true)
		{
			Function command;
			{
				LockGuard<Lockable> lock(_lock);
				if(_commands.empty())
				{
					_draining = false;
					return;
				}
				command = std::move(_commands.front());
				_commands.pop_front();
			}
			// One failing sink must not strand the remaining accepted commands.
			try
			{
				command();
			}
			catch(...)
			{}
		}
	}
} // namespace RN
