//
//  RNLogFormatter.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LOGFORMATTER_H__
#define __RAYNE_LOGFORMATTER_H__

#include "../Base/RNBase.h"
#include "../Objects/RNString.h"
#include "RNLogger.h"

namespace RN
{
	class LogFormatter : public Object
	{
	public:
		LogFormatter() = default;
		virtual ~LogFormatter() = default;

		RNAPI virtual String *FormatLogMessage(const LogMessage &message) const = 0;

		__RNDeclareMetaInternal(LogFormatter)
	};

	class DebugLogFormatter : public LogFormatter
	{
	public:
		RNAPI String *FormatLogMessage(const LogMessage &message) const final;

		__RNDeclareMetaInternal(DebugLogFormatter)
	};
}


#endif /* __RAYNE_LOGFORMATTER_H__ */
