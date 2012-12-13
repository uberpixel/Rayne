//
//  RNWorld.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WORLD_H__
#define __RAYNE_WORLD_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNVector.h"

namespace RN
{
	class World : public Object
	{
	public:
		World();
		virtual ~World();
	};
}

#endif /* __RAYNE_WORLD_H__ */
