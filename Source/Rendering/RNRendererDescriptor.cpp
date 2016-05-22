//
//  RendererDescriptor.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRendererDescriptor.h"
#include "RNRenderer.h"
#include "../Base/RNKernel.h"

namespace RN
{
	RNDefineMeta(RendererDescriptor, Object)

	static ExtensionPoint<RendererDescriptor> _extensionPoint("net.uberpixel.rayne.renderer_descriptor");

	ExtensionPoint<RendererDescriptor> *RendererDescriptor::GetExtensionPoint()
	{
		return &_extensionPoint;
	}

	RendererDescriptor::RendererDescriptor(const String *identifier, const String *api) :
		_identifier(identifier->Copy()),
		_api(api->Copy()),
		_isPrepared(false)
	{}

	RendererDescriptor::~RendererDescriptor()
	{
		_identifier->Release();
		_api->Release();
	}

	void RendererDescriptor::__PrepareWithSettings(const Dictionary *settings)
	{
		if(!_isPrepared)
		{
			_isPrepared = true;
			PrepareWithSettings(settings);
		}
	}





	Dictionary *RendererDescriptor::GetSettings()
	{
		Dictionary *renderer = Settings::GetSharedInstance()->GetEntryForKey<Dictionary>(RNCSTR("RNRenderer"));
		if(renderer)
			return renderer;

		return (new Dictionary())->Autorelease();
	}

	Dictionary *RendererDescriptor::GetParameters()
	{
		Dictionary *settings = GetSettings();
		return settings->GetObjectForKey<Dictionary>(RNCSTR("parameters"));
	}



	Array *RendererDescriptor::GetRenderers()
	{
		return RendererDescriptor::GetExtensionPoint()->GetExtensions();
	}

	Array *RendererDescriptor::GetAvailableRenderers()
	{
		Dictionary *settings = GetParameters();

		return GetRenderers()->GetObjectsPassingTest<RendererDescriptor>([&](RendererDescriptor *descriptor, bool &stop) -> bool {

			descriptor->__PrepareWithSettings(settings);
			return descriptor->CanCreateRenderer();

		});
	}

	RendererDescriptor *RendererDescriptor::GetPreferredRenderer()
	{
		Dictionary *settings = GetSettings();
		String *preferredIdentifier = settings->GetObjectForKey<String>(RNCSTR("identifier"));

		if(preferredIdentifier)
			return __GetRendererWithIdentifier(preferredIdentifier);

		return nullptr;
	}

	RendererDescriptor *RendererDescriptor::GetRendererWithIdentifier(const String *identifier)
	{
		return __GetRendererWithIdentifier(identifier);
	}

	RendererDescriptor *RendererDescriptor::__GetRendererWithIdentifier(const String *identifier)
	{
		RendererDescriptor *descriptor = nullptr;

		GetRenderers()->Enumerate<RendererDescriptor>([&](RendererDescriptor *tdescriptor, size_t index, bool &stop) {

			if(tdescriptor->GetIdentifier()->IsEqual(identifier))
			{
				stop = true;
				descriptor = tdescriptor;
			}

		});

		if(descriptor)
			descriptor->__PrepareWithSettings(GetParameters());

		return descriptor;
	}

	Renderer *RendererDescriptor::ActivateRenderer(RendererDescriptor *descriptor)
	{
		descriptor->__PrepareWithSettings(GetParameters());

		if(!descriptor->CanCreateRenderer())
			throw InconsistencyException("Tried to activate renderer that is not available");

		// Get the preferred device
		const Array *devices = descriptor->GetDevices();

		if(devices->GetCount() == 0)
			throw InconsistencyException("Tried to activate renderer without devices");

		RenderingDevice *device = Kernel::GetSharedInstance()->GetApplication()->GetPreferredRenderingDevice(devices);
		if(!device || !devices->ContainsObject(device))
			throw InconsistencyException("Invalid preferred rendering device");


		Renderer *renderer = nullptr;

		try
		{
			renderer = descriptor->CreateRenderer(device);
			renderer->Activate();

			RNInfo("Using renderer: " << renderer << ", device: " << device);
		}
		catch(...)
		{
			delete renderer;
			std::rethrow_exception(std::current_exception());
		}

		return renderer;
	}
}
