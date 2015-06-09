//
//  RNMetalRendererDescriptor.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_METALRENDERERDESCRIPTOR_H__
#define __RAYNE_METALRENDERERDESCRIPTOR_H__

#include "../../Base/RNBase.h"
#include "../RNRenderingDescriptor.h"

namespace RN
{
	class MetalRendererDescriptor : public RenderingDescriptor
	{
	public:
		MetalRendererDescriptor();

		Renderer *CreateAndSetActiveRenderer() final;
	};
}


#endif /* __RAYNE_METALRENDERERDESCRIPTOR_H__ */
