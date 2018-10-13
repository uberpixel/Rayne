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
#include "../Modules/RNExtensionPoint.h"
#include "RNRenderingDevice.h"

namespace RN
{
	class Renderer;
	class RendererDescriptor : public Object
	{
	public:
		friend class Kernel;

		RNAPI static ExtensionPoint<RendererDescriptor> *GetExtensionPoint();

		RNAPI ~RendererDescriptor();

		RNAPI virtual Renderer *CreateRenderer(RenderingDevice *device) = 0;
		RNAPI virtual bool CanCreateRenderer() const = 0;

		RNAPI virtual const Array *GetDevices() const = 0;

		RNAPI virtual void PrepareWithSettings(const Dictionary *settings) = 0;

		const String *GetIdentifier() const { return _identifier; }
		const String *GetAPI() const { return _api; }

		RNAPI static Array *GetRenderers();
		RNAPI static Array *GetAvailableRenderers(Dictionary *additionalSettings = nullptr);

		RNAPI static RendererDescriptor *GetPreferredRenderer(Dictionary *additionalSettings = nullptr);
		RNAPI static RendererDescriptor *GetRendererWithIdentifier(const String *identifier, Dictionary *additionalSettings = nullptr);

	protected:
		RNAPI RendererDescriptor(const String *identifier, const String *api);

	private:
		static Dictionary *GetSettings();
		static Dictionary *GetParameters(Dictionary *additionalSettings);

		static RendererDescriptor *__GetRendererWithIdentifier(const String *identifier, Dictionary *additionalSettings);
		static Renderer *ActivateRenderer(RendererDescriptor *descriptor, Dictionary *additionalSettings = nullptr);

		void __PrepareWithSettings(const Dictionary *settings);

		String *_identifier;
		String *_api;
		bool _isPrepared;

		__RNDeclareMetaInternal(RendererDescriptor)
	};
}

#endif /* __RAYNE_RENDERERDESCRIPTOR_H__ */
