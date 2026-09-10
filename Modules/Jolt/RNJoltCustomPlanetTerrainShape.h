//
//  RNJoltCustomPlanetTerrainShape.h
//  Rayne-Jolt
//
//  Copyright 2026 by Überpixel. All rights reserved.
//

#ifndef __RAYNE_JOLTCUSTOMPLANETTERRAINSHAPE_H_
#define __RAYNE_JOLTCUSTOMPLANETTERRAINSHAPE_H_

#include "RNJoltShape.h"

namespace RN
{
	class JoltCustomPlanetTerrainProvider : public Object
	{
	public:
		struct SurfaceSample
		{
			JoltPosition position;
			Vector3 normal;
			float radius;
		};

		JTAPI virtual float GetMaximumPlanetTerrainRadius() const = 0;
		JTAPI virtual JoltPosition GetPlanetTerrainLocalOrigin() const;
		JTAPI virtual uint32 GetPlanetTerrainCollisionRevision() const;
		JTAPI virtual uint32 GetPlanetTerrainCollisionCacheEpoch() const;
		// Called only to revalidate cached grid samples after publication changes.
		JTAPI virtual bool CanReusePlanetTerrainCollisionSample(const Vector3 &direction, uint32 oldRevision, uint32 oldEpoch, uint32 revision, uint32 epoch) const;
		JTAPI virtual bool SamplePlanetTerrain(const Vector3 &direction, SurfaceSample &sample) const = 0;

		RNDeclareMetaAPI(JoltCustomPlanetTerrainProvider, JTAPI)
	};

	class JoltCustomPlanetTerrainShape : public JoltShape
	{
	public:
		JTAPI JoltCustomPlanetTerrainShape(JoltCustomPlanetTerrainProvider *provider, bool solidRecoveryOnly = false);

		JTAPI static JoltCustomPlanetTerrainShape *WithProvider(JoltCustomPlanetTerrainProvider *provider, bool solidRecoveryOnly = false);
		JTAPI static void RegisterJoltShape();

		RNDeclareMetaAPI(JoltCustomPlanetTerrainShape, JTAPI)
	};
} // namespace RN

#endif /* defined(__RAYNE_JOLTCUSTOMPLANETTERRAINSHAPE_H_) */
