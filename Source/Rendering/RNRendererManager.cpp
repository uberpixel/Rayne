//
//  RNRendererManager.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNSettings.h"
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
		std::lock_guard<std::mutex> lock(_lock);
		_descriptors->AddObject(descriptor);
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

		Dictionary *settings = GetParameters();

		return _descriptors->GetObjectsPassingTest<RendererDescriptor>([&](RendererDescriptor *descriptor, bool &stop) -> bool {
			return descriptor->CanConstructWithSettings(settings);
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
