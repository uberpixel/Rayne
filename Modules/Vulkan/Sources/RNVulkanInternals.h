//
//  RNVulkanInternals.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANINTERNALS_H__
#define __RAYNE_VULKANINTERNALS_H__

#include "RNVulkan.h"

namespace RN
{
	struct VulkanDrawable : public Drawable
	{
		VulkanDrawable *_next;
		VulkanDrawable *_prev;
	};

	struct VulkanRenderPass
	{
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		Matrix projectionViewMatrix;
		Matrix inverseProjectionViewMatrix;

		VulkanDrawable *drawableHead;
		size_t drawableCount;
	};

	struct VulkanRendererInternals
	{
		VulkanRenderPass renderPass;
	};
}

#endif /* __RAYNE_VULKANINTERNALS_H__ */
