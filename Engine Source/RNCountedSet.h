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
	class CountedSetInternal;
	
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
		
		RNAPI void Enumerate(const std::function<void (Object *object, size_t count, bool &stop)>& callback) const;
		
		template<class T>
		void Enumerate(const std::function<void (T *object, size_t count, bool &stop)>& callback) const
		{
			Enumerate([&](Object *object, size_t count, bool &stop) {
				callback(static_cast<T *>(object), count, stop);
			});
		}
		
		RNAPI Array *GetAllObjects() const;
		
		RNAPI size_t GetCount() const;
		RNAPI size_t GetCountForObject(Object *object) const;
		
	private:
		PIMPL<CountedSetInternal> _internals;
		
		RNDeclareMeta(CountedSet)
	};
}


#endif /* __RAYNE_COUNTEDSET_H__ */
