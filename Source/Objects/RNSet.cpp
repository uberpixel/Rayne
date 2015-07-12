//
//  RNSet.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSet.h"
#include "RNArray.h"
#include "RNHashTableInternal.h"
#include "RNSerialization.h"
#include "RNString.h"

namespace RN
{
	RNDefineMeta(Set, Object)
	
	class SetInternal
	{
	public:
		struct Bucket
		{
			Bucket()
			{
				object = nullptr;
				next   = nullptr;
			}
			
			Bucket(const Bucket *other)
			{
				object = SafeRetain(other->object);
				next   = nullptr;
			}
			
			~Bucket()
			{
				SafeRelease(object);
			}
			
			
			bool WrapsLookup(const Object *lookup) const
			{
				return (object && lookup->IsEqual(object));
			}
			
			size_t GetHash() const
			{
				return object->GetHash();
			}
			
			Object *object;
			Bucket *next;
		};
		
		HashTableCore<Bucket> hashTable;
	};
	
	Set::Set()
	{
		_internals->hashTable.Initialize(0);
	}
	
	Set::Set(size_t capacity)
	{
		_internals->hashTable.Initialize(capacity);
	}
	
	Set::Set(const Set *other)
	{
		_internals->hashTable.Initialize(other->_internals->hashTable);
	}
	
	Set::Set(const Array *other)
	{
		_internals->hashTable.Initialize(other->GetCount());
		
		other->Enumerate([&](Object *object, size_t index, bool &stop) {
			AddObject(object);
		});
	}
	
	Set::~Set()
	{}
	
	
	
	Set::Set(Deserializer *deserializer)
	{
		size_t count = static_cast<size_t>(deserializer->DecodeInt64());
		
		_internals->hashTable.Initialize(count);
		
		for(size_t i = 0; i < count; i ++)
		{
			Object *object = deserializer->DecodeObject();
			AddObject(object);
		}
	}
	
	void Set::Serialize(Serializer *serializer) const
	{
		serializer->EncodeInt64(static_cast<int64>(GetCount()));
		
		Enumerate([&](Object *object, bool &stop) {
			
			serializer->EncodeObject(object);
			
		});
	}

	const String *Set::GetDescription() const
	{
		if(_internals->hashTable._count == 0)
			return RNCSTR("[]");

		String *result = String::WithString("[\n", false);

		Enumerate([&](Object *object, bool &stop) {
			result->Append("\t");
			result->Append(object->GetDescription());
			result->Append(",\n");
		});

		result->Append("]");
		return result;
	}
	
	
	Array *Set::GetAllObjects() const
	{
		Array *array = new Array(_internals->hashTable.GetCount());
		
		for(size_t i = 0; i < _internals->hashTable._capacity; i++)
		{
			SetInternal::Bucket *bucket = _internals->hashTable._buckets[i];
			while(bucket)
			{
				if(bucket->object)
					array->AddObject(bucket->object);
				
				bucket = bucket->next;
			}
		}
		
		return array->Autorelease();
	}
	
	size_t Set::GetCount() const
	{
		return _internals->hashTable.GetCount();
	}
	
	
	void Set::AddObject(Object *object)
	{
		bool create;
		SetInternal::Bucket *bucket = _internals->hashTable.FindBucket(object, create);
		
		if(bucket && create)
		{
			bucket->object = object->Retain();
			_internals->hashTable.GrowIfPossible();
		}
	}
	
	void Set::RemoveObject(Object *key)
	{
		SetInternal::Bucket *bucket = _internals->hashTable.FindBucket(key);
		if(bucket)
		{
			SafeRelease(bucket->object);
			
			_internals->hashTable.ResignBucket(bucket);
			_internals->hashTable.CollapseIfPossible();
		}
	}
	
	void Set::RemoveAllObjects()
	{
		_internals->hashTable.RemoveAllBuckets();
	}
	
	bool Set::ContainsObject(Object *object) const
	{
		SetInternal::Bucket *bucket = _internals->hashTable.FindBucket(object);
		return (bucket != nullptr);
	}
	
	
	
	void Set::Enumerate(const std::function<void (Object *, bool &)>& callback) const
	{
		bool stop = false;
		
		for(size_t i = 0; i < _internals->hashTable._capacity; i ++)
		{
			SetInternal::Bucket *bucket = _internals->hashTable._buckets[i];
			while(bucket)
			{
				if(bucket->object)
				{
					callback(bucket->object, stop);
					
					if(stop)
						return;
				}
				
				bucket = bucket->next;
			}
		}
	}
}
