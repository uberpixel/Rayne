//
//  RNLoggingEngine.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_LOGGINGENGINE_H_
#define __RAYNE_LOGGINGENGINE_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "RNLogger.h"
#include "RNLogFormatter.h"

namespace RN
{
	class LoggingEngine : public Object
	{
	public:
		friend class Logger;

		RNAPI virtual void Open() = 0;
		RNAPI virtual void Close() = 0;
		RNAPI virtual void Flush() = 0;

		RNAPI void SetLogFormatter(LogFormatter *formatter);

		RNAPI virtual bool IsOpen() const = 0;
		bool IsThreadBound() const { return _threadBound; }
		const LogFormatter *GetFormatter() const { return _formatter; }

		RNAPI virtual void Log(const String *message) = 0;
		RNAPI virtual void LogBreak() = 0;

	protected:
		RNAPI LoggingEngine(bool threadBound);
		RNAPI ~LoggingEngine();

	private:
		bool _threadBound;
		LogFormatter *_formatter;
		Logger::Level _level;

		__RNDeclareMetaInternal(LoggingEngine)
	};

	class StreamLoggingEngine : public LoggingEngine
	{
	public:
		RNAPI StreamLoggingEngine();
		RNAPI StreamLoggingEngine(std::ostream &stream, bool threadBound);

		RNAPI void Open() final;
		RNAPI void Close() final;
		RNAPI void Flush() final;

		RNAPI bool IsOpen() const final;

		RNAPI void Log(const String *message) final;
		RNAPI void LogBreak() final;

	private:
		bool _open;
		std::ostream &_stream;

		__RNDeclareMetaInternal(StreamLoggingEngine)
	};

#if RN_PLATFORM_WINDOWS
	class WideCharStreamLoggingEngine : public LoggingEngine
	{
	public:
		RNAPI WideCharStreamLoggingEngine();
		RNAPI WideCharStreamLoggingEngine(std::wostream &stream, bool threadBound);

		RNAPI void Open() final;
		RNAPI void Close() final;
		RNAPI void Flush() final;

		RNAPI bool IsOpen() const final;

		RNAPI void Log(const String *message) final;
		RNAPI void LogBreak() final;

	private:
		void Log(const char *string);

		bool _open;
		std::wostream &_stream;

		__RNDeclareMetaInternal(WideCharStreamLoggingEngine)
	};
#endif
}


#endif /* __RAYNE_LOGGINGENGINE_H_ */
