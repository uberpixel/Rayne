//
//  RNJoltCustomPlanetTerrainShape.cpp
//  Rayne-Jolt
//
//  Copyright 2026 by Überpixel. All rights reserved.
//

#include "RNJoltCustomPlanetTerrainShape.h"
#include "RNJoltCustomPlanetTerrainShapeInternal.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

namespace RN
{
	RNDefineMeta(JoltCustomPlanetTerrainProvider, Object)
	RNDefineMeta(JoltCustomPlanetTerrainShape, JoltShape)

	class JoltCustomPlanetTerrainProviderAdapter final : public JoltCustomPlanetTerrainInternalProvider
	{
	public:
		JoltCustomPlanetTerrainProviderAdapter(JoltCustomPlanetTerrainProvider *provider) :
			_referenceCount(1),
			_provider(provider ? provider->Retain() : nullptr)
		{}

		void RetainProvider() override
		{
			_referenceCount += 1;
		}

		void ReleaseProvider() override
		{
			_referenceCount -= 1;
			if(_referenceCount == 0) delete this;
		}

		float GetMaximumPlanetTerrainRadius() const override
		{
			return _provider ? _provider->GetMaximumPlanetTerrainRadius() : 0.0f;
		}

		void GetPlanetTerrainLocalOrigin(double &x, double &y, double &z) const override
		{
			JoltPosition origin;
			if(_provider) origin = _provider->GetPlanetTerrainLocalOrigin();
			x = origin.x;
			y = origin.y;
			z = origin.z;
		}

		unsigned int GetPlanetTerrainCollisionRevision() const override
		{
			return _provider ? _provider->GetPlanetTerrainCollisionRevision() : 0;
		}

		unsigned int GetPlanetTerrainCollisionCacheEpoch() const override
		{
			return _provider ? _provider->GetPlanetTerrainCollisionCacheEpoch() : 0;
		}

		bool CanReusePlanetTerrainCollisionSample(float x, float y, float z, unsigned int oldRevision, unsigned int oldEpoch, unsigned int revision, unsigned int epoch) const override
		{
			return _provider && _provider->CanReusePlanetTerrainCollisionSample(Vector3(x, y, z), oldRevision, oldEpoch, revision, epoch);
		}

		bool SamplePlanetTerrain(float directionX, float directionY, float directionZ, JoltCustomPlanetTerrainSample &sample) const override
		{
			if(!_provider) return false;

			JoltCustomPlanetTerrainProvider::SurfaceSample surfaceSample;
			if(!_provider->SamplePlanetTerrain(Vector3(directionX, directionY, directionZ), surfaceSample)) return false;

			sample.positionX = surfaceSample.position.x;
			sample.positionY = surfaceSample.position.y;
			sample.positionZ = surfaceSample.position.z;
			sample.normalX = surfaceSample.normal.x;
			sample.normalY = surfaceSample.normal.y;
			sample.normalZ = surfaceSample.normal.z;
			sample.radius = surfaceSample.radius;
			return true;
		}

	private:
		~JoltCustomPlanetTerrainProviderAdapter() override
		{
			if(_provider) _provider->Release();
		}

		uint32 _referenceCount;
		JoltCustomPlanetTerrainProvider *_provider;
	};

	JoltPosition JoltCustomPlanetTerrainProvider::GetPlanetTerrainLocalOrigin() const
	{
		return JoltPosition();
	}

	uint32 JoltCustomPlanetTerrainProvider::GetPlanetTerrainCollisionRevision() const
	{
		return 0;
	}

	uint32 JoltCustomPlanetTerrainProvider::GetPlanetTerrainCollisionCacheEpoch() const
	{
		return 0;
	}

	bool JoltCustomPlanetTerrainProvider::CanReusePlanetTerrainCollisionSample([[maybe_unused]] const Vector3 &direction, [[maybe_unused]] uint32 oldRevision, [[maybe_unused]] uint32 oldEpoch, [[maybe_unused]] uint32 revision, [[maybe_unused]] uint32 epoch) const
	{
		return false;
	}

	JoltCustomPlanetTerrainShape::JoltCustomPlanetTerrainShape(JoltCustomPlanetTerrainProvider *provider, bool solidRecoveryOnly)
	{
		JoltCustomPlanetTerrainProviderAdapter *adapter = new JoltCustomPlanetTerrainProviderAdapter(provider);
		_shape = JoltCustomPlanetTerrainInternal::CreateShape(adapter, solidRecoveryOnly);
		adapter->ReleaseProvider();
		_shape->AddRef();
	}

	JoltCustomPlanetTerrainShape *JoltCustomPlanetTerrainShape::WithProvider(JoltCustomPlanetTerrainProvider *provider, bool solidRecoveryOnly)
	{
		JoltCustomPlanetTerrainShape *shape = new JoltCustomPlanetTerrainShape(provider, solidRecoveryOnly);
		return shape->Autorelease();
	}

	void JoltCustomPlanetTerrainShape::RegisterJoltShape()
	{
		JoltCustomPlanetTerrainInternal::RegisterShape();
	}
} // namespace RN
