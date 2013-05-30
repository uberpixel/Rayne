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

namespace RN
{
	class JSONSerialization
	{
	public:
		enum
		{
			PrettyPrint = (1 << 1)
		};
		typedef uint32 SerializerOptions;
		
		static Data *JSONDataFromObject(Object *root, SerializerOptions options);
		static Object *JSONObjectFromData(Data *data);
		
		static bool IsValidJSONObject(Object *object);
		
	private:
		static Object *DeserializeObject(void *);
		static void *SerializeObject(Object *object);
	};
}

#endif /* __RAYNE_JSONSERIALIZATION_H__ */
