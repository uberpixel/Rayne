//
//  RNLogFormatter.cpp
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNLogFormatter.h"

namespace RN
{
	RNDefineMeta(LogFormatter, Object)

	static std::string FormatTime(const std::chrono::system_clock::time_point &time)
	{
		std::stringstream stream;
		std::time_t timet = std::chrono::system_clock::to_time_t(time);

#if RN_PLATFORM_POSIX
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

		return stream.str();
	}

	String *DebugLogFormatter::FormatLogMessage(const LogMessage &message) const
	{
		std::string time = FormatTime(message.time);
		return RNSTR(time << " " << message.GetFileName() << ":" << message.line << " " << message.message);
	}
}
