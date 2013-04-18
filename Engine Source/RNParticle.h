//
//  RNParticle.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PARTICLE_H__
#define __RAYNE_PARTICLE_H__

#include "RNBase.h"
#include "RNVector.h"
#include "RNColor.h"
#include "RNMatrix.h"
#include "RNTexture.h"

namespace RN
{	
	class Particle
	{
	public:
		Particle();
		Particle(const Vector3& position);
		virtual ~Particle();
		
		virtual void Initialize();
		virtual void Update(float delta);
		
		float time;
		float lifespan;
		
		Vector3 velocity;

		struct
		{
			Vector3 position;
			Vector2 size;
			Color color;
		};
	};
}

#endif /* __RAYNE_PARTICLE_H__ */
