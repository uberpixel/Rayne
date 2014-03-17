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
		RN::Vector2 GetTargetHeight() const { return _targetHeight; }
		void SetTargetHeight(const RN::Vector2 &targetHeight) { _targetHeight = targetHeight; }
		
	private:
		RN::Particle *CreateParticle();
		void Initialize();
		
		RN::Observable<RN::Vector3, Fire> _velocityMin;
		RN::Observable<RN::Vector3, Fire> _velocityMax;
		RN::Observable<RN::Vector3, Fire> _positionMin;
		RN::Observable<RN::Vector3, Fire> _positionMax;
		RN::Observable<RN::Vector2, Fire> _sizeStart;
		RN::Observable<RN::Vector2, Fire> _sizeEnd;
		RN::Observable<RN::Vector2, Fire> _targetHeight;
		
		RNDeclareMeta(Fire)
	};
}

#endif /* defined(__Test_Game__TGFire__) */
