//
//  RNObject.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OBJECT_H__
#define __RAYNE_OBJECT_H__

#include "RNBase.h"

namespace RN
{
	class Object
	{
	public:
		RNAPI Object();
		RNAPI virtual ~Object();
		
		template <typename T=Object>
		RNAPI T *Retain()
		{
			static_assert(std::is_base_of<Object, T>::value, "Release called with incompatible class");
			
			return static_cast<T *>(CoreRetain());
		}
		
		template <typename T=Object>
		RNAPI T *Release()
		{
			static_assert(std::is_base_of<Object, T>::value, "Release called with incompatible class");
			
			return static_cast<T *>(CoreRelease());
		}
		
		template <typename T=Object>
		RNAPI T *Autorelease()
		{
			static_assert(std::is_base_of<Object, T>::value, "Release called with incompatible class");
			
			return static_cast<T *>(CoreAutorelease());
		}
		
		RNAPI virtual bool IsEqual(Object *other) const;
		RNAPI virtual machine_hash Hash() const;
		
	private:
		Object *CoreRetain();
		Object *CoreRelease();
		Object *CoreAutorelease();
		
		machine_int _refCount;
	};
}

#endif /* __RAYNE_OBJECT_H__ */
