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

	protected:
		RNAPI virtual void Open() = 0;
		RNAPI virtual void Close() = 0;
		RNAPI virtual void Flush() = 0;

		RNAPI virtual bool IsOpen() const = 0;
		bool IsThreadBound() const { return _threadBound; }

		RNAPI virtual void Log(Logger::Level level, const LogMessage &message) = 0;
		RNAPI virtual void LogBreak() = 0;

		RNAPI LoggingEngine(bool threadBound);

		std::atomic<Logger::Level> _level;

	private:
		bool _threadBound;

		RNDeclareMeta(LoggingEngine)
	};

	class STDOUTLoggingEngine : public LoggingEngine
	{
	public:
		RNAPI STDOUTLoggingEngine();

		RNAPI void Open() final;
		RNAPI void Close() final;
		RNAPI void Flush() final;

		RNAPI bool IsOpen() const final;

		RNAPI void Log(Logger::Level level, const LogMessage &message) final;
		RNAPI void LogBreak() final;

	private:
		bool _open;
		std::ostream &_stream;

		RNDeclareMeta(STDOUTLoggingEngine)
	};
}


#endif /* __RAYNE_LOGGINGENGINE_H_ */
