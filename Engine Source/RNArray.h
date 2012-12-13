//
//  RNArray.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ARRAY_H__
#define __RAYNE_ARRAY_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Array : public Object
	{
	public:
		Array();
		Array(size_t capacity);
		virtual ~Array();
		
		void AddObject(Object *object);
		void RemoveObject(Object *object);
		void RemoveObjectAtIndex(machine_uint index);
		
		Object *ObjectAtIndex(machine_uint index) const;
		
		machine_uint IndexOfObject(Object *object) const;
		machine_uint Count() const;
		
	private:
		Object **_objects;
		machine_uint _capacity;
		machine_uint _count;
	};
}

#endif /* __RAYNE_ARRAY_H__ */
