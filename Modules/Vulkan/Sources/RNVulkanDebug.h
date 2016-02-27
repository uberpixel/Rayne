//
//  RNVulkanDebug.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANDEBUG_H_
#define __RAYNE_VULKANDEBUG_H_

#include "RNVulkan.h"

namespace RN
{
	bool SetupVulkanDebugging(VkInstance instance);
	std::vector<const char *> DebugLayers();
}

#endif /* __RAYNE_VULKANDEBUG_H_ */
