//
//  RNModuleCoordinator.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNKernel.h"
#include "../Objects/RNAutoreleasePool.h"
#include "RNModuleCoordinator.h"

#if RN_PLATFORM_POSIX

#include <dlfcn.h>

#endif

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

			String *nameCopy = name->Copy()->Autorelease();

			try
			{
				_modules->AddObject(module);
				_moduleMap->SetObjectForKey(module, nameCopy);

				module->Initialize();
			}
			catch(Exception &e)
			{
				if(module)
				{
					_modules->RemoveObject(module);
					_moduleMap->RemoveObjectForKey(nameCopy);

					delete module;
				}

				throw e;
			}
		}
		catch(Exception &e)
		{
			throw e;
		}

		return module;
	}

	void ModuleCoordinator::LoadModules()
	{
		AutoreleasePool::PerformBlock([&]{

			Array *modules = Kernel::GetSharedInstance()->GetManifestEntryForKey<Array>(RNCSTR("RNModules"));
			if(modules)
			{
				modules->Enumerate<String>([&](String *name, size_t index, bool &stop) {

					Module *module = nullptr;
					String *nameCopy = name->Copy()->Autorelease();

					try
					{
						module = new Module(name);

						_modules->AddObject(module);
						_moduleMap->SetObjectForKey(module, nameCopy);

						module->Initialize();
					}
					catch(Exception &e)
					{
						if(module)
						{
							_modules->RemoveObject(module);
							_moduleMap->RemoveObjectForKey(nameCopy);

							delete module;
						}

						throw e;
					}

					RNDebug("Loaded module " << module);

				});
			}

		});
	}

	Module *ModuleCoordinator::GetModuleForClass(MetaClass *meta) const
	{
		return meta->GetModule();
	}

	Module *ModuleCoordinator::__GetModuleForSymbol(void *symbol)
	{
		Dl_info info;
		int status = dladdr(symbol, &info);
		if(status == 0)
			return nullptr;

		Module *outModule = nullptr;

		_modules->Enumerate<Module>([&](Module *module, size_t index, bool &stop) {

			if(dlsym(module->_handle, info.dli_sname))
			{
				outModule = module;
				stop = true;
			}

		});

		return outModule;
	}
}
