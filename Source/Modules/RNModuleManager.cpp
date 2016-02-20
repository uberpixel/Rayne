//
//  RNModuleManager.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Base/RNKernel.h"
#include "../Objects/RNAutoreleasePool.h"
#include "RNModuleManager.h"

#if RN_PLATFORM_POSIX

#include <dlfcn.h>

#endif

namespace RN
{
	static ModuleManager *__sharedInstance = nullptr;

	ModuleManager::ModuleManager() :
		_modules(new Array()),
		_moduleMap(new Dictionary())
	{
		__sharedInstance = this;
	}

	ModuleManager::~ModuleManager()
	{
		SafeRelease(_modules);
		SafeRelease(_moduleMap);

		__sharedInstance = nullptr;
	}

	ModuleManager *ModuleManager::GetSharedInstance()
	{
		return __sharedInstance;
	}


	Module *ModuleManager::GetModuleWithName(const String *name)
	{
		return GetModuleWithName(name, 0);
	}

	Module *ModuleManager::GetModuleWithName(const String *name, Options options)
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

	void ModuleManager::LoadModules()
	{
		AutoreleasePool::PerformBlock([&]{

			Array *modules = Kernel::GetSharedInstance()->GetManifestEntryForKey<Array>(RNCSTR("RNModules"));
			if(modules)
			{
				modules->Enumerate<Object>([&](Object *value, size_t index, bool &stop) {

					bool optional = true;
					String *nameCopy = nullptr;

					// Read the module entry
					if(value->IsKindOfClass(String::GetMetaClass()))
					{
						nameCopy = value->Downcast<String>()->Copy()->Autorelease();
					}
					else if(value->IsKindOfClass(Dictionary::GetMetaClass()))
					{
						Dictionary *entry = value->Downcast<Dictionary>();

						String *name = entry->GetObjectForKey<String>(RNCSTR("location"));
						if(!name)
							throw InconsistencyException(RNSTR("Invalid modules entry, 'location' must be set!"));

						nameCopy = name->Copy()->Autorelease();

						Number *optionalValue = entry->GetObjectForKey<Number>(RNCSTR("optional"));
						if(optionalValue && optionalValue->GetBoolValue())
							optional = true;
					}
					else
					{
						throw InconsistencyException(RNSTR("Invalid modules entry" << value));
					}

					// Create the module
					Module *module = nullptr;

					try
					{
						module = new Module(nameCopy);

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

						if(optional)
							return;

						throw e;
					}

					RNInfo("Loaded module " << module);

				});
			}

		});
	}

	Module *ModuleManager::GetModuleForClass(MetaClass *meta) const
	{
		return meta->GetModule();
	}

	Module *ModuleManager::__GetModuleForSymbol(void *symbol)
	{
#if RN_PLATFORM_POSIX
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
#endif
#if RN_PLATFORM_WINDOWS
		return nullptr;
#endif
	}
}
