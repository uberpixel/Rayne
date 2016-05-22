//
//  RNHashTableInternal.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_HASHTABLEINTERNAL_H__
#define __RAYNE_HASHTABLEINTERNAL_H__

#include "../Base/RNBase.h"

namespace RN
{
	static const size_t kHashTableCapacity[] =
	{
		3, 7, 13, 23, 41, 71, 127, 191, 251, 383, 631, 1087, 1723,
		2803, 4523, 7351, 11959, 19447, 31231, 50683, 81919, 132607,
		214519, 346607, 561109, 907759, 1468927, 2376191, 3845119,
		6221311, 10066421, 16287743, 26354171, 42641881, 68996069,
		111638519, 180634607, 292272623, 472907251,
		765180413UL, 1238087663UL, 2003267557UL, 3241355263UL, 5244622819UL,
	};
	
	static const size_t kHashTableMaxCount[] =
	{
		3, 6, 11, 19, 32, 52, 85, 118, 155, 237, 390, 672, 1065,
		1732, 2795, 4543, 7391, 12019, 19302, 31324, 50629, 81956,
		132580, 214215, 346784, 561026, 907847, 1468567, 2376414,
		3844982, 6221390, 10066379, 16287773, 26354132, 42641916,
		68996399, 111638327, 180634415, 292272755,
		472907503UL, 765180257UL, 1238087439UL, 2003267722UL, 3241355160UL,
	};

	#define kRNHashTablePrimitiveCount (sizeof(kHashTableCapacity) / sizeof(size_t))

	
	template<class Bucket>
	class HashTableCore
	{
	public:
		HashTableCore()
		{}
		
		~HashTableCore()
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
		
		void Initialize(size_t capacity)
		{
			size_t primitive = 0;
			
			for(size_t i = 0; i < kRNHashTablePrimitiveCount; i ++)
			{
				if(kHashTableCapacity[i] > capacity || i == kRNHashTablePrimitiveCount - 1)
				{
					primitive = i;
					break;
				}
			}
			
			_primitive = primitive;
			_capacity  = kHashTableCapacity[_primitive];
			_count     = 0;
			
			_buckets = new Bucket *[_capacity];
			std::fill(_buckets, _buckets + _capacity, nullptr);
		}
		
		void Initialize(const HashTableCore &other)
		{
			_primitive = other._primitive;
			_capacity  = other._capacity;
			_count     = other._count;
			
			_buckets = new Bucket *[_capacity];
			
			for(size_t i = 0; i < _capacity; i ++)
			{
				Bucket *temp = nullptr;
				Bucket *bucket = other._buckets[i];
				
				while(bucket)
				{
					if(bucket->object)
					{
						Bucket *copy = new Bucket(bucket);

						copy->next = temp;
						temp = copy;
					}
					
					bucket = bucket->next;
				}
				
				_buckets[i] = temp;
			}
		}
		
		
		
		
		Bucket *FindBucket(const Object *object) const
		{
			size_t hash = object->GetHash();
			size_t index = hash % _capacity;
			
			Bucket *bucket = _buckets[index];
			while(bucket)
			{
				if(bucket->WrapsLookup(object))
					return bucket;
				
				bucket = bucket->next;
			}
			
			return bucket;
		}
		
		Bucket *FindBucket(const Object *object, bool &created)
		{
			size_t hash = object->GetHash();
			size_t index = hash % _capacity;
			
			Bucket *bucket = _buckets[index];
			Bucket *empty  = nullptr;
			
			created = false;
			
			while(bucket)
			{
				if(bucket->WrapsLookup(object))
					return bucket;
				
				if(!bucket->object)
					empty = bucket;
				
				bucket = bucket->next;
			}
			
			created = true;
			
			if(empty)
			{
				_count ++;
				return empty;
			}
			
			bucket = new Bucket();
			bucket->next = _buckets[index];
			
			_buckets[index] = bucket;
			_count ++;
			
			return bucket;
		}
		
		bool ContainsObject(const Object *object)
		{
			Bucket *bucket = FindBucket(object);
			return (bucket != nullptr);
		}
		
		
		void ResignBucket(Bucket *bucket)
		{
			bucket->object = nullptr;
			_count --;
		}
		
		void Rehash(size_t primitive)
		{
			size_t cCapacity = _capacity;
			Bucket **buckets = _buckets;
			
			_capacity = kHashTableCapacity[primitive];
			_buckets  = new Bucket *[_capacity];
			
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
						size_t hash = bucket->GetHash();
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
	
		void RemoveAllBuckets()
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
			_capacity  = kHashTableCapacity[_primitive];
			
			_buckets = new Bucket *[_capacity];
			std::fill(_buckets, _buckets + _capacity, nullptr);
		}
		
		void GrowIfPossible()
		{
			if(_count >= kHashTableMaxCount[_primitive] && _primitive < kRNHashTablePrimitiveCount)
			{
				Rehash(_primitive + 1);
			}
		}
		
		void CollapseIfPossible()
		{
			if(_primitive > 0 && _count <= kHashTableMaxCount[_primitive - 1])
			{
				Rehash(_primitive - 1);
			}
		}
		
		size_t GetCount() const
		{
			return _count;
		}
		
		
		Bucket **_buckets;
		size_t _count;
		size_t _capacity;
		
	private:
		size_t _primitive;
	};
}

#endif /* __RAYNE_HASHTABLEINTERNAL_H__ */
