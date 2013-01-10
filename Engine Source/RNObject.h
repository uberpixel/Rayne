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
		Object();
		virtual ~Object();
		
		void Retain();
		void Release();
		
		virtual bool IsEqual(Object *other) const;
		virtual machine_hash Hash() const;
		
	private:
		machine_int _refCount;
	};
}

#endif /* __RAYNE_OBJECT_H__ */
