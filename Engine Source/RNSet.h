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
	class SetInternal;
	
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
		
		template<class T>
		void Enumerate(const std::function<void (T *object, bool &stop)>& callback) const
		{
			Enumerate([&](Object *object, bool &stop) {
				callback(static_cast<T *>(object), stop);
			});
		}
		
		RNAPI Array *GetAllObjects() const;
		
		RNAPI size_t GetCount() const;
		
	private:
		PIMPL<SetInternal> _internals;
		
		RNDeclareMeta(Set)
	};
}

#endif /* __RAYNE_SET_H__ */
