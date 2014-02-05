//
//  RNLoggingEngine.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LOGGINGENGINE_H__
#define __RAYNE_LOGGINGENGINE_H__

#include "RNBase.h"
#include "RNLogging.h"
#include "RNObject.h"

namespace RN
{
	namespace Log
	{
		class LoggingEngine : public Object
		{
		public:
			RNAPI virtual void Open() = 0;
			RNAPI virtual void Close() = 0;
			RNAPI virtual bool IsOpen() const = 0;
			
			RNAPI virtual void CutOff() = 0;
			RNAPI virtual void Write(const Message& message) = 0;
			
			RNDeclareMeta(LoggingEngine, Object)
		};
		
		class StreamLoggingInternal;
		class StdoutLoggingEngine : public LoggingEngine, public ISingleton<StdoutLoggingEngine>
		{
		public:
			RNAPI StdoutLoggingEngine();
			
			RNAPI virtual void Open() final;
			RNAPI virtual void Close() final;
			RNAPI virtual bool IsOpen() const final;
			
			RNAPI virtual void CutOff() final;
			RNAPI virtual void Write(const Message& message) final;
			
		private:
			PIMPL<StreamLoggingInternal> _internal;
			
			RNDeclareMeta(StdoutLoggingEngine, LoggingEngine)
			RNDefineSingleton(StdoutLoggingEngine)
		};
		
		class SimpleLoggingEngine : public LoggingEngine, public ISingleton<SimpleLoggingEngine>
		{
		public:
			RNAPI SimpleLoggingEngine();
			
			RNAPI virtual void Open() final;
			RNAPI virtual void Close() final;
			RNAPI virtual bool IsOpen() const final;
			
			RNAPI virtual void CutOff() final;
			RNAPI virtual void Write(const Message& message) final;
			
		private:
			std::fstream _stream;
			PIMPL<StreamLoggingInternal> _internal;
			
			RNDeclareMeta(SimpleLoggingEngine, LoggingEngine)
			RNDefineSingleton(SimpleLoggingEngine)
		};
		
		class HTMLLoggingEngine : public LoggingEngine, public ISingleton<HTMLLoggingEngine>
		{
		public:
			RNAPI virtual void Open() final;
			RNAPI virtual void Close() final;
			RNAPI virtual bool IsOpen() const final;
			
			RNAPI virtual void CutOff() final;
			RNAPI virtual void Write(const Message& message) final;
			
		private:
			void WriteCSSBoilerplate();
			void SwitchMode(int mode);
			
			std::fstream _stream;
			int _mode;
			
			RNDeclareMeta(HTMLLoggingEngine, LoggingEngine)
			RNDefineSingleton(HTMLLoggingEngine)
		};
	}
}

#endif /* __RAYNE_LOGGINGENGINE_H__ */
