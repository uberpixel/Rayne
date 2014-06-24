//
//  RNObjectInternals.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_OBJECTINTERNALS_H__
#define __RAYNE_OBJECTINTERNALS_H__

#include "RNBase.h"

namespace RN
{
	class Object;

	RNAPI Object *__InitWeak(Object **weak, Object *value);
	RNAPI Object *__StoreWeak(Object **weak, Object *value);
	RNAPI Object *__LoadWeakObjectRetained(Object **weak);
	RNAPI Object *__RemoveWeakObject(Object **weak);
	
	// Not exported in any way, used by the deallocation routine of the Object class itself
	void __DestroyWeakReferences(Object *object);
	void __InitWeakTables();
}

#endif /* __RAYNE_OBJECTINTERNALS_H__ */
