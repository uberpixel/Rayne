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

namespace RN
{
	Module::Module(const std::string& name) :
		_name(name)
	{
#if RN_PLATFORM_MAC_OS
		_path = PathManager::PathForName(_name + ".dylib");
#endif
		
#if RN_PLATFORM_LINUX
		_path = PathManager::PathForName(_name + ".so");
#endif
		
		memset(&_exports, 0, sizeof(ModuleExports));
		
		_handle = 0;
		
		_exports.module = this;
		_exports.kernel = Kernel::SharedInstance();
		_exports.application = Application::SharedInstance();
	}
	
	Module::~Module()
	{
		Unload();
	}
	
	
	
	void Module::Load()
	{
		if(!IsLoaded())
		{
			_handle = dlopen(_path.c_str(), RTLD_LAZY);
			if(!_handle)
				throw ErrorException(0, 0, 0, std::string(dlerror()));
			
			_constructor = (bool (*)(ModuleExports *))(FunctionAddress("RNModuleConstructor"));
			_destructor  = (void (*)())(FunctionAddress("RNModuleDestructor"));
			
			if(!_constructor(&_exports))
				Unload();
		}
	}
	
	void Module::Unload()
	{
		if(IsLoaded())
		{
			if(_destructor)
				_destructor();
			
			dlclose(_handle);
			_handle = 0;
		}
	}
	
	
	void *Module::FunctionAddress(const std::string& name)
	{
		return dlsym(_handle, name.c_str());
	}
	
	
	
	ModuleCoordinator::ModuleCoordinator()
	{
		const std::vector<std::string>& moduleNames = Settings::SharedInstance()->Modules();
		
		for(auto i=moduleNames.begin(); i!=moduleNames.end(); i++)
		{
			Module *module = new Module(*i);
			_modules.push_back(module);
		}
		
		for(auto i=_modules.begin(); i!=_modules.end(); i++)
		{
			Module *module = *i;
			module->Load();
		}
	}
	
	Module *ModuleCoordinator::ModuleWithName(const std::string& name)
	{
		for(auto i=_modules.begin(); i!=_modules.end(); i++)
		{
			Module *module = *i;
			
			if(name.compare(module->Name()) == 0)
			   return module;
		}
		
		return 0;
	}
}
