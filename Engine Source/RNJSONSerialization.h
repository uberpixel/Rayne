//
//  RNJSONSerialization.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JSONSERIALIZATION_H__
#define __RAYNE_JSONSERIALIZATION_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNData.h"
#include "RNString.h"

namespace RN
{
	class JSONSerialization
	{
	public:
		enum
		{
			PrettyPrint = (1 << 0),
			AllowFragments = (1 << 1)
		};
		typedef uint32 SerializerOptions;
		
		
		RNAPI static String *JSONStringFromObject(Object *root, SerializerOptions options = 0);
		RNAPI static Data *JSONDataFromObject(Object *root, SerializerOptions options = 0);
		
		template<class T = Object>
		static T *JSONObjectFromString(String *string, SerializerOptions options = 0)
		{
			return __JSONObjectFromString(string, options)->Downcast<T>();
		}
		
		template<class T = Object>
		static T *JSONObjectFromData(Data *data, SerializerOptions options = 0)
		{
			return __JSONObjectFromData(data, options)->Downcast<T>();
		}
		
		RNAPI static bool IsValidJSONObject(Object *object);
		
	private:
		static Object *DeserializeObject(void *);
		static Object *DeserializeFromUTF8String(const char *string, SerializerOptions options);
		
		static void *SerializeObject(Object *object);
		static void *SerializeObject(Object *root, SerializerOptions options);
		
		static Object *__JSONObjectFromString(String *string, SerializerOptions options);
		static Object *__JSONObjectFromData(Data *data, SerializerOptions options);
	};
}

#endif /* __RAYNE_JSONSERIALIZATION_H__ */
