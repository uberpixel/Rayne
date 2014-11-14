//
//  RNAngelscript.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <angelscript.h>

#include "RNAngelscript.h"
#include "RNLogging.h"
#include "RNFile.h"
#include "RNString.h"

#include "RNScriptBridge.h"

namespace RN
{
	// ---------------------
	// MARK: -
	// MARK: AngelScriptEngine
	// ---------------------
	
	struct AngelScriptEngine::Internals
	{
		asIScriptEngine *_engine;
	};
	
	
	AngelScriptEngine::AngelScriptEngine() :
		ScriptEngine(RNCSTR("net.uberpixel.script.angelscript"))
	{
		_internals->_engine =  asCreateScriptEngine(ANGELSCRIPT_VERSION);
		_internals->_engine->SetMessageCallback(asMETHOD(AngelScriptEngine, DebugCallback), this, asCALL_THISCALL);
		
		RegisterStrings();
		
		ScriptBridge bridge(this);
		bridge.Bind();
	}
	
	AngelScriptEngine::~AngelScriptEngine()
	{}
	
	
	void AngelScriptEngine::DebugCallback(void *msg, void *param)
	{
		asSMessageInfo *message = reinterpret_cast<asSMessageInfo *>(msg);
		
		switch(message->type)
		{
			case asMSGTYPE_ERROR:
				RNError("(Angelscript) %s (%d, %d): %s", message->section, message->row, message->col, message->message);
				break;
			case asMSGTYPE_WARNING:
				RNWarning("(Angelscript) %s (%d, %d): %s", message->section, message->row, message->col, message->message);
				break;
			case asMSGTYPE_INFORMATION:
				RNInfo("(Angelscript) %s (%d, %d): %s", message->section, message->row, message->col, message->message);
				break;
		}
	}
	
	
	void AngelScriptEngine::RegisterClass(const std::string &name, size_t size, ObjectType type)
	{
		asDWORD flags = 0;
		
		switch(type)
		{
			case ScriptEngine::ObjectType::Object:
				flags = asOBJ_REF;
				break;
			case ScriptEngine::ObjectType::Reference:
				flags = asOBJ_REF | asOBJ_NOCOUNT;
				break;
			case ScriptEngine::ObjectType::Value:
				flags = asOBJ_VALUE;
				break;
		}
		
		_internals->_engine->RegisterObjectType(name.c_str(), static_cast<int>(size), flags);
	}
	void AngelScriptEngine::AddObjectBehavior(const std::string &object, ObjectBehavior behavior, const std::string &signature, const asSFuncPtr &ptr)
	{
		asEBehaviours asBehavior;
		asDWORD convention = asCALL_THISCALL;
		
		switch(behavior)
		{
			case ScriptEngine::ObjectBehavior::Constructor:
				asBehavior = asBEHAVE_CONSTRUCT;
				break;
			case ScriptEngine::ObjectBehavior::Destructor:
				asBehavior = asBEHAVE_DESTRUCT;
				break;
			case ScriptEngine::ObjectBehavior::Release:
				asBehavior = asBEHAVE_RELEASE;
				break;
			case ScriptEngine::ObjectBehavior::Retain:
				asBehavior = asBEHAVE_ADDREF;
				break;
				
			case ScriptEngine::ObjectBehavior::Factory:
			{
				size_t divide = signature.find("@ ") + 2;
				
				std::string complete = signature.substr(0, divide);
				complete += object + "_" + signature.substr(divide);
				
				_internals->_engine->RegisterGlobalFunction(complete.c_str(), ptr, asCALL_CDECL);
				return;
			}
				
			case ScriptEngine::ObjectBehavior::Method:
				_internals->_engine->RegisterObjectMethod(object.c_str(), signature.c_str(), ptr, asCALL_THISCALL);
				return;
		}
		
		_internals->_engine->RegisterObjectBehaviour(object.c_str(), asBehavior, signature.c_str(), ptr, convention);
	}
	void AngelScriptEngine::RegisterFunction(const std::string &signature, const asSFuncPtr &ptr)
	{
		_internals->_engine->RegisterGlobalFunction(signature.c_str(), ptr, asCALL_CDECL);
	}
	
	bool AngelScriptEngine::SwitchNamespace(const std::string &name)
	{
		_internals->_engine->SetDefaultNamespace(name.c_str());
		return true;
	}
	
	ScriptModule *AngelScriptEngine::CreateModule(String *name)
	{
		AngelScriptModule *module = new AngelScriptModule(name, this);
		return module;
	}
	
	
	// ---------------------
	// MARK: -
	// MARK: AngelScriptModule
	// ---------------------
	
	struct AngelScriptModule::Internals
	{
		asIScriptModule *_module;
		asIScriptEngine *_engine;
	};
	
	AngelScriptModule::AngelScriptModule(String *name, AngelScriptEngine *engine) :
		ScriptModule(name),
		_engine(engine)
	{
		_internals->_engine = engine->_internals->_engine;
		_internals->_module = _internals->_engine->GetModule(name->GetUTF8String(), asGM_CREATE_IF_NOT_EXISTS);
	}
	
