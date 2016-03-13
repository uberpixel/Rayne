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
	RNDefineMeta(StreamLoggingEngine, LoggingEngine)

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

	StreamLoggingEngine::StreamLoggingEngine() :
		StreamLoggingEngine(std::cout, true)
	{}

	StreamLoggingEngine::StreamLoggingEngine(std::ostream &stream, bool threadBound) :
		LoggingEngine(threadBound),
		_stream(stream),
		_open(true)
	{}

	void StreamLoggingEngine::Open()
	{
		_open = true;
	}
	void StreamLoggingEngine::Close()
	{
		_open = false;
		_stream.flush();
	}
	void StreamLoggingEngine::Flush()
	{
		_stream.flush();
	}

	bool StreamLoggingEngine::IsOpen() const
	{
		return _open;
	}

	void StreamLoggingEngine::Log(Logger::Level level, const LogMessage &message, const std::string &header)
	{
		_stream << header << ": " << kLogLevelStrings[static_cast<size_t>(level)] << " " << message.message << "\n";
	}
	void StreamLoggingEngine::LogBreak()
	{}
}
