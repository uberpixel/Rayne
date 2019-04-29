//
//  RNParticle.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PARTICLE_H__
#define __RAYNE_PARTICLE_H__

#include "../Base/RNBase.h"
#include "../Math/RNVector.h"
#include "../Math/RNColor.h"
#include "../Math/RNMatrix.h"
#include "../Math/RNInterpolation.h"

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
		
		struct Storage
		{
			Vector3 position;
			Vector2 size;
			Color color;
		};

		Storage storage;
	};
	
	
	class GenericParticle : public Particle
	{
	public:
		RNAPI void Update(float delta) final;
		
		Vector3 velocity;
		Vector3 gravity;
		Interpolator<Color> colorInterpolator;
		Interpolator<Vector2> sizeInterpolator;
	};
}

#endif /* __RAYNE_PARTICLE_H__ */
