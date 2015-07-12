//
//  RNLoggingEngine.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLoggingEngine.h"

namespace RN
{
	RNDefineMeta(LoggingEngine, Object)
	RNDefineMeta(STDOUTLoggingEngine, LoggingEngine)

	LoggingEngine::LoggingEngine(bool threadBound) :
		_level(Logger::Level::Info),
		_threadBound(threadBound)
	{}

	void LoggingEngine::SetLevel(Logger::Level level)
	{
		_level.store(level, std::memory_order_release);
	}


	const char *kLogLevelStrings[] = {
		"(dbg)",
		"(info)",
		"(warn)",
		"(error)"
	};

	STDOUTLoggingEngine::STDOUTLoggingEngine() :
		LoggingEngine(true),
		_stream(std::cout),
		_open(true)
	{}

	void STDOUTLoggingEngine::Open()
	{
		_open = true;
	}
	void STDOUTLoggingEngine::Close()
	{
		_open = false;
		_stream.flush();
	}
	void STDOUTLoggingEngine::Flush()
	{
		_stream.flush();
	}

	bool STDOUTLoggingEngine::IsOpen() const
	{
		return _open;
	}

	void STDOUTLoggingEngine::Log(Logger::Level level, const LogMessage &message)
	{
		if(static_cast<size_t>(level) < static_cast<size_t>(_level.load(std::memory_order_acquire)))
			return;

		_stream << message.formattedTime << ": " << kLogLevelStrings[static_cast<size_t>(level)] << " " << message.GetMessage() << "\n";
	}
	void STDOUTLoggingEngine::LogBreak()
	{}
}
