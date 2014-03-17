//
//  TGSmoke.h
//  Test Game
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __Test_Game__TGSmoke__
#define __Test_Game__TGSmoke__

#include <Rayne.h>

namespace TG
{
	class Smoke : public RN::ParticleEmitter
	{
	public:
		Smoke();
		Smoke(const Smoke *fire);
		Smoke(RN::Deserializer *deserializer);
		
		void Serialize(RN::Serializer *serializer) override;
		
		float GetTransparency() const { return _transparency; }
		void SetTransparency(float transparency) { _transparency = transparency; }
		
	private:
		RN::Particle *CreateParticle();
		void Initialize();
		
		RN::Observable<float, Smoke> _transparency;
		
		RNDeclareMeta(Smoke)
	};
}

#endif /* defined(__Test_Game__TGSmoke__) */