	void AngelScriptModule::AddCode(const String *code, const String *section)
	{
		_internals->_module->AddScriptSection(section->GetUTF8String(), code->GetUTF8String());
	}
	
	void AngelScriptModule::Compile()
	{
		_internals->_module->Build();
	}
	
	
	ScriptExecutionContext *AngelScriptModule::GetExecutionContextForFunction(String *signature)
	{
		asIScriptFunction *function = _internals->_module->GetFunctionByDecl(signature->GetUTF8String());
		AngelScriptExecutionContext *context = new AngelScriptExecutionContext(_engine, this, function);
		
		return context;
	}

	// ---------------------
	// MARK: -
	// MARK: AngelScriptExecutionContext
	// ---------------------
	
	struct AngelScriptExecutionContext::Internals
	{
		asIScriptEngine *_engine;
		asIScriptModule *_module;
		asIScriptFunction *_function;
		asIScriptContext *_context;
	};
	
	AngelScriptExecutionContext::AngelScriptExecutionContext(AngelScriptEngine *engine, void *module, void *function)
	{
		_internals->_engine = engine->_internals->_engine;
		_internals->_module = reinterpret_cast<asIScriptModule *>(module);
		_internals->_function = reinterpret_cast<asIScriptFunction *>(function);
		_internals->_context = _internals->_engine->CreateContext();
		_internals->_context->Prepare(_internals->_function);
	}
	
	
	void AngelScriptExecutionContext::SetInt8Argument(size_t arg, int8 value)
	{
		_internals->_context->SetArgByte(static_cast<asUINT>(arg), value);
	}
	void AngelScriptExecutionContext::SetInt16Argument(size_t arg, int16 value)
	{
		_internals->_context->SetArgWord(static_cast<asUINT>(arg), value);
	}
	void AngelScriptExecutionContext::SetInt32Argument(size_t arg, int32 value)
	{
		_internals->_context->SetArgDWord(static_cast<asUINT>(arg), value);
	}
	void AngelScriptExecutionContext::SetFloatArgument(size_t arg, float value)
	{
		_internals->_context->SetArgFloat(static_cast<asUINT>(arg), value);
	}
	void AngelScriptExecutionContext::SetDoubleArgument(size_t arg, double value)
	{
		_internals->_context->SetArgDouble(static_cast<asUINT>(arg), value);
	}
	
	
	void AngelScriptExecutionContext::Execute()
	{
		_internals->_context->Execute();
	}
	void AngelScriptExecutionContext::Suspend()
	{
		_internals->_context->Suspend();
	}
	
	// ---------------------
	// MARK: -
	// MARK: AngelScript String
	// ---------------------
	
	namespace STDString
	{
		template<class T>
		std::string toString(const T &value)
		{
			std::stringstream stream;
			stream << value;
			
			return stream.str();
		}
		
		static std::string StringFactory(asUINT length, const char *s)
		{
			return std::string(s, length);
		}
		
		static void ConstructString(std::string *ptr)
		{
			new(ptr) std::string();
		}
		
		static void ConstructStringCopy(const std::string &str, std::string *ptr)
		{
			new(ptr) std::string(str);
		}
		
		static void DestructString(std::string *ptr)
		{
			using namespace std;
			ptr->~string();
		}
		
		static char *StringCharAt(unsigned int i, std::string &str)
		{
			return &str[i];
		}
		
		static int StringCmp(const std::string &lhs, const std::string &rhs)
		{
			int cmp = 0;
			if (lhs < rhs)
				cmp = -1;
			else if (lhs > rhs)
				cmp = 1;
			return cmp;
		}
		
		static int StringFind(const std::string &rhs, const std::string &str)
		{
			return static_cast<int>(str.find(rhs));
		}
		
		static void ConstructStringInt(int value, std::string *ptr)
		{
			new(ptr) std::string();
			*ptr = toString(value);
		}
		
		static void ConstructStringUInt(unsigned value, std::string *ptr)
		{
			new(ptr) std::string();
			*ptr = toString(value);
		}
		
		static void ConstructStringFloat(float value, std::string *ptr)
		{
			new(ptr) std::string();
			*ptr = toString(value);
		}
		
		static void ConstructStringBool(bool value, std::string *ptr)
		{
			new(ptr) std::string();
			*ptr = toString(value);
		}
		
		static std::string &StringAssignInt(int value, std::string &str)
		{
			str = toString(value);
			return str;
		}
		
		static std::string &StringAddAssignInt(int value, std::string &str)
		{
			str += toString(value);
			return str;
		}
		
		static std::string StringAddInt(int value, const std::string &str)
		{
			return str + toString(value);
		}
		
		static std::string StringAddIntReverse(int value, const std::string &str)
		{
			return toString(value) + str;
		}
		
		static std::string &StringAssignUInt(unsigned value, std::string &str)
		{
			str = toString(value);
			return str;
		}
		
		static std::string &StringAddAssignUInt(unsigned value, std::string &str)
		{
			str += toString(value);
			return str;
		}
		
