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
#include "RNString.h"
#include "RNSerialization.h"

namespace RN
{
	RNDefineMeta(Dictionary, Object)
	
	class DictionaryInternal
	{
	public:
		struct Bucket
		{
			Bucket()
			{
				key    = nullptr;
				object = nullptr;
				next   = nullptr;
			}
			
			Bucket(const Bucket *other)
			{
				key    = SafeRetain(other->key);
				object = SafeRetain(other->object);
				
				next = nullptr;
			}
			
			~Bucket()
			{
				SafeRelease(key);
				SafeRelease(object);
			}
			
			bool WrapsLookup(Object *lookup)
			{
				return (key && key->IsEqual(lookup));
			}
			
			machine_hash GetHash() const
			{
				return key->GetHash();
			}
			
			Object *key;
			Object *object;
			
			Bucket *next;
		};
		
		HashTableCore<Bucket> hashTable;
	};

	
	Dictionary::Dictionary()
	{
		_internals->hashTable.Initialize(0);
	}
	
	Dictionary::Dictionary(size_t capacity)
	{
		_internals->hashTable.Initialize(capacity);
	}
	
	Dictionary::Dictionary(const Dictionary *other)
	{
		_internals->hashTable.Initialize(other->_internals->hashTable);
	}
	
	Dictionary::~Dictionary()
	{}
	
	
	Dictionary::Dictionary(Deserializer *deserializer)
	{
		size_t count = static_cast<size_t>(deserializer->DecodeInt64());
		
		_internals->hashTable.Initialize(count);
		
		for(size_t i = 0; i < count; i ++)
		{
			Object *object = deserializer->DecodeObject();
			Object *key    = deserializer->DecodeObject();
			
			SetObjectForKey(object, key);
		}
	}
	
	void Dictionary::Serialize(Serializer *serializer)
	{
		serializer->EncodeInt64(static_cast<int64>(GetCount()));
		
		Enumerate([&](Object *object, Object *key, bool &stop) {
			
			serializer->EncodeObject(object);
			serializer->EncodeObject(key);
			
		});
	}
	
	
	bool Dictionary::IsEqual(Object *temp) const
	{
		if(!temp->IsKindOfClass(Dictionary::MetaClass()))
			return false;
		
		Dictionary *other = static_cast<Dictionary *>(temp);
		if(GetCount() != other->GetCount())
			return false;
		
		for(size_t i = 0; i < _internals->hashTable._capacity; i ++)
		{
			DictionaryInternal::Bucket *bucket = _internals->hashTable._buckets[i];
			while(bucket)
			{
				if(bucket->key)
				{
					DictionaryInternal::Bucket *otherBucket = other->_internals->hashTable.FindBucket(bucket->key);
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
		Array *array = new Array(_internals->hashTable._count);
		
		for(size_t i=0; i<_internals->hashTable._capacity; i++)
		{
			DictionaryInternal::Bucket *bucket = _internals->hashTable._buckets[i];
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
		Array *array = new Array(_internals->hashTable._count);
		
		for(size_t i = 0; i < _internals->hashTable._capacity; i++)
		{
			DictionaryInternal::Bucket *bucket = _internals->hashTable._buckets[i];
			while(bucket)
			{
				if(bucket->key)
					array->AddObject(bucket->key);
				
				bucket = bucket->next;
			}
		}
		
		return array->Autorelease();
	}
	
	size_t Dictionary::GetCount() const
	{
		return _internals->hashTable.GetCount();
	}
	
	
	Object *Dictionary::PrimitiveObjectForKey(Object *key)
	{
		DictionaryInternal::Bucket *bucket = _internals->hashTable.FindBucket(key);
		return bucket ? bucket->object : nullptr;
	}
	
	void Dictionary::AddEntriesFromDictionary(const Dictionary *other)
	{
		for(size_t i = 0; i < other->_internals->hashTable._capacity; i ++)
		{
			DictionaryInternal::Bucket *bucket = other->_internals->hashTable._buckets[i];
			while(bucket)
			{
				if(bucket->object)
					SetObjectForKey(bucket->object, bucket->key);
				
				bucket = bucket->next;
			}
		}
	}
	
	void Dictionary::SetObjectForKey(Object *object, Object *key)
	{
		bool created;
		DictionaryInternal::Bucket *bucket = _internals->hashTable.FindBucket(key, created);
		
		if(bucket)
		{
			SafeRelease(bucket->object);
			SafeRelease(bucket->key);
			
			bucket->key    = key->Retain();
			bucket->object = object->Retain();
			
			if(created)
				_internals->hashTable.GrowIfPossible();
		}
	}
	
	void Dictionary::RemoveObjectForKey(Object *key)
	{
		DictionaryInternal::Bucket *bucket = _internals->hashTable.FindBucket(key);
		
		if(bucket)
		{
			SafeRelease(bucket->key);
			SafeRelease(bucket->object);
			
			_internals->hashTable.ResignBucket(bucket);
			_internals->hashTable.CollapseIfPossible();
		}
	}
	
	void Dictionary::RemoveAllObjects()
	{
		_internals->hashTable.RemoveAllBuckets();
	}
	
	void Dictionary::Enumerate(const std::function<void (Object *, Object *, bool &)>& callback) const
	{
		bool stop = false;
		
		for(size_t i = 0; i < _internals->hashTable._capacity; i ++)
		{
			DictionaryInternal::Bucket *bucket = _internals->hashTable._buckets[i];
			while(bucket)
			{
				if(bucket->key)
				{
					callback(bucket->object, bucket->key, stop);
					
					if(stop)
						return;
				}
				
				bucket = bucket->next;
			}
		}
	}
	
	
	// KVO
	void Dictionary::SetValueForUndefinedKey(Object *value, const std::string &key)
	{
		SetObjectForKey(value, RNSTR(key.c_str()));
	}
	
	Object *Dictionary::GetValueForUndefinedKey(const std::string &key)
	{
		Object *value = GetObjectForKey(RNSTR(key.c_str()));
		return value;
	}
}
