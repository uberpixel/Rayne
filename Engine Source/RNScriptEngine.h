//
//  RNScriptEngine.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SCRIPTENGINE_H__
#define __RAYNE_SCRIPTENGINE_H__

#include <angelscript.h>
#include "RNBase.h"
#include "RNString.h"
#include "RNObject.h"
#include "RNDictionary.h"

namespace RN
{
	class ScriptExecutionContext : public Object
	{
	public:
		virtual void SetInt8Argument(size_t arg, int8 value) = 0;
		virtual void SetInt16Argument(size_t arg, int16 value) = 0;
		virtual void SetInt32Argument(size_t arg, int32 value) = 0;
		virtual void SetFloatArgument(size_t arg, float value) = 0;
		virtual void SetDoubleArgument(size_t arg, double value) = 0;
		
		virtual void Execute() = 0;
		virtual void Suspend() = 0;
	};
	
	class ScriptModule : public Object
	{
	public:
		~ScriptModule() override;
		
		virtual void AddFile(const String *file);
		virtual void AddCode(const String *code, const String *section) = 0;
		
		virtual void Compile() = 0;
		
		virtual ScriptExecutionContext *GetExecutionContextForFunction(String *signature) = 0;
		
		const String *GetName() { return _name; }
		
	protected:
		ScriptModule(const String *name);
		
	private:
		String *_name;	};
	
	class ScriptEngine : public Object
	{
	public:
		enum class ObjectBehavior
		{
			Factory,
			Retain,
			Release,
			Constructor,
			Destructor,
			Method
		};
		
		enum class ObjectType
		{
			Value,
			Object,
			Reference
		};
		
		
		~ScriptEngine() override;
		
		virtual void RegisterClass(const std::string &name, size_t size, ObjectType type) = 0;
		virtual void AddObjectBehavior(const std::string &object, ObjectBehavior behavior, const std::string &signature, const asSFuncPtr &ptr) = 0;
		virtual void RegisterFunction(const std::string &signature, const asSFuncPtr &ptr) = 0;
		
		void SetNamespace(const std::string &name);
		
		ScriptModule *GetModule(String *name);
		const String *GetIdentifier() const { return _identifier; }
		const std::string &GetNamespace() { return _namespace; }
		
		
	protected:
		ScriptEngine(const String *identifier);
		
		virtual ScriptModule *CreateModule(String *name) = 0;
		virtual bool SwitchNamespace(const std::string &name) = 0;
		
	private:
		String *_identifier;
		Dictionary *_modules;
		std::string _namespace;
	};
}

#endif /* __RAYNE_SCRIPTENGINE_H__ */
