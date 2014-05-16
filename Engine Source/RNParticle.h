//
//  RNParticle.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PARTICLE_H__
#define __RAYNE_PARTICLE_H__

#include "RNBase.h"
#include "RNVector.h"
#include "RNColor.h"
#include "RNMatrix.h"
#include "RNTexture.h"
#include "RNInterpolation.h"

namespace RN
{
	class ParticleEmitter;
	
	class Particle
	{
	public:
		RNAPI Particle();
		RNAPI virtual ~Particle();
		RNAPI virtual void Update(float delta);
		
		float time;
		float lifespan;
		
		struct
		{
			Vector3 position;
			Vector2 size;
			Color color;
		};
	};
	
	
	class GenericParticle : public RN::Particle
	{
	public:
		void Update(float delta);
		
		Vector3 velocity;
		Vector3 gravity;
		Interpolator<Color> colorInterpolator;
		Interpolator<Vector2> sizeInterpolator;
	};
}

#endif /* __RAYNE_PARTICLE_H__ */
