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
		_threadBound(threadBound),
		_formatter(nullptr)
	{}

	LoggingEngine::~LoggingEngine()
	{
		SafeRelease(_formatter);
	}

	void LoggingEngine::SetLogFormatter(LogFormatter *formatter)
	{
		SafeRelease(_formatter);
		_formatter = SafeRetain(formatter);
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
		_open(true),
		_stream(stream)
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

	void StreamLoggingEngine::Log(const String *message)
	{
		_stream << message->GetUTF8String() << "\n";
	}
	void StreamLoggingEngine::LogBreak()
	{}


#if RN_PLATFORM_WINDOWS
	RNDefineMeta(WideCharStreamLoggingEngine, LoggingEngine)

	WideCharStreamLoggingEngine::WideCharStreamLoggingEngine() :
		WideCharStreamLoggingEngine(std::wcout, true)
	{}

	WideCharStreamLoggingEngine::WideCharStreamLoggingEngine(std::wostream &stream, bool threadBound) :
		LoggingEngine(threadBound),
		_stream(stream),
		_open(true)
	{}

	void WideCharStreamLoggingEngine::Open()
	{
		_open = true;
	}
	void WideCharStreamLoggingEngine::Close()
	{
		_open = false;
		_stream.flush();
	}
	void WideCharStreamLoggingEngine::Flush()
	{
		_stream.flush();
	}

	bool WideCharStreamLoggingEngine::IsOpen() const
	{
		return _open;
	}

	void WideCharStreamLoggingEngine::Log(const String *message)
	{
		std::stringstream stream;
		stream << message->GetUTF8String() << "\n";

		Log(stream.str().c_str());
	}
	void WideCharStreamLoggingEngine::LogBreak()
	{}

	void WideCharStreamLoggingEngine::Log(const char *string)
	{
		wchar_t buffer[512];
		int result = ::MultiByteToWideChar(CP_UTF8, 0, string, -1, buffer, 512);

		if(result != 0)
		{
			_stream << buffer;
			return;
		}
		else
		{
			if(::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				wchar_t *allocBuffer = new wchar_t[result + 1];
				result = ::MultiByteToWideChar(CP_UTF8, 0, string, -1, allocBuffer, result + 1);

				if(result != 0)
					_stream << allocBuffer;

				delete[] allocBuffer;
			}
		}
	}
#endif
}
