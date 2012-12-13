//
//  RNArray.cpp
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNArray.h"

namespace RN
{
	Array::Array()
	{
		_count    = 0;
		_capacity = 10;
		_objects  = (Object **)malloc(_capacity * sizeof(Object *));
		
		RN::Assert(_objects);
	}
	
	Array::Array(size_t capacity)
	{
		RN::Assert(capacity > 0);
		
		_count    = 0;
		_capacity = capacity;
		_objects  = (Object **)malloc(_capacity * sizeof(Object *));
		
		RN::Assert(_objects);
	}
	
	Array::~Array()
	{
		for(size_t i=0; i<_count; i++)
		{
			Object *object = _objects[i];
			object->Release();
		}
		
		free(_objects);
	}
	
	
	void Array::AddObject(Object *object)
	{
		if(_count >= _capacity)
		{
			_capacity = _capacity * 2;
			_objects  = (Object **)realloc(_objects, _capacity * sizeof(Object *));
			
			RN::Assert(_objects);
		}
		
		_objects[_count ++] = object;
		object->Retain();
	}
	
	void Array::RemoveObject(Object *object)
	{
		machine_uint index = IndexOfObject(object);
		RemoveObjectAtIndex(index);
	}
	
	void Array::RemoveObjectAtIndex(machine_uint index)
	{
		RN::Assert(index != RN_NOT_FOUND);
		
		Object *object = _objects[index];
		object->Release();
		
		_count --;
		
		for(machine_uint i=index; i<_count; i++)
		{
			_objects[i] = _objects[i + 1];
		}
		
		if(_count < (_capacity / 2))
		{
			machine_uint capacity = _capacity / 2;
			Object **objects = (Object **)realloc(_objects, _capacity * sizeof(Object *));
			
			if(objects)
			{
				_objects  = objects;
				_capacity = capacity;
			}
		}
	}
	
	Object *Array::ObjectAtIndex(machine_uint index) const
	{
		RN::Assert(index != RN_NOT_FOUND && index < _count);
		return _objects[index];
	}
	
	machine_uint Array::IndexOfObject(Object *object) const
	{
		for(machine_uint i=0; i<_count; i++)
		{
			Object *tobject = _objects[i];
			
			if(object->Hash() == tobject->Hash() && object->IsEqual(tobject))
			{
				return true;
			}
		}
		
		return RN_NOT_FOUND;
	}
	
	machine_uint Array::Count() const
	{
		return _count;
	}
}