		static std::string StringAddUInt(unsigned value, const std::string &str)
		{
			return str + toString(value);
		}
		
		static std::string StringAddUIntReverse(unsigned value, const std::string &str)
		{
			return toString(value) + str;
		}
		
		static std::string &StringAssignFloat(float value, std::string &str)
		{
			str = toString(value);
			return str;
		}
		
		static std::string &StringAddAssignFloat(float value, std::string &str)
		{
			str += toString(value);
			return str;
		}
		
		static std::string StringAddFloat(float value, const std::string &str)
		{
			return str + toString(value);
		}
		
		static std::string StringAddFloatReverse(float value, const std::string &str)
		{
			return toString(value) + str;
		}
		
		static std::string &StringAssignBool(bool value, std::string &str)
		{
			str = toString(value);
			return str;
		}
		
		static std::string &StringAddAssignBool(bool value, std::string &str)
		{
			str += toString(value);
			return str;
		}
		
		static std::string StringAddBool(bool value, const std::string &str)
		{
			return str + toString(value);
		}
		
		static std::string StringAddBoolReverse(bool value, const std::string &str)
		{
			return toString(value) + str;
		}
	}
	
	void AngelScriptEngine::RegisterStrings()
	{
		_internals->_engine->RegisterObjectType("string", sizeof(std::string), asOBJ_VALUE | asOBJ_APP_CLASS_CDA);
		_internals->_engine->RegisterStringFactory("string", asFUNCTION(STDString::StringFactory), asCALL_CDECL);
		_internals->_engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(STDString::ConstructString), asCALL_CDECL_OBJLAST);
		_internals->_engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(const string& in)", asFUNCTION(STDString::ConstructStringCopy), asCALL_CDECL_OBJLAST);
		_internals->_engine->RegisterObjectBehaviour("string", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(STDString::DestructString), asCALL_CDECL_OBJLAST);
	}
}

		/*engine->RegisterObjectMethod("string", "string &opAssign(const string& in)", asMETHODPR(std::string, operator =, (const std::string&), std::string&), asCALL_THISCALL);
		 engine->RegisterObjectMethod("string", "string &opAddAssign(const string& in)", asMETHODPR(std::string, operator+=, (const std::string&), std::string&), asCALL_THISCALL);
		 engine->RegisterObjectMethod("string", "bool opEquals(const string& in) const", asFUNCTIONPR(std::operator ==, (const std::string&, const std::string&), bool), asCALL_CDECL_OBJFIRST);
		 engine->RegisterObjectMethod("string", "int opCmp(const string& in) const", asFUNCTION(StringCmp), asCALL_CDECL_OBJFIRST);
		 engine->RegisterObjectMethod("string", "string opAdd(const string& in) const", asFUNCTIONPR(std::operator +, (const std::string&, const std::string&), std::string), asCALL_CDECL_OBJFIRST);
		 engine->RegisterObjectMethod("string", "uint length() const", asMETHOD(std::string, size), asCALL_THISCALL);
		 engine->RegisterObjectMethod("string", "void resize(uint)", asMETHODPR(std::string, resize, (size_t), void), asCALL_THISCALL);
		 engine->RegisterObjectMethod("string", "uint8 &opIndex(uint)", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "const uint8 &opIndex(uint) const", asFUNCTION(StringCharAt), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "int find(const string& in) const", asFUNCTION(StringFind), asCALL_CDECL_OBJLAST);*/
		
		// Register automatic conversion functions for convenience
		/*engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(ConstructStringInt), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(uint)", asFUNCTION(ConstructStringUInt), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(ConstructStringFloat), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectBehaviour("string", asBEHAVE_CONSTRUCT, "void f(bool)", asFUNCTION(ConstructStringBool), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string &opAssign(int)", asFUNCTION(StringAssignInt), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string &opAddAssign(int)", asFUNCTION(StringAddAssignInt), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string opAdd(int) const", asFUNCTION(StringAddInt), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string opAdd_r(int) const", asFUNCTION(StringAddIntReverse), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string &opAssign(uint)", asFUNCTION(StringAssignUInt), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string &opAddAssign(uint)", asFUNCTION(StringAddAssignUInt), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string opAdd(uint) const", asFUNCTION(StringAddUInt), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string opAdd_r(uint) const", asFUNCTION(StringAddUIntReverse), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string &opAssign(float)", asFUNCTION(StringAssignFloat), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string &opAddAssign(float)", asFUNCTION(StringAddAssignFloat), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string opAdd(float) const", asFUNCTION(StringAddFloat), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string opAdd_r(float) const", asFUNCTION(StringAddFloatReverse), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string &opAssign(bool)", asFUNCTION(StringAssignBool), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string &opAddAssign(bool)", asFUNCTION(StringAddAssignBool), asCALL_CDECL_OBJLAST);
		 engine->RegisterObjectMethod("string", "string opAdd(bool) const", asFUNCTION(StringAddBool), asCALL_CDECL_OBJLAST);
		 engine->Regi
} */
