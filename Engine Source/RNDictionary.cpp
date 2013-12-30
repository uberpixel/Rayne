//
//  RNDictionary.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNDictionary.h"
#include "RNArray.h"
#include "RNHashTableInternal.h"

namespace RN
{
	RNDeclareMeta(Dictionary)

	Dictionary::Dictionary()
	{
		Initialize(0);
	}
	
	Dictionary::Dictionary(size_t capacity)
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
	
	Dictionary::Dictionary(const Dictionary *other)
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
				if(bucket->key && bucket->object)
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
	
	Dictionary::~Dictionary()
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
		
		delete[] _buckets;
	}
	
	void Dictionary::Initialize(size_t primitive)
	{
		_primitive = primitive;
		_capacity  = HashTableCapacity[_primitive];
		_count     = 0;
		
		_buckets = new Bucket *[_capacity];
		std::fill(_buckets, _buckets + _capacity, nullptr);
	}
	
	
	
	Dictionary::Bucket *Dictionary::FindBucket(Object *key, bool createIfNeeded)
	{
		machine_hash hash = key->GetHash();
		size_t index = hash % _capacity;
		
		Bucket *bucket = _buckets[index];
		Bucket *empty  = nullptr;
		
		while(bucket)
		{
			if(bucket->key && key->IsEqual(bucket->key))
			{
				return bucket;
			}
			
			if(!bucket->key)
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
	
	void Dictionary::Rehash(size_t primitive)
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
		
		for(size_t i=0; i<cCapacity; i++)
		{
			Bucket *bucket = buckets[i];
			while(bucket)
			{
				Bucket *next = bucket->next;
				
				if(bucket->key)
				{
					machine_hash hash = bucket->key->GetHash();
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
		
		delete[] buckets;
	}
	
	
	
	void Dictionary::GrowIfPossible()
	{
		if(_count >= HashTableMaxCount[_primitive] && _primitive < kRNHashTablePrimitiveCount)
		{
			Rehash(_primitive + 1);
		}
	}
	
	void Dictionary::CollapseIfPossible()
	{
		if(_primitive > 0 && _count <= HashTableMaxCount[_primitive - 1])
		{
			Rehash(_primitive - 1);
		}
	}
	
	
	bool Dictionary::IsEqual(Object *temp) const
	{
		if(!temp->IsKindOfClass(Dictionary::MetaClass()))
			return false;
		
		Dictionary *other = static_cast<Dictionary *>(temp);
		if(_count != other->_count)
			return false;
		
		for(size_t i=0; i<_capacity; i++)
		{
			Bucket *bucket = _buckets[i];
			while(bucket)
			{
				if(bucket->key)
				{
					Bucket *otherBucket = other->FindBucket(bucket->key, false);
					if(!otherBucket)
						return false;
					
					if(!bucket->object->IsEqual(otherBucket->object))
						return false;
				}
				
				bucket = bucket->next;
			}
		}
		
		return true;
	}
	
	
	Array *Dictionary::GetAllObjects() const
	{
		Array *array = new Array(_count);
		
		for(size_t i=0; i<_capacity; i++)
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
	
	Array *Dictionary::GetAllKeys() const
	{
		Array *array = new Array(_count);
		
		for(size_t i=0; i<_capacity; i++)
		{
			Bucket *bucket = _buckets[i];
			while(bucket)
			{
				if(bucket->key)
					array->AddObject(bucket->key);
				
				bucket = bucket->next;
			}
		}
		
		return array->Autorelease();
	}
	
	
	Object *Dictionary::PrimitiveObjectForKey(Object *key)
	{
		Bucket *bucket = FindBucket(key, false);
		return bucket ? bucket->object : nullptr;
	}
	
	void Dictionary::SetObjectForKey(Object *object, Object *key)
	{
		Bucket *bucket = FindBucket(key, true);
		if(bucket)
		{
			bool wasOccupied = (bucket->key != nullptr);
			
			if(bucket->object)
				bucket->object->Release();
			
			if(bucket->key)
				bucket->key->Release();
			
			bucket->key    = key->Retain();
			bucket->object = object->Retain();
			
			if(!wasOccupied)
			{
				_count ++;
				GrowIfPossible();
			}
		}
	}
	
	void Dictionary::RemoveObjectForKey(Object *key)
	{
		Bucket *bucket = FindBucket(key, false);
		if(bucket)
		{
			bucket->key->Release();
			bucket->object->Release();
			
			bucket->key = bucket->object = nullptr;
			
			_count --;
			CollapseIfPossible();
		}
	}
	
	void Dictionary::RemoveAllObjects()
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
	
	void Dictionary::Enumerate(const std::function<void (Object *, Object *, bool *)>& callback)
	{
		bool stop = false;
		
		for(size_t i=0; i<_capacity; i++)
		{
			Bucket *bucket = _buckets[i];
			while(bucket)
			{
				if(bucket->key)
				{
					callback(bucket->object, bucket->key, &stop);
					
					if(stop)
						return;
				}
				
				bucket = bucket->next;
			}
		}
	}
}
