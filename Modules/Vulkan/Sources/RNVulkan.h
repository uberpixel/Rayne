//
//  RNVulkan.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKAN_H_
#define __RAYNE_VULKAN_H_

#include <Rayne.h>

#if defined(RN_BUILD_VULKAN)
	#define VKAPI RN_EXPORT
#else
	#define VKAPI RN_IMPORT
#endif

#endif /* __RAYNE_VULKAN_H_ */
