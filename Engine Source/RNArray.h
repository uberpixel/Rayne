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
	template <class T>
	class Array : public Object
	{
	public:
		Array()
		{
			_count    = 0;
			_capacity = 10;
			_data     = (T *)malloc(_capacity * sizeof(T));
			
			RN::Assert(_data);
		}
		
		Array(size_t capacity)
		{
			RN::Assert(capacity > 0);
			
			_count    = 0;
			_capacity = capacity;
			_data     = (T *)malloc(_capacity * sizeof(T));
			
			RN::Assert(_data);
		}
		
		virtual ~Array()
		{
			free(_data);
		}
		
		virtual void AddObject(T object)
		{
			if(_count >= _capacity)
			{
				_capacity = _capacity * 2;
				_data     = (T *)realloc(_data, _capacity * sizeof(T));
				
				RN::Assert(_data);
			}
			
			_data[_count ++] = object;
		}
		
		virtual void RemoveObject(T object)
		{
			machine_uint index = IndexOfObject(object);
			RemoveObjectAtIndex(index);
		}
		
		virtual void RemoveObjectAtIndex(machine_uint index)
		{
			RN::Assert(index != RN_NOT_FOUND);
			
			_count --;
			
			for(machine_uint i=index; i<_count; i++)
			{
				_data[i] = _data[i + 1];
			}
			
			if(_count < (_capacity / 2))
			{
				machine_uint capacity = _capacity / 2;
				T *data = (T *)realloc(_data, _capacity * sizeof(T));
				
				if(data)
				{
					_data     = data;
					_capacity = capacity;
				}
			}
		}
		
		virtual T ObjectAtIndex(machine_uint index) const
		{
			RN::Assert(index != RN_NOT_FOUND && index < _count);
			return _data[index];
		}
		
		virtual machine_uint IndexOfObject(T object) const
		{
			for(machine_uint i=0; i<_count; i++)
			{
				T tobject = _data[i];
				
				if(tobject == object)
					return true;
			}
			
			return RN_NOT_FOUND;
		}
		
		virtual machine_uint Count() const
		{
			return _count;
		}
		
	protected:
		T *_data;
		machine_uint _capacity;
		machine_uint _count;
	};
	
	
	
	class ObjectArray : public Array<Object *>
	{
	public:
		ObjectArray()
		{
		}
		
		ObjectArray(size_t size) :
			Array(size)
		{
		}
		
		virtual ~ObjectArray()
		{
			for(size_t i=0; i<_count; i++)
			{
				Object *object = _data[i];
				object->Release();
			}
			
			free(_data);
		}
		
		virtual void AddObject(Object *object)
		{
			Array::AddObject(object);
			object->Retain();
		}
		
		virtual void RemoveObjectAtIndex(machine_uint index)
		{
			RN::Assert(index != RN_NOT_FOUND);
			Object *object = _data[index];
			
			Array::RemoveObjectAtIndex(index);
			object->Release();
		}
		
		virtual machine_uint IndexOfObject(Object *object) const
		{
			for(machine_uint i=0; i<_count; i++)
			{
				Object *tobject = _data[i];
				
				if(object->Hash() == tobject->Hash() && object->IsEqual(tobject))
					return true;
			}
			
			return RN_NOT_FOUND;
		}
	};
}

#endif /* __RAYNE_ARRAY_H__ */
