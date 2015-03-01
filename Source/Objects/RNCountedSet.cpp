//
//  RNCountedSet.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNCountedSet.h"
#include "RNArray.h"
#include "RNHashTableInternal.h"
#include "RNSerialization.h"

namespace RN
{
	RNDefineMeta(CountedSet, Object)
	
	class CountedSetInternal
	{
	public:
		struct Bucket
		{
			Bucket()
			{
				object = nullptr;
				next   = nullptr;
				count  = 0;
			}
			
			Bucket(const Bucket *other)
			{
				object = SafeRetain(other->object);
				next   = nullptr;
				count  = other->count;
			}
			
			~Bucket()
			{
				for(size_t i = 0; i < count; i ++)
				{
					object->Release();
				}
			}
			
			
			machine_hash GetHash() const
			{
				return object->GetHash();
			}
			
			bool WrapsLookup(Object *lookup) const
			{
				return (object && object->IsEqual(lookup));
			}
			
			
			Object *object;
			Bucket *next;
			size_t count;
		};
		
		HashTableCore<Bucket> hashTable;
	};
	
	
	CountedSet::CountedSet()
	{
		_internals->hashTable.Initialize(0);
	}
	
	CountedSet::CountedSet(size_t capacity)
	{
		_internals->hashTable.Initialize(capacity);
	}
	
	CountedSet::CountedSet(const CountedSet *other)
	{
		_internals->hashTable.Initialize(other->_internals->hashTable);
	}
	
	CountedSet::CountedSet(const Array *other)
	{
		_internals->hashTable.Initialize(other->GetCount());
		
		other->Enumerate([&](Object *object, size_t index, bool &stop) {
			AddObject(object);
		});
	}
	
	CountedSet::~CountedSet()
	{}
	
	
	CountedSet::CountedSet(Deserializer *deserializer)
	{
		size_t count = static_cast<size_t>(deserializer->DecodeInt64());
		
		_internals->hashTable.Initialize(count);
		
		for(size_t i = 0; i < count; i ++)
		{
			size_t ocount  = static_cast<size_t>(deserializer->DecodeInt64());
			Object *object = deserializer->DecodeObject();
			
			for(size_t j = 0; j < ocount; j ++)
				AddObject(object);
		}
	}
	
	void CountedSet::Serialize(Serializer *serializer)
	{
		serializer->EncodeInt64(static_cast<int64>(GetCount()));
		
		Enumerate([&](Object *object, size_t count, bool &stop) {
			
			serializer->EncodeInt64(static_cast<int64>(count));
			serializer->EncodeObject(object);
			
		});
	}
	
	
	Array *CountedSet::GetAllObjects() const
	{
		Array *array = new Array(GetCount());
		
		for(size_t i = 0; i < _internals->hashTable._capacity; i++)
		{
			CountedSetInternal::Bucket *bucket = _internals->hashTable._buckets[i];
			while(bucket)
			{
				if(bucket->object)
					array->AddObject(bucket->object);
				
				bucket = bucket->next;
			}
		}
		
		return array->Autorelease();
	}
	
	
	
	void CountedSet::AddObject(Object *object)
	{
		bool created;
		CountedSetInternal::Bucket *bucket = _internals->hashTable.FindBucket(object, created);
		
		if(bucket)
		{
			bucket->count ++;
			bucket->object = object->Retain();
			
			if(created)
				_internals->hashTable.GrowIfPossible();
		}
	}
	
	void CountedSet::RemoveObject(Object *key)
	{
		CountedSetInternal::Bucket *bucket = _internals->hashTable.FindBucket(key);
		if(bucket)
		{
			bucket->object->Release();
			
			if((-- bucket->count) == 0)
			{
				bucket->object = nullptr;
				
				_internals->hashTable.ResignBucket(bucket);
				_internals->hashTable.CollapseIfPossible();
			}
		}
	}
	
	void CountedSet::RemoveAllObjects()
	{
		_internals->hashTable.RemoveAllBuckets();
	}
	
	bool CountedSet::ContainsObject(Object *object)
	{
		CountedSetInternal::Bucket *bucket = _internals->hashTable.FindBucket(object);
		return (bucket != nullptr);
	}
	
	size_t CountedSet::GetCountForObject(Object *object) const
	{
		CountedSetInternal::Bucket *bucket = _internals->hashTable.FindBucket(object);
		return bucket ? bucket->count : 0;
	}
	
	size_t CountedSet::GetCount() const
	{
		return _internals->hashTable.GetCount();
	}
	
	
	void CountedSet::Enumerate(const std::function<void (Object *, size_t count, bool &)>& callback) const
	{
		bool stop = false;
		
		for(size_t i = 0; i < _internals->hashTable._capacity; i ++)
		{
			CountedSetInternal::Bucket *bucket = _internals->hashTable._buckets[i];
			while(bucket)
			{
				if(bucket->object)
				{
					callback(bucket->object, bucket->count, stop);
					
					if(stop)
						return;
				}
				
				bucket = bucket->next;
			}
		}
	}
}

