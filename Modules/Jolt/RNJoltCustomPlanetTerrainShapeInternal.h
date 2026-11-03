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

	struct JoltCustomPlanetTerrainBatchSample
	{
		float directionX, directionY, directionZ;
		JoltCustomPlanetTerrainSample sample;
		bool valid = false;
	};

	class JoltCustomPlanetTerrainInternalProvider
	{
	public:
		virtual void RetainProvider() = 0;
		virtual void ReleaseProvider() = 0;
		virtual float GetMaximumPlanetTerrainRadius() const = 0;
		// Conservative maximum dot(position, direction) for all regional samples.
		// Return zero when no bound is available; queries then use exact triangles.
		virtual double GetMaximumPlanetTerrainProjectionInRegion(unsigned char, double, double, double, double, double, double, double) const { return 0.0; }
		virtual void GetPlanetTerrainLocalOrigin(double &x, double &y, double &z) const = 0;
		virtual unsigned int GetPlanetTerrainCollisionRevision() const = 0;
		virtual unsigned int GetPlanetTerrainCollisionCacheEpoch() const = 0;
		// Optional validation after an epoch/revision change. Default providers
		// retain full invalidation; true must guarantee the sampled value is unchanged.
		virtual bool CanReusePlanetTerrainCollisionSample(float, float, float, unsigned int, unsigned int, unsigned int, unsigned int) const { return false; }
		virtual bool SamplePlanetTerrain(float directionX, float directionY, float directionZ, JoltCustomPlanetTerrainSample &sample) const = 0;

		// Collider submits at most 32 vertices; return the sampled state version.
		// Providers with concurrent publication must pin that state for the batch.
		virtual void SamplePlanetTerrainBatch(JoltCustomPlanetTerrainBatchSample *samples, unsigned int count, unsigned int &revision, unsigned int &epoch) const
		{
			revision = GetPlanetTerrainCollisionRevision();
			epoch = GetPlanetTerrainCollisionCacheEpoch();
			for(unsigned int i = 0; i < count; i += 1)
				samples[i].valid = SamplePlanetTerrain(samples[i].directionX, samples[i].directionY, samples[i].directionZ, samples[i].sample);
		}

	protected:
		virtual ~JoltCustomPlanetTerrainInternalProvider() = default;
	};

	namespace JoltCustomPlanetTerrainInternal
	{
#ifdef RN_PLANET_TERRAIN_SAMPLE_STATS
		struct SamplingStats
		{
			unsigned long coldVertices = 0, tileReplacements = 0, epochRefreshes = 0, revalidations = 0;
		};
		inline thread_local SamplingStats samplingStats;
#endif
		JPH::Shape *CreateShape(JoltCustomPlanetTerrainInternalProvider *provider, bool solidRecoveryOnly);
		void RegisterShape();
		void InstallSimulationCollideBodyVsBody(JPH::PhysicsSystem *physicsSystem);
	} // namespace JoltCustomPlanetTerrainInternal
} // namespace RN

#endif /* defined(__RAYNE_JOLTCUSTOMPLANETTERRAINSHAPEINTERNAL_H_) */
