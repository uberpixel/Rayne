//
//  RNAutoreleasePool.h
//  Rayne
//
//  Created by Sidney Just on 31.01.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#ifndef __RAYNE_AUTORELEASEPOOL_H__
#define __RAYNE_AUTORELEASEPOOL_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNThread.h"

namespace RN
{
	class AutoreleasePool
	{
	public:
		AutoreleasePool();
		~AutoreleasePool();
		
		void AddObject(Object *object);
		void Drain();
		
		static AutoreleasePool *CurrentPool();
		
	private:
		Thread *_owner;
		AutoreleasePool *_parent;
		
		std::vector<Object *> _objects;
	};
}

#endif /* __RAYNE_AUTORELEASEPOOL_H__ */
