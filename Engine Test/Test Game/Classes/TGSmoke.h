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
		Smoke(const Smoke *smoke);
		Smoke(RN::Deserializer *deserializer);
		
		void Serialize(RN::Serializer *serializer) override;
		
		float GetTransparency() const { return _transparency; }
		void SetTransparency(float transparency) { _transparency = transparency; }
		RN::Vector3 GetVelocityMin() const { return _velocityMin; }
		void SetVelocityMin(const RN::Vector3 &velocityMin) { _velocityMin = velocityMin; }
		RN::Vector3 GetVelocityMax() const { return _velocityMax; }
		void SetVelocityMax(const RN::Vector3 &velocityMax) { _velocityMax = velocityMax; }
		RN::Vector3 GetPositionMin() const { return _positionMin; }
		void SetPositionMin(const RN::Vector3 &positionMin) { _positionMin = positionMin; }
		RN::Vector3 GetPositionMax() const { return _positionMax; }
		void SetPositionMax(const RN::Vector3 &positionMax) { _positionMax = positionMax; }
		RN::Vector2 GetSizeStart() const { return _sizeStart; }
		void SetSizeStart(const RN::Vector2 &sizeStart) { _sizeStart = sizeStart; }
		RN::Vector2 GetSizeEnd() const { return _sizeEnd; }
		void SetSizeEnd(const RN::Vector2 &sizeEnd) { _sizeEnd = sizeEnd; }
		
	private:
		RN::Particle *CreateParticle();
		
		RN::Observable<float, Smoke> _transparency;
		RN::Observable<RN::Vector3, Smoke> _velocityMin;
		RN::Observable<RN::Vector3, Smoke> _velocityMax;
		RN::Observable<RN::Vector3, Smoke> _positionMin;
		RN::Observable<RN::Vector3, Smoke> _positionMax;
		RN::Observable<RN::Vector2, Smoke> _sizeStart;
		RN::Observable<RN::Vector2, Smoke> _sizeEnd;
		
		RNDeclareMeta(Smoke)
	};
}

#endif /* defined(__Test_Game__TGSmoke__) */
