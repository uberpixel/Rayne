//
//  RNSet.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SET_H__
#define __RAYNE_SET_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Array;
	class Set : public Object
	{
	public:
		RNAPI Set();
		RNAPI Set(size_t capacity);
		RNAPI Set(const Array *other);
		RNAPI Set(const Set *other);
		RNAPI ~Set() override;
		
		RNAPI void AddObject(Object *object);
		RNAPI void RemoveObject(Object *object);
		RNAPI void RemoveAllObjects();
		RNAPI bool ContainsObject(Object *object) const;
		
		RNAPI void Enumerate(const std::function<void (Object *object, bool &stop)>& callback) const;
		
		RNAPI Array *GetAllObjects() const;
		
		RNAPI size_t GetCount() const { return _count; }
		
	private:
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

			
			Object *object;
			Bucket *next;
		};
		
		void Initialize(size_t primitive);
		
		Bucket *FindBucket1(Object *object) const;
		Bucket *FindBucket2(Object *object);
		
		void GrowIfPossible();
		void CollapseIfPossible();
		
		void Rehash(size_t primitive);
		
		Bucket **_buckets;
		size_t _capacity;
		size_t _count;
		size_t _primitive;
		
		RNDefineMetaWithTraits(Set, Object, MetaClassTraitCronstructable, MetaClassTraitCopyable)
	};
}

#endif /* __RAYNE_SET_H__ */
