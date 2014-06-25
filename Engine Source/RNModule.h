//
//  RNModule.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MODULE_H__
#define __RAYNE_MODULE_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNArray.h"
#include "RNString.h"
#include "RNMutex.h"

#define kRNModuleManifestIgnoreABI RNCSTR("RNModuleIgnoreABI")
#define kRNModuleManifestOptional  RNCSTR("RNModuleOptional")

namespace RN
{
	class Kernel;
	class Application;
	class Module;
	struct ModuleInternals;
	
	
	struct ModuleExports
	{
		uint32 version;
		
		Module *module;
		Kernel *kernel;
		Application *application;
	};
	
	class Module : public Object
	{
	public:
		RNAPI Module(const std::string& name);
		RNAPI ~Module();
		
		RNAPI void Load();
		RNAPI void Unload();
		RNAPI bool IsLoaded() const;
		
		RNAPI const std::string& GetName() const { return _name; }
		RNAPI const std::string& GetPath() const { return _path; }
		
		RNAPI uint32 GetABIVersion() const { return _exports.version; }
		
		RNAPI void *GetFunctionAddress(const std::string& name);
		
		template<class T>
		T *GetManifestObject(String *key) const
		{
			Object *result = __GetManifestObject(key);
			return result ? result->Downcast<T>() : nullptr;
		}
		
	private:
		RNAPI Object *__GetManifestObject(String *key) const;
		
		PIMPL<ModuleInternals> _internals;
		
		std::string _name;
		std::string _path;
		
		bool (*_constructor)(ModuleExports *);
		void (*_destructor)();
		
		ModuleExports _exports;
		
		RNDeclareMeta(Module)
	};
	
	RNObjectClass(Module)
	
	class ModuleCoordinator : public ISingleton<ModuleCoordinator>
	{
	public:
		RNAPI ModuleCoordinator();
		RNAPI ~ModuleCoordinator() override;
		
		RNAPI Module *GetModuleWithName(const std::string& name);
		RNAPI const Array *GetModules() const { return &_modules; }
		
	private:
		Mutex _mutex;
		Array _modules;
		
		RNDeclareSingleton(ModuleCoordinator)
	};
}

#endif /* __RAYNE_MODULE_H__ */
