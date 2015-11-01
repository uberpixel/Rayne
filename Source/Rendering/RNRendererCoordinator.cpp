//
//  RNRendererCoordinator.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNSettings.h"
#include "RNRendererCoordinator.h"
#include "RNRenderer.h"

namespace RN
{
	static RendererCoordinator *__sharedInstance;

	RendererCoordinator::RendererCoordinator() :
		_descriptors(new Array())
	{
		__sharedInstance = this;
	}

	RendererCoordinator::~RendererCoordinator()
	{
		SafeRelease(_descriptors);
		__sharedInstance = nullptr;
	}

	RendererCoordinator *RendererCoordinator::GetSharedInstance()
	{
		return __sharedInstance;
	}


	Dictionary *RendererCoordinator::GetSettings() const
	{
		Dictionary *renderer = Settings::GetSharedInstance()->GetEntryForKey<Dictionary>(RNCSTR("RNRenderer"));
		if(renderer)
			return renderer;

		return (new Dictionary())->Autorelease();
	}

	Dictionary *RendererCoordinator::GetParameters() const
	{
		Dictionary *settings = GetSettings();
		return settings->GetObjectForKey<Dictionary>(RNCSTR("parameters"));
	}



	void RendererCoordinator::AddDescriptor(RendererDescriptor *descriptor)
	{
		std::lock_guard<std::mutex> lock(_lock);
		_descriptors->AddObject(descriptor);
	}
	void RendererCoordinator::RemoveDescriptor(RendererDescriptor *descriptor)
	{
		std::lock_guard<std::mutex> lock(_lock);
		_descriptors->RemoveObject(descriptor);
	}


	Array *RendererCoordinator::GetRenderers() const
	{
		std::lock_guard<std::mutex> lock(_lock);
		return _descriptors->Copy()->Autorelease();
	}

	Array *RendererCoordinator::GetAvailableRenderers() const
	{
		std::lock_guard<std::mutex> lock(_lock);

		Dictionary *settings = GetParameters();

		return _descriptors->GetObjectsPassingTest<RendererDescriptor>([&](RendererDescriptor *descriptor, bool &stop) -> bool {
			return descriptor->CanConstructWithSettings(settings);
		});
	}

	RendererDescriptor *RendererCoordinator::GetPreferredRenderer() const
	{
		std::lock_guard<std::mutex> lock(_lock);

		Dictionary *settings = GetSettings();
		String *preferredIdentifier = settings->GetObjectForKey<String>(RNCSTR("identifier"));

		if(preferredIdentifier)
			return __GetRendererWithIdentifier(preferredIdentifier);

		return nullptr;
	}

	RendererDescriptor *RendererCoordinator::GetRendererWithIdentifier(const String *identifier) const
	{
		std::lock_guard<std::mutex> lock(_lock);
		return __GetRendererWithIdentifier(identifier);
	}

	RendererDescriptor *RendererCoordinator::__GetRendererWithIdentifier(const String *identifier) const
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

	Renderer *RendererCoordinator::ActivateRenderer(RendererDescriptor *descriptor)
	{
		Dictionary *parameters = GetParameters();

		if(!descriptor->CanConstructWithSettings(parameters))
			throw InconsistencyException("Tried to activate renderer that is not available");

		Renderer *renderer = nullptr;

		try
		{
			renderer = descriptor->CreateRenderer(parameters);
			renderer->Activate();
		}
		catch(...)
		{
			delete renderer;
			std::rethrow_exception(std::current_exception());
		}

		return renderer;
	}
}
