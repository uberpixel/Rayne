//
//  RNObject.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include <iostream>
#include "RNObject.h"

namespace RN
{
	Object::Object() :
			_refCount(1)
	{}

	Object::~Object()
	{
		std::cout << "Dealloc" << std::endl;
	}

	void Object::WillDealloc()
	{}

	void Object::Retain()
	{
		_refCount ++;
	}

	void Object::Release()
	{
		if((-- _refCount) == 0)
		{
			WillDealloc();

			delete this;
			return;
		}
	}
}
