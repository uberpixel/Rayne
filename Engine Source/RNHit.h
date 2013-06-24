//
//  RNHit.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_HIT_H__
#define __RAYNE_HIT_H__

#include "RNBase.h"
#include "RNVector.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"

namespace RN
{
	class Hit
	{
	public:
		Hit(class SceneNode *node=0, float dist = -1.0f, const Vector3 &pos=Vector3(), const Vector3 &norm=Vector3());
		
		float distance;
		Vector3 position;
		Vector3 normal;
		class SceneNode *node;
		uint32 meshid;
	};
	
	RN_INLINE Hit::Hit(class SceneNode *node, float dist, const Vector3 &pos, const Vector3 &norm)
	: node(node), distance(dist), position(pos), normal(norm), meshid(-1)
	{
		
	}
}

#endif /* __RAYNE_HIT_H__ */
