//
//  RNCountedSet.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_COUNTEDSET_H__
#define __RAYNE_COUNTEDSET_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Array;
	class CountedSet : public Object
	{
	public:
		RNAPI CountedSet();
		RNAPI CountedSet(size_t capacity);
		RNAPI CountedSet(const Array *other);
		RNAPI CountedSet(const CountedSet *other);
		RNAPI ~CountedSet() override;
		
		RNAPI void AddObject(Object *object);
		RNAPI void RemoveObject(Object *object);
		RNAPI void RemoveAllObjects();
		RNAPI bool ContainsObject(Object *object);
		
		RNAPI void Enumerate(const std::function<void (Object *object, size_t count, bool *stop)>& callback);
		
		RNAPI Array *GetAllObjects() const;
		
		size_t GetCount() const { return _count; }
		RNAPI size_t GetCountForObject(Object *object);
		
	private:
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
				SafeRelease(object);
			}
			
			
			Object *object;
			Bucket *next;
			size_t count;
		};
		
		void Initialize(size_t primitive);
		
		Bucket *FindBucket(Object *object, bool createIfNeeded);
		
		void GrowIfPossible();
		void CollapseIfPossible();
		
		void Rehash(size_t primitive);
		
		Bucket **_buckets;
		size_t _capacity;
		size_t _count;
		size_t _primitive;
		
		RNDefineMetaWithTraits(CountedSet, Object, MetaClassTraitCronstructable, MetaClassTraitCopyable)
	};
}


#endif /* __RAYNE_COUNTEDSET_H__ */
