//
//  RNDictionary.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DICTIONARY_H__
#define __RAYNE_DICTIONARY_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Array;
	class Dictionary : public Object
	{
	public:
		RNAPI Dictionary();
		RNAPI Dictionary(size_t capacity);
		RNAPI ~Dictionary() override;
		
		RNAPI bool IsEqual(Object *other) const override;
		
		template<typename T=Object>
		T *ObjectForKey(Object *key)
		{
			Object *object = PrimitiveObjectForKey(key);
			if(object)
				return object->Downcast<T>();
			
			return nullptr;
		}
		
		RNAPI void SetObjectForKey(Object *object, Object *key);
		RNAPI void RemoveObjectForKey(Object *key);
		
		RNAPI void Enumerate(const std::function<void (Object *object, Object *key, bool *stop)>& callback);
		
		RNAPI Array *AllObjects() const;
		RNAPI Array *AllKeys() const;
		
		size_t Count() const { return _count; }
		
	private:
		struct Bucket
		{
			~Bucket()
			{
				if(key)
					key->Release();
				
				if(object)
					object->Release();
			}
			
			Object *key;
			Object *object;
			
			Bucket *next;
		};
		
		void Initialize(size_t primitive);
		
		Bucket *FindBucket(Object *key, bool createIfNeeded);
		
		Object *PrimitiveObjectForKey(Object *key);
		
		void GrowIfPossible();
		void CollapseIfPossible();
		
		void Rehash(size_t primitive);
		
		Bucket **_buckets;
		size_t _capacity;
		size_t _count;
		size_t _primitive;
		
		RNDefineMetaWithTraits(Dictionary, Object, MetaClassTraitCronstructable)
	};
}

#endif /* __RAYNE_DICTIONARY_H__ */
