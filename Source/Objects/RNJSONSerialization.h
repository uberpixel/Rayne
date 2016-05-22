//
//  RNJSONSerialization.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JSONSERIALIZATION_H__
#define __RAYNE_JSONSERIALIZATION_H__

#include "../Base/RNBase.h"
#include "RNObject.h"
#include "RNData.h"
#include "RNString.h"

namespace RN
{
	class JSONSerialization
	{
	public:
		RN_OPTIONS(Options, uint32,
					PrettyPrint = (1 <<1),
					AllowFragments = (1 << 1));


		RNAPI static String *JSONStringFromObject(const Object *root, Options options = 0);
		RNAPI static Data *JSONDataFromObject(const Object *root, Options options = 0);

		template<class T = Object>
		static T *ObjectFromString(const String *string, Options options = 0)
		{
			return __ObjectFromString(string, options)->Downcast<T>();
		}

		template<class T = Object>
		static T *ObjectFromData(const Data *data, Options options = 0)
		{
			return __ObjectFromData(data, options)->Downcast<T>();
		}

		RNAPI static bool IsValidJSONObject(const Object *object);

	private:
		static Object *DeserializeObject(void *);
		static Object *DeserializeFromUTF8String(const char *string, Options options);

		static void *SerializeObject(const Object *object);
		static void *SerializeObject(const Object *root, Options options);

		RNAPI static Object *__ObjectFromString(const String *string, Options options);
		RNAPI static Object *__ObjectFromData(const Data *data, Options options);
	};
}

#endif /* __RAYNE_JSONSERIALIZATION_H__ */