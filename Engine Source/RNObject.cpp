//
//  RNObject.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNObject.h"

namespace RN
{
	Object::Object()
	{
		_refCount = 1;
	}
	
	Object::~Object()
	{
		RN_ASSERT(_refCount <= 1, "refCount must be <= 1 upon destructor call. Use object->Release(); instead of delete object;");
	}
	
	
	void Object::Retain()
	{
		if(!this)
			return;
		
		_refCount ++;
	}
	
	void Object::Release()
	{
		if(!this)
			return;
		
		if((-- _refCount) == 0)
		{
			delete this;
			return;
		}
	}
	
	
	
	bool Object::IsEqual(Object *other) const
	{
		return (this == other);
	}
	
	machine_hash Object::Hash() const
	{
		return (machine_hash)this;
	}
}
