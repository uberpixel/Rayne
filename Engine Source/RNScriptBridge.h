//
//  RNScriptBridge.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __Rayne__RNScriptBridge__
#define __Rayne__RNScriptBridge__

#include "RNBase.h"
#include "RNNumber.h"
#include "RNScriptEngine.h"

namespace RN
{
	template<bool R>
	struct Conditional
	{
		template<class F>
		Conditional(F &&f)
		{}
	};
	
	template<>
	struct Conditional<true>
	{
		template<class F>
		Conditional(F &&f)
		{
			f();
		}
	};
	
	
	void __RNDebugShim(const std::string &string);
	void __RNInfoShim(const std::string &string);
	void __RNWarningShim(const std::string &string);
	void __RNErrorShim(const std::string &string);

#define __RNScriptBindMethod(c, m, ret, params) _engine->AddObjectBehavior(#c, ScriptEngine::ObjectBehavior::Method, ret " " #m "(" params ")", asMETHOD(c, m))
#define __RNScriptBindMethodVoid(c, m, ret) __RNScriptBindMethod(c, m, ret, "")
#define __RNScriptBindMethodVoidVoid(c, m, ret) __RNScriptBindMethod(c, m, "void", "")
#define __RNScriptBindFactory(c, m, params) _engine->AddObjectBehavior(#c, ScriptEngine::ObjectBehavior::Factory, #c "@ " #m "(" params ")", asFUNCTION(c::m))
	
	
	class ScriptBridge
	{
	public:
		enum class Behavior
		{
			Create,
			Retain,
			Release,
			Constructor,
			Destructor
		};
		
		ScriptBridge(ScriptEngine *engine) :
			_engine(engine),
			_module(engine->GetModule(RNCSTR("RN")))
		{}
		
		void Bind()
		{
			_engine->RegisterFunction("void RNDebug(const string &in)", asFUNCTION(__RNDebugShim));
			_engine->RegisterFunction("void RNInfo(const string &in)", asFUNCTION(__RNInfoShim));
			_engine->RegisterFunction("void RNWarning(const string &in)", asFUNCTION(__RNWarningShim));
			_engine->RegisterFunction("void RNError(const string &in)", asFUNCTION(__RNErrorShim));
			
			// Switch to the RN namespace
			std::string space = _engine->GetNamespace();
			_engine->SetNamespace("RN");
			
			// Register classes
			RegisterClass<Number>("Number");
			
			__RNScriptBindFactory(Number, WithBool, "bool");
			__RNScriptBindFactory(Number, WithFloat, "float");
			__RNScriptBindFactory(Number, WithDouble, "double");
			
			__RNScriptBindFactory(Number, WithInt8, "int8");
			__RNScriptBindFactory(Number, WithInt16, "int16");
			__RNScriptBindFactory(Number, WithInt32, "int");
			__RNScriptBindFactory(Number, WithInt64, "int64");
			
			__RNScriptBindFactory(Number, WithUint8, "uint8");
			__RNScriptBindFactory(Number, WithUint16, "uint16");
			__RNScriptBindFactory(Number, WithUint32, "uint");
			__RNScriptBindFactory(Number, WithUint64, "uint64");
			
			__RNScriptBindMethodVoid(Number, GetBoolValue, "bool");
			__RNScriptBindMethodVoid(Number, GetFloatValue, "float");
			__RNScriptBindMethodVoid(Number, GetDoubleValue, "double");
			
			__RNScriptBindMethodVoid(Number, GetInt8Value, "int8");
			__RNScriptBindMethodVoid(Number, GetInt16Value, "int16");
			__RNScriptBindMethodVoid(Number, GetInt32Value, "int");
			__RNScriptBindMethodVoid(Number, GetInt64Value, "int64");
			
			__RNScriptBindMethodVoid(Number, GetUint8Value, "uint8");
			__RNScriptBindMethodVoid(Number, GetUint16Value, "uint16");
			__RNScriptBindMethodVoid(Number, GetUint32Value, "uint");
			__RNScriptBindMethodVoid(Number, GetUint64Value, "uint64");
			
			_engine->SetNamespace(space);
		}
		
		template<class U>
		void RegisterClass(const std::string &name)
		{
			Conditional<std::is_base_of<Object, U>::value>([&] {
				
				_engine->RegisterClass(name, sizeof(U), ScriptEngine::ObjectType::Object);
				_engine->AddObjectBehavior(name, ScriptEngine::ObjectBehavior::Retain, "void Retain()", asMETHOD(U, Retain));
				_engine->AddObjectBehavior(name, ScriptEngine::ObjectBehavior::Release, "void Release()", asMETHOD(U, Release));
				
				_engine->AddObjectBehavior(name, ScriptEngine::ObjectBehavior::Method, "uint64 GetHash()", asMETHOD(U, GetHash));
				
			});
		}
		
	private:
		ScriptEngine *_engine;
		ScriptModule *_module;
	};
}

#endif /* defined(__Rayne__RNScriptBridge__) */
