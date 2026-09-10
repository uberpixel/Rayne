//
//  RNJoltCustomPlanetTerrainShapeInternal.h
//  Rayne-Jolt
//
//  Copyright 2026 by Überpixel. All rights reserved.
//

#ifndef __RAYNE_JOLTCUSTOMPLANETTERRAINSHAPEINTERNAL_H_
#define __RAYNE_JOLTCUSTOMPLANETTERRAINSHAPEINTERNAL_H_

namespace JPH
{
	class PhysicsSystem;
	class Shape;
}

namespace RN
{
	struct JoltCustomPlanetTerrainSample
	{
		double positionX;
		double positionY;
		double positionZ;
		float normalX;
		float normalY;
		float normalZ;
		float radius;
	};

	class JoltCustomPlanetTerrainInternalProvider
	{
	public:
		virtual void RetainProvider() = 0;
		virtual void ReleaseProvider() = 0;
		virtual float GetMaximumPlanetTerrainRadius() const = 0;
		virtual void GetPlanetTerrainLocalOrigin(double &x, double &y, double &z) const = 0;
		virtual unsigned int GetPlanetTerrainCollisionRevision() const = 0;
		virtual unsigned int GetPlanetTerrainCollisionCacheEpoch() const = 0;
		// Optional validation after an epoch/revision change. Default providers
		// retain full invalidation; true must guarantee the sampled value is unchanged.
		virtual bool CanReusePlanetTerrainCollisionSample(float, float, float, unsigned int, unsigned int, unsigned int, unsigned int) const { return false; }
		virtual bool SamplePlanetTerrain(float directionX, float directionY, float directionZ, JoltCustomPlanetTerrainSample &sample) const = 0;

	protected:
		virtual ~JoltCustomPlanetTerrainInternalProvider() = default;
	};

	namespace JoltCustomPlanetTerrainInternal
	{
		JPH::Shape *CreateShape(JoltCustomPlanetTerrainInternalProvider *provider, bool solidRecoveryOnly);
		void RegisterShape();
		void InstallSimulationCollideBodyVsBody(JPH::PhysicsSystem *physicsSystem);
	} // namespace JoltCustomPlanetTerrainInternal
} // namespace RN

#endif /* defined(__RAYNE_JOLTCUSTOMPLANETTERRAINSHAPEINTERNAL_H_) */
