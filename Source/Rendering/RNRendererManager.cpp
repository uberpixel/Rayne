//
//  RNRendererManager.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNSettings.h"
#include "../Base/RNKernel.h"
#include "../Base/RNApplication.h"
#include "RNRendererManager.h"
#include "RNRenderer.h"

namespace RN
{
	static RendererManager *__sharedInstance;

	RendererManager::RendererManager() :
		_descriptors(new Array())
	{
		__sharedInstance = this;
	}

	RendererManager::~RendererManager()
	{
		SafeRelease(_descriptors);
		__sharedInstance = nullptr;
	}

	RendererManager *RendererManager::GetSharedInstance()
	{
		return __sharedInstance;
	}


	Dictionary *RendererManager::GetSettings() const
	{
		Dictionary *renderer = Settings::GetSharedInstance()->GetEntryForKey<Dictionary>(RNCSTR("RNRenderer"));
		if(renderer)
			return renderer;

		return (new Dictionary())->Autorelease();
	}

	Dictionary *RendererManager::GetParameters() const
	{
		Dictionary *settings = GetSettings();
		return settings->GetObjectForKey<Dictionary>(RNCSTR("parameters"));
	}



	void RendererManager::AddDescriptor(RendererDescriptor *descriptor)
	{
		{
			std::lock_guard<std::mutex> lock(_lock);
			_descriptors->AddObject(descriptor);
		}

		Dictionary *settings = GetParameters();
		descriptor->PrepareWithSettings(settings);
	}
	void RendererManager::RemoveDescriptor(RendererDescriptor *descriptor)
	{
		std::lock_guard<std::mutex> lock(_lock);
		_descriptors->RemoveObject(descriptor);
	}


	Array *RendererManager::GetRenderers() const
	{
		std::lock_guard<std::mutex> lock(_lock);
		return _descriptors->Copy()->Autorelease();
	}

	Array *RendererManager::GetAvailableRenderers() const
	{
		std::lock_guard<std::mutex> lock(_lock);

		return _descriptors->GetObjectsPassingTest<RendererDescriptor>([&](RendererDescriptor *descriptor, bool &stop) -> bool {
			return descriptor->CanCreateRenderer();
		});
	}

	RendererDescriptor *RendererManager::GetPreferredRenderer() const
	{
		std::lock_guard<std::mutex> lock(_lock);

		Dictionary *settings = GetSettings();
		String *preferredIdentifier = settings->GetObjectForKey<String>(RNCSTR("identifier"));

		if(preferredIdentifier)
			return __GetRendererWithIdentifier(preferredIdentifier);

		return nullptr;
	}

	RendererDescriptor *RendererManager::GetRendererWithIdentifier(const String *identifier) const
	{
		std::lock_guard<std::mutex> lock(_lock);
		return __GetRendererWithIdentifier(identifier);
	}

	RendererDescriptor *RendererManager::__GetRendererWithIdentifier(const String *identifier) const
	{
		RendererDescriptor *descriptor = nullptr;

		_descriptors->Enumerate<RendererDescriptor>([&](RendererDescriptor *tdescriptor, size_t index, bool &stop) {

			if(tdescriptor->GetIdentifier()->IsEqual(identifier))
			{
				stop = true;
				descriptor = tdescriptor;
			}

		});

		return descriptor;
	}

	Renderer *RendererManager::ActivateRenderer(RendererDescriptor *descriptor)
	{
		Dictionary *parameters = GetParameters();

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
