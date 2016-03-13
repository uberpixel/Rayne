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


namespace RN
{
	class LoggingEngine : public Object
	{
	public:
		friend class Logger;

		RNAPI void SetLevel(Logger::Level level);

		RNAPI virtual void Open() = 0;
		RNAPI virtual void Close() = 0;
		RNAPI virtual void Flush() = 0;

		RNAPI virtual bool IsOpen() const = 0;
		bool IsThreadBound() const { return _threadBound; }
		Logger::Level GetLevel() const { return _level.load(std::memory_order_acquire); }

		RNAPI virtual void Log(Logger::Level level, const LogMessage &message, const std::string &header) = 0;
		RNAPI virtual void LogBreak() = 0;

	protected:
		RNAPI LoggingEngine(bool threadBound);

	private:
		bool _threadBound;
		std::atomic<Logger::Level> _level;

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

		RNAPI void Log(Logger::Level level, const LogMessage &message, const std::string &header) final;
		RNAPI void LogBreak() final;

	private:
		bool _open;
		std::ostream &_stream;

		__RNDeclareMetaInternal(StreamLoggingEngine)
	};
}


#endif /* __RAYNE_LOGGINGENGINE_H_ */
