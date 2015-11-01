//
//  RNModuleCoordinator.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNKernel.h"
#include "RNModuleCoordinator.h"

namespace RN
{
	static ModuleCoordinator *__sharedInstance = nullptr;

	ModuleCoordinator::ModuleCoordinator() :
		_modules(new Array()),
		_moduleMap(new Dictionary())
	{
		__sharedInstance = this;
	}
	ModuleCoordinator::~ModuleCoordinator()
	{
		SafeRelease(_modules);
		SafeRelease(_moduleMap);

		__sharedInstance = nullptr;
	}

	ModuleCoordinator *ModuleCoordinator::GetSharedInstance()
	{
		return __sharedInstance;
	}



	Module *ModuleCoordinator::GetModuleWithName(const String *name)
	{
		return GetModuleWithName(name, 0);
	}

	Module *ModuleCoordinator::GetModuleWithName(const String *name, Options options)
	{
		std::lock_guard<std::mutex> lock(_lock);

		Module *module = _moduleMap->GetObjectForKey<Module>(const_cast<String *>(name));
		if(!module && options & Options::NoLoad)
			return nullptr;

		try
		{
			module = new Module(name);

			try
			{
				module->Initialize();
			}
			catch(Exception &e)
			{
				delete module;
				throw e;
			}


			String *nameCopy = name->Copy()->Autorelease();

			_modules->AddObject(module);
			_moduleMap->SetObjectForKey(module, nameCopy);
		}
		catch(Exception &e)
		{
			throw e;
		}

		return module;
	}

	void ModuleCoordinator::LoadModules()
	{
		Array *modules = Kernel::GetSharedInstance()->GetManifestEntryForKey<Array>(RNCSTR("RNModules"));
		if(modules)
		{
			modules->Enumerate<String>([&](String *name, size_t index, bool &stop) {

				Module *module = new Module(name);
				module->Initialize();

				RNDebug("Loaded module " << module);

			});
		}
	}
}
