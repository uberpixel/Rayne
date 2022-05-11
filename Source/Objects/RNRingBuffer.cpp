//
//  RNRingBuffer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRingBuffer.h"
#include "RNSet.h"
#include "RNString.h"
#include "RNSerialization.h"

namespace RN
{
	RNDefineMeta(RingBuffer, Object)
	
	RingBuffer::RingBuffer(size_t size)
	{
		_size = size + 1; //Make it one bigger to allow the tail element to always be empty
		_head = _tail = 0;
		
		_data = new Object *[_size];
	}
	
	RingBuffer::RingBuffer(const RingBuffer *other)
	{
		_size  = other->_size;
		_head = other->_head;
		_tail = other->_tail;
		
		_data = new Object *[_size];
		for(size_t i = _head; i != _tail; i = GetNextIndex(i))
		{
			_data[i] = other->_data[i]->Retain();
		}
	}
	
	RingBuffer::~RingBuffer()
	{
		for(size_t i = _head; i != _tail; i = GetNextIndex(i))
		{
			_data[i]->Release();
		}
		
		delete [] _data;
	}
	
	
	RingBuffer::RingBuffer(Deserializer *deserializer)
	{
		_size  = static_cast<size_t>(deserializer->DecodeInt64());
		_head = static_cast<size_t>(deserializer->DecodeInt64());
		_tail = static_cast<size_t>(deserializer->DecodeInt64());
		
		_data = new Object *[_size];
		
		for(size_t i = _head; i != _tail; i = GetNextIndex(i))
		{
			_data[i] = deserializer->DecodeObject()->Retain();
		}
	}

	void RingBuffer::Serialize(Serializer *serializer) const
	{
		serializer->EncodeInt64(static_cast<int64>(_size));
		serializer->EncodeInt64(static_cast<int64>(_head));
		serializer->EncodeInt64(static_cast<int64>(_tail));
		
		for(size_t i = _head; i != _tail; i = GetNextIndex(i))
		{
			serializer->EncodeObject(_data[i]);
		}
	}
	
	
	RingBuffer *RingBuffer::WithRingBuffer(const RingBuffer *other)
	{
		RingBuffer *ringbuffer = new RingBuffer(other);
		return ringbuffer->Autorelease();
	}
	
	RingBuffer *RingBuffer::WithObjects(const std::initializer_list<Object *> objects)
	{
		RingBuffer *ringbuffer = new RingBuffer(objects.size());
		
		for(Object *object : objects)
			ringbuffer->PushObject(object);
		
		return ringbuffer->Autorelease();
	}

	const String *RingBuffer::GetDescription() const
	{
		if(_head == _tail)
			return RNCSTR("[]");

		String *result = String::WithString("[\n", false);

		Enumerate([&](Object *object, size_t index, bool &stop) {
			result->Append("\t");
			result->Append(object->GetDescription());
			result->Append(",\n");
		});

		result->Append("]");
		return result;
	}
}
