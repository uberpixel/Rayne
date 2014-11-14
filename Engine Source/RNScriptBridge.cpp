//
//  RNScriptBridge.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNScriptBridge.h"
#include "RNLogging.h"

namespace RN
{
	void __RNDebugShim(const std::string &string)
	{
		RNDebug("%s", string.c_str());
	}
	void __RNInfoShim(const std::string &string)
	{
		RNInfo("%s", string.c_str());
	}
	void __RNWarningShim(const std::string &string)
	{
		RNWarning("%s", string.c_str());
	}
	void __RNErrorShim(const std::string &string)
	{
		RNError("%s", string.c_str());
	}
}
