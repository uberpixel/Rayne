//
//  RNAngelscript.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ANGELSCRIPT_H__
#define __RAYNE_ANGELSCRIPT_H__

#include "RNBase.h"
#include "RNScriptEngine.h"

namespace RN
{
	class AngelScriptModule;
	class AngelScriptExecutionContext;
	
	class AngelScriptEngine : public ScriptEngine
	{
	public:
		friend class AngelScriptModule;
		friend class AngelScriptExecutionContext;
		
		AngelScriptEngine();
		~AngelScriptEngine() override;
		
		void RegisterClass(const std::string &name, size_t size, ObjectType type) final;
		void AddObjectBehavior(const std::string &object, ObjectBehavior behavior, const std::string &signature, const asSFuncPtr &ptr) final;
		void RegisterFunction(const std::string &signature, const asSFuncPtr &ptr) final;
		
	protected:
		ScriptModule *CreateModule(String *name) final;
		bool SwitchNamespace(const std::string &name) final;
		
	private:
		void RegisterStrings();
		void DebugCallback(void *msg, void *param);
		
		struct Internals;
		
		PIMPL<Internals> _internals;
	};
	
	class AngelScriptModule : public ScriptModule
	{
	public:
		friend class AngelScriptEngine;
		
		void AddCode(const String *code, const String *section) final;
		void Compile() final;
		
		ScriptExecutionContext *GetExecutionContextForFunction(String *signature) final;
		
	protected:
		AngelScriptModule(String *name, AngelScriptEngine *engine);
		
	private:
		struct Internals;
		
		AngelScriptEngine *_engine;
		PIMPL<Internals> _internals;
	};
	
	class AngelScriptExecutionContext : public ScriptExecutionContext
	{
	public:
		friend class AngelScriptModule;
		
		void SetInt8Argument(size_t arg, int8 value) final;
		void SetInt16Argument(size_t arg, int16 value) final;
		void SetInt32Argument(size_t arg, int32 value) final;
		void SetFloatArgument(size_t arg, float value) final;
		void SetDoubleArgument(size_t arg, double value) final;
		
		void Execute() final;
		void Suspend() final;
		
	private:
		AngelScriptExecutionContext(AngelScriptEngine *engine, void *module, void *function);
		
		struct Internals;
		
		PIMPL<Internals> _internals;
	};
}

#endif /* defined(__Rayne__RNAngelscript__) */
