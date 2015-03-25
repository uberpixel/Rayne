//
//  RNDictionary.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_DICTIONARY_H__
#define __RAYNE_DICTIONARY_H__

#include "../Base/RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Array;
	class DictionaryInternal;
	
	class Dictionary : public Object
	{
	public:
		RNAPI Dictionary();
		RNAPI Dictionary(size_t capacity);
		RNAPI Dictionary(const Dictionary *other);
		RNAPI Dictionary(Deserializer *deserializer);
		RNAPI ~Dictionary() override;
		
		RNAPI void Serialize(Serializer *serializer);
		
		RNAPI size_t GetHash() const;
		RNAPI bool IsEqual(Object *other) const override;
		
		template<typename T=Object>
		T *GetObjectForKey(Object *key)
		{
			Object *object = PrimitiveObjectForKey(key);
			if(object)
				return object->Downcast<T>();
			
			return nullptr;
		}
		
		RNAPI void AddEntriesFromDictionary(const Dictionary *other);
		RNAPI void SetObjectForKey(Object *object, Object *key);
		RNAPI void RemoveObjectForKey(Object *key);
		RNAPI void RemoveAllObjects();
		
		RNAPI void Enumerate(const std::function<void (Object *object, Object *key, bool &stop)>& callback) const;
		
		template<class T, class K>
		void Enumerate(const std::function<void (T *object, K *key, bool &stop)>& callback) const
		{
			Enumerate([&](Object *object, Object *key, bool &stop) {
				callback(static_cast<T *>(object), static_cast<K *>(key), stop);
			});
		}
		
		RNAPI Array *GetAllObjects() const;
		RNAPI Array *GetAllKeys() const;
		
		RNAPI size_t GetCount() const;
		
	protected:
		RNAPI void SetValueForUndefinedKey(Object *value, const std::string &key) override;
		RNAPI Object *GetValueForUndefinedKey(const std::string &key) override;
		
	private:
		PIMPL<DictionaryInternal> _internals;
		
		RNAPI Object *PrimitiveObjectForKey(Object *key);
		
		RNDeclareMeta(Dictionary)
	};
	
	RNObjectClass(Dictionary)
}

#endif /* __RAYNE_DICTIONARY_H__ */
