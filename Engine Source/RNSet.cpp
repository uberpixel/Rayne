//
//  RNSet.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSet.h"
#include "RNArray.h"

#define kRNSetPrimitiveCount 64

namespace RN
{
	RNDeclareMeta(Set)
	
	static const size_t SetCapacity[kRNSetPrimitiveCount] =
	{
		3, 7, 13, 23, 41, 71, 127, 191, 251, 383, 631, 1087, 1723,
		2803, 4523, 7351, 11959, 19447, 31231, 50683, 81919, 132607,
		214519, 346607, 561109, 907759, 1468927, 2376191, 3845119,
		6221311, 10066421, 16287743, 26354171, 42641881, 68996069,
		111638519, 180634607, 292272623, 472907251,
#if RN_PLATFORM_64BIT
		765180413UL, 1238087663UL, 2003267557UL, 3241355263UL, 5244622819UL,
#endif
	};
	
	static const size_t SetMaxCount[kRNSetPrimitiveCount] =
	{
		3, 6, 11, 19, 32, 52, 85, 118, 155, 237, 390, 672, 1065,
		1732, 2795, 4543, 7391, 12019, 19302, 31324, 50629, 81956,
		132580, 214215, 346784, 561026, 907847, 1468567, 2376414,
		3844982, 6221390, 10066379, 16287773, 26354132, 42641916,
		68996399, 111638327, 180634415, 292272755,
#if RN_PLATFORM_64BIT
		472907503UL, 765180257UL, 1238087439UL, 2003267722UL, 3241355160UL,
#endif
	};
	
	Set::Set()
	{
		Initialize(0);
	}
	
	Set::Set(size_t capacity)
	{
		for(size_t i = 0; i < kRNSetPrimitiveCount; i ++)
		{
			if(SetCapacity[i] > capacity || i == kRNSetPrimitiveCount - 1)
			{
				Initialize(i);
				break;
			}
		}
	}
	
	Set::Set(const Set *other)
	{
		_primitive = other->_primitive;
		_capacity  = other->_capacity;
		_count     = other->_count;
		
		_buckets = new Bucket *[_capacity];
		
		for(size_t i = 0; i < _capacity; i ++)
		{
			Bucket *temp = nullptr;
			Bucket *bucket = other->_buckets[i];
			
			while(bucket)
			{
				if(bucket->object)
				{
					Bucket *copy = new Bucket(bucket);
					if(temp)
						temp->next = copy;
					
					temp = copy;
				}
				
				bucket = bucket->next;
			}
			
			_buckets[i] = temp;
		}
	}
	
	Set::Set(const Array *other)
	{
		for(size_t i = 0; i < kRNSetPrimitiveCount; i ++)
		{
			if(SetCapacity[i] > other->GetCount() || i == kRNSetPrimitiveCount - 1)
			{
				Initialize(i);
				break;
			}
		}
		
		other->Enumerate([&](Object *object, size_t index, bool *stop) {
			AddObject(object);
		});
	}
	
	Set::~Set()
	{
		for(size_t i = 0; i < _capacity; i ++)
		{
			Bucket *bucket = _buckets[i];
			while(bucket)
			{
				Bucket *next = bucket->next;
				delete bucket;
				
				bucket = next;
			}
		}
		
		delete [] _buckets;
	}
	
	void Set::Initialize(size_t primitive)
	{
		_primitive = primitive;
		_capacity  = SetCapacity[_primitive];
		_count     = 0;
		
		_buckets = new Bucket *[_capacity];
		std::fill(_buckets, _buckets + _capacity, nullptr);
	}
	
	
	
	
	Set::Bucket *Set::FindBucket(Object *object, bool createIfNeeded)
	{
		machine_hash hash = object->GetHash();
		size_t index = hash % _capacity;
		
		Bucket *bucket = _buckets[index];
		Bucket *empty  = nullptr;
		
		while(bucket)
		{
			if(bucket->object && object->IsEqual(bucket->object))
				return bucket;
			
			if(!bucket->object)
				empty = bucket;
			
			bucket = bucket->next;
		}
		
		if(createIfNeeded)
		{
			if(empty)
				return empty;
			
			bucket = new Bucket();
			bucket->next = _buckets[index];
			
			_buckets[index] = bucket;
		}
		
		return bucket;
	}
	
	void Set::Rehash(size_t primitive)
	{
		size_t cCapacity = _capacity;
		Bucket **buckets = _buckets;
		
		_capacity = SetCapacity[primitive];
		_buckets = new Bucket *[_capacity];
		
		if(!_buckets)
		{
			_buckets  = buckets;
			_capacity = cCapacity;
			
			return;
		}
		
		_primitive = primitive;
		std::fill(_buckets, _buckets + _capacity, nullptr);
		
		for(size_t i = 0; i < cCapacity; i ++)
		{
			Bucket *bucket = buckets[i];
			while(bucket)
			{
				Bucket *next = bucket->next;
				
				if(bucket->object)
				{
					machine_hash hash = bucket->object->GetHash();
					size_t index = hash % _capacity;
					
					bucket->next = _buckets[index];
					_buckets[index] = bucket;
				}
				else
				{
					delete bucket;
				}
				
				bucket = next;
			}
		}
		
		delete [] buckets;
	}
	
	
	
	void Set::GrowIfPossible()
	{
		if(_count >= SetMaxCount[_primitive] && _primitive < kRNSetPrimitiveCount)
		{
			Rehash(_primitive + 1);
		}
	}
	
	void Set::CollapseIfPossible()
	{
		if(_primitive > 0 && _count <= SetMaxCount[_primitive - 1])
		{
			Rehash(_primitive - 1);
		}
	}
	
	
	
	Array *Set::GetAllObjects() const
	{
		Array *array = new Array(_count);
		
		for(size_t i = 0; i < _capacity; i++)
		{
			Bucket *bucket = _buckets[i];
			while(bucket)
			{
				if(bucket->object)
					array->AddObject(bucket->object);
				
				bucket = bucket->next;
			}
		}
		
		return array->Autorelease();
	}
	
	
	
	void Set::AddObject(Object *object)
	{
		Bucket *bucket = FindBucket(object, true);
		if(bucket && !bucket->object)
		{
			bucket->object = object->Retain();
			_count ++;
			
			GrowIfPossible();
		}
	}
	
	void Set::RemoveObjectForKey(Object *key)
	{
		Bucket *bucket = FindBucket(key, false);
		if(bucket)
		{
			bucket->object->Release();
			bucket->object = nullptr;
			
			_count --;
			CollapseIfPossible();
		}
	}
	
	bool Set::ContainsObject(Object *object)
	{
		Bucket *bucket = FindBucket(object, false);
		return (bucket != nullptr);
	}
	
	
	
	void Set::Enumerate(const std::function<void (Object *, bool *)>& callback)
	{
		bool stop = false;
		
		for(size_t i = 0; i < _capacity; i ++)
		{
			Bucket *bucket = _buckets[i];
			while(bucket)
			{
				if(bucket->object)
				{
					callback(bucket->object, &stop);
					
					if(stop)
						return;
				}
				
				bucket = bucket->next;
			}
		}
	}
}
