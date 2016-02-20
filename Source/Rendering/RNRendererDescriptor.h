//
//  RendererDescriptor.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_RENDERERDESCRIPTOR_H__
#define __RAYNE_RENDERERDESCRIPTOR_H__

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"
#include "../Objects/RNString.h"
#include "RNRenderingDevice.h"

namespace RN
{
	class Renderer;
	class RendererDescriptor : public Object
	{
	public:
		RNAPI ~RendererDescriptor();

		RNAPI virtual Renderer *CreateRenderer(RenderingDevice *device) = 0;
		RNAPI virtual bool CanCreateRenderer() const = 0;

		RNAPI virtual const Array *GetDevices() const = 0;

		RNAPI virtual void PrepareWithSettings(const Dictionary *settings) = 0;

		const String *GetIdentifier() const { return _identifier; }
		const String *GetAPI() const { return _api; }

	protected:
		RNAPI RendererDescriptor(const String *identifier, const String *api);

	private:
		String *_identifier;
		String *_api;

		RNDeclareMeta(RendererDescriptor)
	};
}

#endif /* __RAYNE_RENDERERDESCRIPTOR_H__ */
