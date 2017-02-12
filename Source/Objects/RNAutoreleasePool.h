//
//  RNAutoreleasePool.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_AUTORELEASEPOOL_H__
#define __RAYNE_AUTORELEASEPOOL_H__

#include "../Base/RNBase.h"
#include "RNObject.h"

namespace RN
{
	class AutoreleasePool
	{
	public:
		RNAPI AutoreleasePool();
		RNAPI ~AutoreleasePool();

		RNAPI static void PerformBlock(Function &&function);
		
		RNAPI void AddObject(const Object *object);
		RNAPI void Drain();
		
		RNAPI static AutoreleasePool *GetCurrentPool();
		
	private:
		AutoreleasePool *_parent;
		std::thread::id _owner;
		std::vector<const Object *> _objects;
	};
}

#endif /* __RAYNE_AUTORELEASEPOOL_H__ */
