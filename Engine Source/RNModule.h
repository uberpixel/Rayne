//
//  RNModule.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_MODULE_H__
#define __RAYNE_MODULE_H__

#include "RNBase.h"

#define kRNModuleCurrentVersion 1

namespace RN
{
	class Kernel;
	class Application;
	class Module;

	
	struct ModuleExports
	{
		int version;
		Module *module;
		Kernel *kernel;
		Application *application;
		
		uint32 type;
		
		// File modules
		bool (*_supportsFileType)(const std::string& file);
	};
	
	class Module
	{
	public:
		enum
		{
			ModuleTypeFile = (1 << 0),
			ModuleTypeTexture = ModuleTypeFile | (1 << 1),
			
			ModuleTypePhysics = (1 << 10)
		};
		
		RNAPI Module(const std::string& name);
		RNAPI ~Module();
		
		RNAPI void Load();
		RNAPI void Unload();
		bool IsLoaded() const { return (_handle != 0); }
		
		uint32 Type() const { return _exports.type; }
		
		const std::string& Name() const { return _name; }
		const std::string& Path() const { return _path; }
		
		RNAPI void *FunctionAddress(const std::string& name);
		
	private:
		void *_handle;
		
		std::string _name;
		std::string _path;
		
		bool (*_constructor)(ModuleExports *);
		void (*_destructor)();
		
		ModuleExports _exports;
	};
	
	class ModuleCoordinator : public Singleton<ModuleCoordinator>
	{
	public:
		ModuleCoordinator();
		
		Module *ModuleWithName(const std::string& name);
		
	private:
		std::vector<Module *> _modules;
	};
}

#endif /* __RAYNE_MODULE_H__ */
