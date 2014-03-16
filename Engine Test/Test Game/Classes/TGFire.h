//
//  TGFire.h
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __Test_Game__TGFire__
#define __Test_Game__TGFire__

#include <Rayne.h>

namespace TG
{
	class Fire : public RN::ParticleEmitter
	{
	public:
		Fire();
		Fire(const Fire *fire);
		Fire(RN::Deserializer *deserializer);
		
		void Serialize(RN::Serializer *serializer) override;
		
		void Update(float delta) override;
		void UpdateEditMode(float delta) override;
		
	private:
		RN::Particle *CreateParticle();
		void Initialize();
		RNDeclareMeta(Fire)
	};
}

#endif /* defined(__Test_Game__TGFire__) */
