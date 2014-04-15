//
//  RNSculptable.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __Rayne__RNSculptable__
#define __Rayne__RNSculptable__

#include "RNSceneNode.h"

namespace RN
{
	class Sculptable : public SceneNode
	{
	public:
		Sculptable() {}
		Sculptable(Deserializer *deserializer) : SceneNode(deserializer) {}
		Sculptable(const Sculptable *other) : SceneNode(other) {}
		~Sculptable() override {}
		
		void Serialize(Serializer *serializer) { SceneNode::Serialize(serializer); }
		
		RNAPI virtual void SetSphere(Vector3 position, float radius) = 0;
		RNAPI virtual void RemoveSphere(Vector3 position, float radius) = 0;
		
		RNAPI virtual void SetCube(Vector3 position, Vector3 size) = 0;
		RNAPI virtual void RemoveCube(Vector3 position, Vector3 size) = 0;
		
	private:
		RNDeclareMeta(Sculptable)
	};
}

#endif /* defined(__Rayne__RNSculptable__) */
