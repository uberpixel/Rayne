//
//  RNJSONSerialization.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <jansson.h>
#include "RNJSONSerialization.h"
#include "RNAutoreleasePool.h"
#include "RNArray.h"
#include "RNDictionary.h"
#include "RNNumber.h"
#include "RNString.h"
#include "RNNull.h"

namespace RN
{
	static MetaClass *__JSONArrayClass = nullptr;
	static MetaClass *__JSONDictionaryClass = nullptr;
	static MetaClass *__JSONNumberClass = nullptr;
	static MetaClass *__JSONStringClass = nullptr;
	static MetaClass *__JSONNullClass = nullptr;

	void JSONReadClasses()
	{
		static std::once_flag onceToken;
		std::call_once(onceToken, [] {
			__JSONArrayClass = Array::GetMetaClass();
			__JSONDictionaryClass = Dictionary::GetMetaClass();
			__JSONNumberClass = Number::GetMetaClass();
			__JSONStringClass = String::GetMetaClass();
			__JSONNullClass = Null::GetMetaClass();
		});
	}




	bool JSONSerialization::IsValidJSONObject(const Object *object)
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
		if(object->IsKindOfClass(__JSONNullClass))
			return true;

		return false;
	}

	void *JSONSerialization::SerializeObject(const Object *object)
	{
		json_t *json = nullptr;

		if(object->IsKindOfClass(__JSONNumberClass))
		{
			const Number *number = static_cast<const Number *>(object);
			switch(number->GetType())
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
			const String *string = static_cast<const String *>(object);
			const char *utf8 = string->GetUTF8String();

			json = json_string_nocheck(utf8);
		}

		if(object->IsKindOfClass(__JSONArrayClass))
		{
			const Array *array = static_cast<const Array *>(object);
			json = json_array();

			array->Enumerate([&](Object *object, size_t index, bool &stop) {
				json_t *data = static_cast<json_t *>(SerializeObject(object));
				json_array_append_new(json, data);
			});
		}

		if(object->IsKindOfClass(__JSONDictionaryClass))
		{
			const Dictionary *dictionary = static_cast<const Dictionary *>(object);
			json = json_object();

			dictionary->Enumerate([&](Object *object, const Object *key, bool &stop) {

				if(key->IsKindOfClass(__JSONStringClass))
				{
					const String *string = static_cast<const String *>(key);
					const char *utf8 = string->GetUTF8String();

					json_t *data = static_cast<json_t *>(SerializeObject(object));
					json_object_set_new_nocheck(json, utf8, data);

					return;
				}

				throw InconsistencyException("Can't JSON serialize dictionaries with non String keys!");
			});
		}

		if(object->IsKindOfClass(__JSONNullClass))
		{
			json = json_null();
		}

		if(!json)
			throw InconsistencyException(RNSTR("Can't JSON serialize object " << object << " of type " << object->GetClass()));

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

				for(size_t i = 0; i < size; i ++)
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
				data = new String(json_string_value(json), Encoding::UTF8);
				break;

			case JSON_NULL:
				data = Null::GetNull()->Retain();
				break;
		}

		return data ? data->Autorelease() : data;
	}



	void *JSONSerialization::SerializeObject(const Object *root, Options options)
	{
		JSONReadClasses();

		size_t flags = 0;

		if(options & Options::PrettyPrint)
			flags |= JSON_INDENT(4);

		if(options & Options::AllowFragments)
			flags |= JSON_DECODE_ANY;


		json_t *json = static_cast<json_t *>(SerializeObject(root));
		char *data = json_dumps(json, flags);
		if(!data)
			throw InconsistencyException("Failed to serialize object to JSON");

		json_decref(json);
		return data;
	}

	Data *JSONSerialization::JSONDataFromObject(const Object *root, Options options)
	{
		char *data = reinterpret_cast<char *>(SerializeObject(root, options));

		Data *temp = new Data(data, strlen(data));
		free(data);

		return temp->Autorelease();
	}

	String *JSONSerialization::JSONStringFromObject(const Object *root, Options options)
	{
		void *data = SerializeObject(root, options);

		String *temp = new String(data, Encoding::UTF8);
		free(data);

		return temp->Autorelease();
	}



	Object *JSONSerialization::DeserializeFromUTF8String(const char *string, Options options)
	{
		JSONReadClasses();

		json_error_t error;
		size_t flags = JSON_DISABLE_EOF_CHECK;

		if(options & Options::AllowFragments)
			flags |= JSON_ENCODE_ANY;

		json_t *root = json_loads(string, flags, &error);

		if(!root)
			throw InconsistencyException(RNSTR(error.text << "\nLine: " << error.line << ", column: " << error.column));


		Object *object;

		{
			AutoreleasePool pool;

			object = DeserializeObject(root)->Retain();
			json_decref(root);
		}

		return object->Autorelease();
	}

	Object *JSONSerialization::__ObjectFromString(const String *string, Options options)
	{
		return DeserializeFromUTF8String(string->GetUTF8String(), options);
	}

	Object *JSONSerialization::__ObjectFromData(const Data *data, Options options)
	{
		return DeserializeFromUTF8String(reinterpret_cast<char *>(data->GetBytes()), options);
	}
}