//
//  RNJSONSerialization.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <jansson.h>
#include "RNJSONSerialization.h"
#include "RNAutoreleasePool.h"
#include "RNArray.h"
#include "RNDictionary.h"
#include "RNNumber.h"
#include "RNString.h"

namespace RN
{
	static MetaClassBase *__JSONArrayClass = 0;
	static MetaClassBase *__JSONDictionaryClass = 0;
	static MetaClassBase *__JSONNumberClass = 0;
	static MetaClassBase *__JSONStringClass = 0;
	
	void JSONReadClasses()
	{
		static std::once_flag onceToken;
		std::call_once(onceToken, []{
			__JSONArrayClass = Array::MetaClass();
			__JSONDictionaryClass = Dictionary::MetaClass();
			__JSONNumberClass = Number::MetaClass();
			__JSONStringClass = String::MetaClass();
		});
	}
	
	
	bool JSONSerialization::IsValidJSONObject(Object *object)
	{
		JSONReadClasses();
		
		if(object->IsKindOfClass(__JSONArrayClass))
			return true;
		
		if(object->IsKindOfClass(__JSONDictionaryClass))
			return true;
		
		if(object->IsKindOfClass(__JSONNumberClass))
			return true;
		
		if(object->IsKindOfClass(__JSONStringClass))
			return true;
		
		return false;
	}
	
	void *JSONSerialization::SerializeObject(Object *object)
	{
		json_t *json = nullptr;
		
		if(object->IsKindOfClass(__JSONNumberClass))
		{
			Number *number = static_cast<Number *>(object);
			switch(number->NumberType())
			{
				case Number::Type::Float32:
				case Number::Type::Float64:
					json = json_real(number->GetDoubleValue());
					break;
					
				case Number::Type::Uint8:
				case Number::Type::Uint16:
				case Number::Type::Uint32:
					json = json_integer(number->GetUint32Value());
					break;
					
				case Number::Type::Int8:
				case Number::Type::Int16:
				case Number::Type::Int32:
					json = json_integer(number->GetInt32Value());
					break;
					
				case Number::Type::Uint64:
					json = json_integer(number->GetUint64Value());
					break;
					
				case Number::Type::Int64:
					json = json_integer(number->GetInt64Value());
					break;
					
				case Number::Type::Boolean:
					json = json_boolean(number->GetBoolValue());
					break;
			}
		}
		
		if(object->IsKindOfClass(__JSONStringClass))
		{
			String *string = static_cast<String *>(object);
			const char *utf8 = reinterpret_cast<const char *>(string->GetBytesWithEncoding(Encoding::UTF8, false, nullptr));
			
			json = json_string_nocheck(utf8);
		}
		
		if(object->IsKindOfClass(__JSONArrayClass))
		{
			Array *array = static_cast<Array *>(object);
			json = json_array();
			
			array->Enumerate([&](Object *object, size_t index, bool *stop) {
				json_t *data = static_cast<json_t *>(SerializeObject(object));
				json_array_append_new(json, data);
			});
		}
		
		if(object->IsKindOfClass(__JSONDictionaryClass))
		{
			Dictionary *dictionary = static_cast<Dictionary *>(object);
			json = json_object();
			
			dictionary->Enumerate([&](Object *object, Object *key, bool *stop) {
				if(key->IsKindOfClass(__JSONStringClass))
				{
					String *string = static_cast<String *>(key);
					const char *utf8 = reinterpret_cast<const char *>(string->GetBytesWithEncoding(Encoding::UTF8, false, nullptr));
					
					json_t *data = static_cast<json_t *>(SerializeObject(object));
					json_object_set_new_nocheck(json, utf8, data);
				}
			});
		}
		
		return json;
	}
	
	Object *JSONSerialization::DeserializeObject(void *temp)
	{
		Object *data = nullptr;
		json_t *json = static_cast<json_t *>(temp);
		
		switch(json_typeof(json))
		{
			case JSON_OBJECT:
			{
				Dictionary *dict = new Dictionary();
				
				void *iterator = json_object_iter(json);
				while(iterator)
				{
					const char *key = json_object_iter_key(iterator);
					json_t *value = json_object_iter_value(iterator);
					
					Object *object = DeserializeObject(value);
					dict->SetObjectForKey(object, RNUTF8STR(key));
					
					iterator = json_object_iter_next(json, iterator);
				}
				
				data = dict;
				break;
			}
				
			case JSON_ARRAY:
			{
				size_t size = json_array_size(json);
				Array *array = new Array(size);
				
				for(size_t i=0; i<size; i++)
				{
					json_t *value = json_array_get(json, i);
					Object *object = DeserializeObject(value);
					
					array->AddObject(object);
				}
				
				data = array;
				break;
			}
				
			case JSON_TRUE:
				data = new Number(true);
				break;
			
			case JSON_FALSE:
				data = new Number(false);
				break;
				
			case JSON_INTEGER:
				data = new Number(json_integer_value(json));
				break;
				
			case JSON_REAL:
				data = new Number(json_real_value(json));
				break;
				
			case JSON_STRING:
			{
				String *string = new String(json_string_value(json), Encoding::UTF8);
				data = string;
				
				break;
			}
				
			case JSON_NULL:
				break;
		}
		
		return data ? data->Autorelease() : data;
	}
	
	
	
	Data *JSONSerialization::JSONDataFromObject(Object *root, SerializerOptions options)
	{
		JSONReadClasses();
		
		if(!root->IsKindOfClass(__JSONArrayClass) && !root->IsKindOfClass(__JSONDictionaryClass))
			return nullptr;
		
		size_t flags = 0;
		
		if(options & PrettyPrint)
		{
			flags |= JSON_INDENT(4);
		}
		
		json_t *json = static_cast<json_t *>(SerializeObject(root));
		char *data = json_dumps(json, flags);
		
		Data *temp = new Data(data, strlen(data));
		free(data);
		
		json_decref(json);
		
		return temp->Autorelease();
	}
	
	Object *JSONSerialization::JSONObjectFromData(Data *data)
	{
		json_error_t error;
		size_t flags = JSON_DISABLE_EOF_CHECK;
		json_t *root = json_loads(reinterpret_cast<char *>(data->GetBytes()), flags, &error);
		
		if(!root)
		{
			char buffer[256];
			sprintf(buffer, "%s\nLine: %i, column: %i", error.text, error.line, error.column);
			
			throw Exception(Exception::Type::GenericException, buffer);
		}
		
		AutoreleasePool *pool = new AutoreleasePool();
		
		Object *object = DeserializeObject(root)->Retain();
		json_decref(root);
		
		delete pool;
		
		return object->Autorelease();
	}
}
