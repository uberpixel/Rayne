//
//  RNSet.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSet.h"
#include "RNArray.h"
#include "RNHashTableInternal.h"

namespace RN
{
	RNDeclareMeta(Set)
	
	Set::Set()
	{
		Initialize(0);
	}
	
	Set::Set(size_t capacity)
	{
		for(size_t i = 0; i < kRNHashTablePrimitiveCount; i ++)
		{
			if(HashTableCapacity[i] > capacity || i == kRNHashTablePrimitiveCount - 1)
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
		for(size_t i = 0; i < kRNHashTablePrimitiveCount; i ++)
		{
			if(HashTableCapacity[i] > other->GetCount() || i == kRNHashTablePrimitiveCount - 1)
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
		_capacity  = HashTableCapacity[_primitive];
		_count     = 0;
		
		_buckets = new Bucket *[_capacity];
		std::fill(_buckets, _buckets + _capacity, nullptr);
	}
	
	
	
	Set::Bucket *Set::FindBucket1(Object *object) const
	{
		machine_hash hash = object->GetHash();
		size_t index = hash % _capacity;
		
		Bucket *bucket = _buckets[index];
		
		while(bucket)
		{
			if(bucket->object && object->IsEqual(bucket->object))
				return bucket;
			
			bucket = bucket->next;
		}
		
		return bucket;
	}
	
	Set::Bucket *Set::FindBucket2(Object *object)
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
		

		if(empty)
			return empty;
		
		bucket = new Bucket();
		bucket->next = _buckets[index];
		
		_buckets[index] = bucket;
		
		return bucket;
	}
	
	void Set::Rehash(size_t primitive)
	{
		size_t cCapacity = _capacity;
		Bucket **buckets = _buckets;
		
		_capacity = HashTableCapacity[primitive];
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
		if(_count >= HashTableMaxCount[_primitive] && _primitive < kRNHashTablePrimitiveCount)
		{
			Rehash(_primitive + 1);
		}
	}
	
	void Set::CollapseIfPossible()
	{
		if(_primitive > 0 && _count <= HashTableMaxCount[_primitive - 1])
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
		Bucket *bucket = FindBucket2(object);
		if(bucket && !bucket->object)
		{
			bucket->object = object->Retain();
			_count ++;
			
			GrowIfPossible();
		}
	}
	
	void Set::RemoveObject(Object *key)
	{
		Bucket *bucket = FindBucket1(key);
		if(bucket)
		{
			bucket->object->Release();
			bucket->object = nullptr;
			
			_count --;
			CollapseIfPossible();
		}
	}
	
	void Set::RemoveAllObjects()
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
		
		_count     = 0;
		_primitive = 1;
		_capacity  = HashTableCapacity[_primitive];
		
		_buckets = new Bucket *[_capacity];
		std::fill(_buckets, _buckets + _capacity, nullptr);
	}
	
	bool Set::ContainsObject(Object *object) const
	{
		Bucket *bucket = FindBucket1(object);
		return (bucket != nullptr);
	}
	
	
	
	void Set::Enumerate(const std::function<void (Object *, bool *)>& callback) const
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
