//
//  RNAutoreleasePool.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
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
		RNAPI AutoreleasePool();
		RNAPI ~AutoreleasePool();
		
		RNAPI void AddObject(Object *object);
		RNAPI void Drain();
		
		RNAPI static AutoreleasePool *GetCurrentPool();
		
	private:
		Thread *_owner;
		AutoreleasePool *_parent;
		
		std::vector<Object *> _objects;
	};
}

#endif /* __RAYNE_AUTORELEASEPOOL_H__ */
