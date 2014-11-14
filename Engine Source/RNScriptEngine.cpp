//
//  RNScriptEngine.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNScriptEngine.h"
#include "RNFile.h"

namespace RN
{
	ScriptModule::ScriptModule(const String *name) :
		_name(name->Copy())
	{}
	
	ScriptModule::~ScriptModule()
	{
		_name->Release();
	}
	
	void ScriptModule::AddFile(const String *name)
	{
		File *file = new File(name->GetUTF8String());
		std::string content = file->GetString();
		std::string complete = file->GetFullPath();
		file->Release();
		
		AddCode(RNSTR(content.c_str()), RNSTR(complete.c_str()));
	}
	
	
	
	ScriptEngine::ScriptEngine(const String *identifier) :
		_identifier(identifier->Copy()),
		_modules(new Dictionary())
	{}
	
	ScriptEngine::~ScriptEngine()
	{
		_identifier->Release();
		_modules->Release();
	}
	
	void ScriptEngine::SetNamespace(const std::string &name)
	{
		if(SwitchNamespace(name))
			_namespace = name;
	}
	
	ScriptModule *ScriptEngine::GetModule(String *name)
	{
		ScriptModule *module = _modules->GetObjectForKey<ScriptModule>(name);
		if(!module)
		{
			module = CreateModule(name);
			if(!module)
				return nullptr;
			
			_modules->SetObjectForKey(module, name);
		}
		
		return module;
	}
}
