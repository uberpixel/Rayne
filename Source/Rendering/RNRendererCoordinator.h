//
//  RNRendererCoordinator.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERERCOORDINATOR_H_
#define __RAYNE_RENDERERCOORDINATOR_H_

#include "../Base/RNBase.h"
#include "../Objects/RNArray.h"
#include "RNRendererDescriptor.h"

namespace RN
{
	class RendererCoordinator
	{
	public:
		friend class Kernel;

		RNAPI static RendererCoordinator *GetSharedInstance();

		RNAPI void AddDescriptor(RendererDescriptor *descriptor);
		RNAPI void RemoveDescriptor(RendererDescriptor *descriptor);

		RNAPI Array *GetRenderers() const;
		RNAPI Array *GetAvailableRenderers() const;

		RNAPI RendererDescriptor *GetPreferredRenderer() const;
		RNAPI RendererDescriptor *GetRendererWithIdentifier(const String *identifier) const;

	private:
		RendererCoordinator();
		~RendererCoordinator();

		Dictionary *GetSettings() const;
		Dictionary *GetParameters() const;

		RendererDescriptor *__GetRendererWithIdentifier(const String *identifier) const;
		Renderer *ActivateRenderer(RendererDescriptor *descriptor);

		mutable std::mutex _lock;
		Array *_descriptors;
	};
}


#endif /* __RAYNE_RENDERERCOORDINATOR_H_ */
