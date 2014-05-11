//
//  RNSculptable.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSculptable.h"

namespace RN
{
	RNDefineMeta(Sculptable, SceneNode)

	Sculptable::Sculptable()
	{}
	Sculptable::Sculptable(Deserializer *deserializer) : 
		SceneNode(deserializer) 
	{}
	Sculptable::Sculptable(const Sculptable *other) : 
		SceneNode(other) 
	{}

	void Sculptable::Serialize(Serializer *serializer)
	{
		SceneNode::Serialize(serializer); 
	}
}
