//
//  RNDictionary.cpp
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNDictionary.h"
#include "RNArray.h"

#define kRNDictionaryPrimitiveCount 64

namespace RN
{
	RNDeclareMeta(Dictionary)
	
	static const size_t DictionaryCapacity[kRNDictionaryPrimitiveCount] =
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
	
	static const size_t DictionaryMaxCount[kRNDictionaryPrimitiveCount] =
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
	
	Dictionary::Dictionary()
	{
		Initialize(0);
	}
	
	Dictionary::Dictionary(size_t capacity)
	{
		for(size_t i=0; i<kRNDictionaryPrimitiveCount; i++)
		{
			if(DictionaryCapacity[i] > capacity || i == kRNDictionaryPrimitiveCount - 1)
			{
				Initialize(i);
				break;
			}
		}
	}
	
	Dictionary::~Dictionary()
	{
		for(size_t i=0; i<_capacity; i++)
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
		_capacity  = DictionaryCapacity[_primitive];
		_count     = 0;
		
		_buckets = new Bucket *[_capacity];
		memset(_buckets, 0, _capacity * sizeof(Bucket **));
	}
	
	
	
	Dictionary::Bucket *Dictionary::FindBucket(Object *key, bool createIfNeeded)
	{
		machine_hash hash = key->Hash();
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
			bucket->key    = nullptr;
			bucket->object = nullptr;
			bucket->next   = _buckets[index];
			
			_buckets[index] = bucket;
		}
		
		return bucket;
	}
	
	void Dictionary::Rehash(size_t primitive)
	{
		size_t cCapacity = _capacity;
		Bucket **buckets = _buckets;
		
		_capacity = DictionaryCapacity[primitive];
		_buckets = new Bucket *[_capacity];
		
		if(!_buckets)
		{
			_buckets  = buckets;
			_capacity = cCapacity;
			
			return;
		}
		
		_primitive = primitive;
		memset(_buckets, 0, _capacity * sizeof(Bucket **));
		
		for(size_t i=0; i<cCapacity; i++)
		{
			Bucket *bucket = buckets[i];
			while(bucket)
			{
				Bucket *next = bucket->next;
				
				if(bucket->key)
				{
					machine_hash hash = bucket->key->Hash();
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
		if(_count >= DictionaryMaxCount[_primitive] && _primitive < kRNDictionaryPrimitiveCount)
		{
			Rehash(_primitive + 1);
		}
	}
	
	void Dictionary::CollapseIfPossible()
	{
		if(_primitive > 0 && _count <= DictionaryMaxCount[_primitive - 1])
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
	
	
	Array *Dictionary::AllObjects() const
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
	
	Array *Dictionary::AllKeys() const
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
}
