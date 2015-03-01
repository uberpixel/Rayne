//
//  RNArray.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNArray.h"
#include "RNSet.h"
#include "RNSerialization.h"

namespace RN
{
	RNDefineMeta(Array, Object)
	
	Array::Array()
	{
		_size  = 5;
		_count = 0;
		
		_data = new Object *[_size];
	}
	
	Array::Array(size_t size)
	{
		_size  = size > 5 ? size : 5;
		_count = 0;
		
		_data = new Object *[_size];
	}
	
	Array::Array(const Array *other)
	{
		_size  = other->_size;
		_count = other->_count;
		
		_data = new Object *[_size];
		
		for(size_t i = 0; i < _count; i ++)
		{
			_data[i] = other->_data[i]->Retain();
		}
	}
	
	Array::Array(const Set *set)
	{
		_size  = set->GetCount() + 1;
		_count = set->GetCount();
		
		_data = new Object *[_size];
		
		size_t index = 0;
		
		set->Enumerate([&](Object *object, bool &stop) {
			_data[index ++] = object->Retain();
		});
	}
	
	Array::~Array()
	{
		for(size_t i = 0; i < _count; i ++)
		{
			_data[i]->Release();
		}
		
		delete [] _data;
	}
	
	
	Array::Array(Deserializer *deserializer)
	{
		_count = static_cast<size_t>(deserializer->DecodeInt64());
		_size  = static_cast<size_t>(deserializer->DecodeInt64());
		
		_data = new Object *[_size];
		
		for(size_t i = 0; i < _count; i ++)
		{
			_data[i] = deserializer->DecodeObject()->Retain();
		}
	}
	void Array::Serialize(Serializer *serializer)
	{
		serializer->EncodeInt64(static_cast<int64>(_count));
		serializer->EncodeInt64(static_cast<int64>(_size));
		
		for(size_t i = 0; i < _count; i ++)
		{
			serializer->EncodeObject(_data[i]);
		}
	}
	
	
	Array *Array::WithArray(const Array *other)
	{
		Array *array = new Array(other);
		return array->Autorelease();
	}
	
	Array *Array::WithSet(const Set *set)
	{
		Array *array = new Array(set);
		return array->Autorelease();
	}
	
	Array *Array::WithObjects(std::initializer_list<Object *> objects)
	{
		Array *array = new Array(objects.size());
		
		for(Object *object : objects)
			array->AddObject(object);
		
		return array->Autorelease();
	}
	
	
	void Array::ShrinkToFit()
	{
		if(_size + 1 < _count)
		{
			Object **tdata = new Object *[(_count + 1)];
			
			if(tdata)
			{
				std::move(_data, _data + _count, tdata);
				delete [] _data;
				
				_data = tdata;
				_size = _count + 1;
			}
		}
	}
	
	void Array::UpdateSizeIfNeeded(size_t required)
	{
		size_t toCopy = std::min(_count, required);
		
		if(required >= _size)
		{
			size_t tsize = std::max(required, static_cast<size_t>(_size * 1.5));
			Object **tdata = new Object *[tsize];
			
			if(tdata)
			{
				std::move(_data, _data + toCopy, tdata);
				delete [] _data;
				
				_size = tsize;
				_data = tdata;
			}
			
			return;
		}
		
		size_t tsize = _size >> 1;
		
		if(tsize >= required && tsize > 5)
		{
			Object **tdata = new Object *[tsize];
			
			if(tdata)
			{
				std::move(_data, _data + toCopy, tdata);
				delete [] _data;
				
				_size = tsize;
				_data = tdata;
			}
		}
	}
}
