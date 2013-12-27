//
//  RNModule.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNModule.h"
#include "RNKernel.h"
#include "RNApplication.h"
#include "RNSettings.h"
#include "RNFile.h"
#include "RNPathManager.h"
#include "RNFileManager.h"
#include "RNLogging.h"

namespace RN
{
	RNDeclareMeta(Module)
	RNDeclareSingleton(ModuleCoordinator)
	
	struct ModuleInternals
	{
#if RN_PLATFORM_POSIX
		void *handle;
#endif
		
#if RN_PLATFORM_WINDOWS
		HMODULE handle;
#endif
	};
	
	// ---------------------
	// MARK: -
	// MARK: Module
	// ---------------------
	
	Module::Module(const std::string& name) :
		_name(name)
	{
#if RN_PLATFORM_MAC_OS
		_path = FileManager::GetSharedInstance()->GetFilePathWithName(_name + ".dylib");
#endif
		
#if RN_PLATFORM_LINUX
		_path = FileManager::GetSharedInstance()->GetFilePathWithName(_name + ".so");
#endif
		
#if RN_PLATFORM_WINDOWS
		_path = FileManager::GetSharedInstance()->GetFilePathWithName(_name + ".dll");
#endif
		
		memset(&_exports, 0, sizeof(ModuleExports));
		
		_internals->handle = nullptr;
		
		_exports.module = this;
		_exports.kernel = Kernel::GetSharedInstance();
		_exports.application = Application::GetSharedInstance();
	}
	
	Module::~Module()
	{
		Unload();
	}
	
	
	void Module::Load()
	{
		if(!IsLoaded())
		{
#if RN_PLATFORM_POSIX
			_internals->handle = dlopen(_path.c_str(), RTLD_LAZY);
			if(!_internals->handle)
				throw Exception(Exception::Type::ModuleNotFoundException, std::string(dlerror()));
#endif
			
#if RN_PLATFORM_WINDOWS
			_internals->handle = LoadLibrary(_path.c_str());
			if(!_internals->handle)
			{
				DWORD lastError = ::GetLastError();
				TCHAR buffer[256] = "?";
				
				if(lastError)
					::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, lastError, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), buffer, 256, NULL);
				
				throw Exception(Exception::Type::ModuleNotFoundException, std::string(buffer));
			}
#endif
			
			_constructor = (bool (*)(ModuleExports *))(GetFunctionAddress("RNModuleConstructor"));
			_destructor  = (void (*)())(GetFunctionAddress("RNModuleDestructor"));
			
			if(!_constructor(&_exports))
			{
				Unload();
				throw Exception(Exception::Type::ModuleConstructFailedException, "Module " + _name + " failed to construct");
			}
			
			// Sanity check
			if(_exports.version != GetABIVersion())
			{
				Unload();
				throw Exception(Exception::Type::ModuleUnsupportedABIException, "Module " + _name + " uses an unuspported ABI version");
			}
		}
	}
	
	void Module::Unload()
	{
		if(IsLoaded())
		{
			if(_destructor)
				_destructor();
			
#if RN_PLATFORM_POSIX
			dlclose(_internals->handle);
			_internals->handle = nullptr;
#endif
			
#if RN_PLATFORM_WINDOWS
			::FreeLibrary(_internals->handle);
			_internals->handle = nullptr;
#endif
		}
	}
	
	void *Module::GetFunctionAddress(const std::string& name)
	{
#if RN_PLATFORM_POSIX
		return dlsym(_internals->handle, name.c_str());
#endif
		
#if RN_PLATFORM_WINDOWS
		return ::GetProcAddress(_internals->handle, name.c_str());
#endif
	}
	
	bool Module::IsLoaded() const
	{
		return (_internals->handle != nullptr);
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: ModuleCoordinator
	// ---------------------
	
	ModuleCoordinator::ModuleCoordinator()
	{
		Array *array = Settings::GetSharedInstance()->GetManifestObjectForKey<Array>(KRNManifestModulesKey);
		if(array)
		{
			array->Enumerate([&](Object *file, size_t index, bool *stop) {
				
				try
				{
					String *string = file->Downcast<String>();
					char   *path   = string->GetUTF8String();
					
					Module *module = new Module(path);
					_modules.AddObject(module->Autorelease());
				}
				catch(Exception e)
				{
					Log::Loggable loggable(Log::Level::Error);
					loggable << "Failed to load module. Reason: " << e.GetReason();
				}
				
			});
		}
		
		_modules.Enumerate<Module>([&](Module *module, size_t index, bool *stop) {
			
			try
			{
				module->Load();
			}
			catch(Exception e)
			{
				Log::Loggable loggable(Log::Level::Error);
				loggable << "Failed to load module. Reason: " << e.GetReason();
			}
			
		});
	}
	
	ModuleCoordinator::~ModuleCoordinator()
	{}
	
	Module *ModuleCoordinator::GetModuleWithName(const std::string& name)
	{
		Module *module = nullptr;
		
		_modules.Enumerate<Module>([&](Module *temp, size_t index, bool *stop) {
			if(temp->GetName().compare(name) == 0)
			{
				module = temp;
				*stop = true;
			}
		});
		
		if(module)
			return module;
		
		
		module = new Module(name);
		try
		{
			module->Load();
			_modules.AddObject(module->Autorelease());
			
			return module;
		}
		catch(Exception e)
		{
			delete module;
			throw e;
		}
	}
}
