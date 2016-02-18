//
//  RNVulkanRendererDescriptor.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANRENDERERDESCRIPTOR_H_
#define __RAYNE_VULKANRENDERERDESCRIPTOR_H_

#include <Rayne.h>

namespace RN
{
	class VulkanRendererDescriptor : public RendererDescriptor
	{
	public:
		static void InitialWakeUp(MetaClass *meta);

		Renderer *CreateRenderer(const Dictionary *parameters) override;
		bool CanConstructWithSettings(const Dictionary *parameters) const override;

	private:
		VulkanRendererDescriptor();

		RNDeclareMeta(VulkanRendererDescriptor)
	};
}


#endif /* __RAYNE_VULKANRENDERERDESCRIPTOR_H_ */
