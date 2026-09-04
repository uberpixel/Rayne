//
//  RNJoltCustomPlanetTerrainShapeInternal.cpp
//  Rayne-Jolt
//
//  Copyright 2026 by Überpixel. All rights reserved.
//

#include "RNJoltCustomPlanetTerrainShapeInternal.h"

#include <Jolt/Jolt.h>
#include <Jolt/Geometry/ClipPoly.h>
#include <Jolt/Geometry/ClosestPoint.h>
#include <Jolt/Geometry/RayTriangle.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/CastConvexVsTriangles.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollideConvexVsTriangles.h>
#include <Jolt/Physics/Collision/CollisionDispatch.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/TransformedShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexShape.h>
#include <Jolt/Physics/Collision/Shape/ScaleHelpers.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#ifdef JPH_DEBUG_RENDERER
	#include <Jolt/Renderer/DebugRenderer.h>
#endif

JPH_NAMESPACE_BEGIN

class RNCustomPlanetTerrainShape final : public Shape
{
public:
	JPH_OVERRIDE_NEW_DELETE

	struct Triangle
	{
		Vec3 vertices[3];
		uint32 id;
	};

	struct PrecisionBase
	{
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		Vec3 vector = Vec3::sZero();
	};

	RNCustomPlanetTerrainShape() :
		Shape(EShapeType::User1, EShapeSubType::User1),
		_provider(nullptr),
		_boundsRadius(0.0f),
		_solidRecoveryOnly(false)
	{}

	RNCustomPlanetTerrainShape(RN::JoltCustomPlanetTerrainInternalProvider *provider, bool solidRecoveryOnly) :
		Shape(EShapeType::User1, EShapeSubType::User1),
		_provider(provider),
		_boundsRadius(0.0f),
		_solidRecoveryOnly(solidRecoveryOnly)
	{
		if(_provider)
		{
			_provider->RetainProvider();
			_boundsRadius = _provider->GetMaximumPlanetTerrainRadius();
			if(_boundsRadius < 0.0f) _boundsRadius = 0.0f;
		}
	}

	~RNCustomPlanetTerrainShape() override
	{
		if(_provider) _provider->ReleaseProvider();
	}

	bool MustBeStatic() const override
	{
		return true;
	}

	AABox GetLocalBounds() const override
	{
		return AABox(Vec3::sReplicate(-_boundsRadius), Vec3::sReplicate(_boundsRadius));
	}

	uint GetSubShapeIDBitsRecursive() const override
	{
		return TriangleSubShapeIDBits;
	}

	float GetInnerRadius() const override
	{
		return 0.0f;
	}

	MassProperties GetMassProperties() const override
	{
		return MassProperties();
	}

	const PhysicsMaterial *GetMaterial([[maybe_unused]] const SubShapeID &subShapeID) const override
	{
		return PhysicsMaterial::sDefault;
	}

	Vec3 GetSurfaceNormal([[maybe_unused]] const SubShapeID &subShapeID, Vec3Arg localSurfacePosition) const override
	{
		const PrecisionBase localOrigin = GetLocalOrigin();
		Triangle triangle;
		if(GetTriangleBySubShapeID(subShapeID, MakePrecisionBase(RVec3(localSurfacePosition)), localOrigin, triangle))
		{
			const Vec3 normal = GetTriangleNormal(triangle);
			if(normal.LengthSq() > 0.0f) return normal;
		}

		const Vec3 direction = GetPlanetLocalDirection(localSurfacePosition, MakePrecisionBase(RVec3::sZero()), localOrigin);
		Vec3 position;
		Vec3 normal;
		float radius = 0.0f;
		if(Sample(direction, position, normal, radius)) return normal;
		return direction;
	}

	void GetSupportingFace(const SubShapeID &subShapeID, [[maybe_unused]] Vec3Arg direction, Vec3Arg scale, Mat44Arg centerOfMassTransform, SupportingFace &outVertices) const override
	{
		const PrecisionBase localOrigin = GetLocalOrigin();
		Triangle triangle;
		if(!GetTriangleBySubShapeID(subShapeID, MakePrecisionBase(RVec3::sZero()), localOrigin, triangle))
		{
			outVertices.clear();
			return;
		}

		outVertices.resize(3);
		outVertices[0] = triangle.vertices[0];
		outVertices[1] = triangle.vertices[1];
		outVertices[2] = triangle.vertices[2];

		if(ScaleHelpers::IsInsideOut(scale))
		{
			const Vec3 vertex = outVertices[1];
			outVertices[1] = outVertices[2];
			outVertices[2] = vertex;
		}

		const Mat44 transform = centerOfMassTransform.PreScaled(scale);
		for(Vec3 &vertex : outVertices)
		{
			vertex = transform * vertex;
		}
	}

	void GetSubmergedVolume([[maybe_unused]] Mat44Arg centerOfMassTransform, [[maybe_unused]] Vec3Arg scale, [[maybe_unused]] const Plane &surface, float &totalVolume, float &submergedVolume, Vec3 &centerOfBuoyancy
#ifdef JPH_DEBUG_RENDERER
		, [[maybe_unused]] RVec3Arg baseOffset
#endif
		) const override
	{
		totalVolume = 0.0f;
		submergedVolume = 0.0f;
		centerOfBuoyancy = Vec3::sZero();
	}

#ifdef JPH_DEBUG_RENDERER
	void Draw([[maybe_unused]] DebugRenderer *renderer, [[maybe_unused]] RMat44Arg centerOfMassTransform, [[maybe_unused]] Vec3Arg scale, [[maybe_unused]] ColorArg color, [[maybe_unused]] bool useMaterialColors, [[maybe_unused]] bool drawWireframe) const override
	{}
#endif

	bool CastRay(const RayCast &ray, const SubShapeIDCreator &subShapeIDCreator, RayCastResult &hit) const override
	{
		if(GetSignedDistance(ray.mOrigin) <= 0.0f)
		{
			hit.mFraction = 0.0f;
			hit.mSubShapeID2 = subShapeIDCreator.GetID();
			return true;
		}

		RayCastSettings settings;
		return FindRayTriangleHit(ray, settings, subShapeIDCreator, hit);
	}

	void CastRay(const RayCast &ray, const RayCastSettings &rayCastSettings, const SubShapeIDCreator &subShapeIDCreator, CastRayCollector &collector, const ShapeFilter &shapeFilter = { }) const override
	{
		if(!shapeFilter.ShouldCollide(this, subShapeIDCreator.GetID())) return;
		if(GetSignedDistance(ray.mOrigin) <= 0.0f)
		{
			RayCastResult result;
			result.mFraction = 0.0f;
			result.mSubShapeID2 = subShapeIDCreator.GetID();
			collector.AddHit(result);
			return;
		}

		CastRayTriangles(ray, rayCastSettings, subShapeIDCreator, collector);
	}

	void CollidePoint(Vec3Arg point, const SubShapeIDCreator &subShapeIDCreator, CollidePointCollector &collector, const ShapeFilter &shapeFilter = { }) const override
	{
		if(!shapeFilter.ShouldCollide(this, subShapeIDCreator.GetID())) return;
		if(GetSignedDistance(point) > 0.0f) return;
		collector.AddHit({ TransformedShape::sGetBodyID(collector.GetContext()), subShapeIDCreator.GetID() });
	}

	void CollideSoftBodyVertices([[maybe_unused]] Mat44Arg centerOfMassTransform, [[maybe_unused]] Vec3Arg scale, [[maybe_unused]] const CollideSoftBodyVertexIterator &vertices, [[maybe_unused]] uint numVertices, [[maybe_unused]] int collidingShapeIndex) const override
	{}

	void GetTrianglesStart(GetTrianglesContext &context, const AABox &box, [[maybe_unused]] Vec3Arg positionCOM, [[maybe_unused]] QuatArg rotation, [[maybe_unused]] Vec3Arg scale) const override
	{
		TriangleContext *triangleContext = reinterpret_cast<TriangleContext *>(&context);
		triangleContext->count = 0;
		triangleContext->index = 0;

		Array<Triangle> triangles;
		CollectTriangles(box, 0.0f, triangles, MaxContextTriangles);
		triangleContext->count = static_cast<uint>(triangles.size());
		for(uint i = 0; i < triangleContext->count; i += 1)
		{
			triangleContext->triangles[i] = triangles[i];
		}
	}

	int GetTrianglesNext(GetTrianglesContext &context, int maxTrianglesRequested, Float3 *triangleVertices, [[maybe_unused]] const PhysicsMaterial **materials = nullptr) const override
	{
		TriangleContext *triangleContext = reinterpret_cast<TriangleContext *>(&context);
		int writtenCount = 0;
		while(writtenCount < maxTrianglesRequested && triangleContext->index < triangleContext->count)
		{
			const Triangle &triangle = triangleContext->triangles[triangleContext->index];
			for(uint vertex = 0; vertex < 3; vertex += 1)
			{
				const Vec3 &position = triangle.vertices[vertex];
				triangleVertices[writtenCount * 3 + vertex] = Float3(position.GetX(), position.GetY(), position.GetZ());
			}
			if(materials) materials[writtenCount] = PhysicsMaterial::sDefault;
			triangleContext->index += 1;
			writtenCount += 1;
		}
		return writtenCount;
	}

	Stats GetStats() const override
	{
		return Stats(sizeof(*this), 0);
	}

	float GetVolume() const override
	{
		return 0.0f;
	}

	bool IsValidScale(Vec3Arg scale) const override
	{
		return scale.GetX() != 0.0f && scale.GetY() != 0.0f && scale.GetZ() != 0.0f;
	}

	Vec3 MakeScaleValid(Vec3Arg scale) const override
	{
		return ScaleHelpers::MakeNonZeroScale(scale);
	}

	static void sRegister()
	{
		ShapeFunctions &functions = ShapeFunctions::sGet(EShapeSubType::User1);
		functions.mConstruct = []() -> Shape * { return new RNCustomPlanetTerrainShape; };
		functions.mColor = Color::sDarkGreen;

		for(EShapeSubType subtype : sConvexSubShapeTypes)
		{
			CollisionDispatch::sRegisterCollideShape(subtype, EShapeSubType::User1, sCollideConvexVsPlanetTerrain);
			CollisionDispatch::sRegisterCollideShape(EShapeSubType::User1, subtype, CollisionDispatch::sReversedCollideShape);
			CollisionDispatch::sRegisterCastShape(subtype, EShapeSubType::User1, sCastConvexVsPlanetTerrain);
			CollisionDispatch::sRegisterCastShape(EShapeSubType::User1, subtype, CollisionDispatch::sReversedCastShape);
		}
	}

	static void sInstallSimulationCollideBodyVsBody(PhysicsSystem *physicsSystem)
	{
		if(!physicsSystem) return;
		physicsSystem->SetSimCollideBodyVsBody(sSimCollideBodyVsBody);
	}

private:
	static constexpr uint MaxCollisionTriangles = 8192;
	static constexpr uint MaxCastTriangles = 8192;
	static constexpr uint MaxRayTriangles = 8192;
	static constexpr uint MaxContextTriangles = 64;
	static constexpr uint TriangleSubShapeIDBits = 32;
	static constexpr uint32 TriangleSubShapeIDMask = 0xffffffffu;
	static constexpr double CollisionGridCellSize = 1.0;
	static constexpr int CollisionGridQueryPaddingCells = 2;
	static constexpr int CollisionGridBlockCellBits = 13;
	static constexpr int CollisionGridBlockCellCount = 1 << CollisionGridBlockCellBits;
	static constexpr uint32 SampledTriangleIDFlag = 0x80000000u;
	static constexpr uint32 SampledTriangleFaceShift = 28;
	static constexpr uint32 SampledTriangleDiagonalShift = 27;
	static constexpr uint32 SampledTriangleGridUShift = 14;
	static constexpr uint32 SampledTriangleGridVShift = 1;
	static constexpr uint32 SampledTriangleGridMask = (1u << CollisionGridBlockCellBits) - 1u;
	static constexpr uint SampledGridVertexCacheSize = 8192;
	static constexpr uint32 SampledGridVertexCacheMask = SampledGridVertexCacheSize - 1u;
	static constexpr float MinimumTriangleNormalLengthSq = 1.0e-12f;
	static constexpr float MinimumSolidRecoverySupportDepth = 0.005f;
	static constexpr float AboveSurfaceQueryMargin = 4.0f;

	struct SampledGridKey
	{
		uint8 face = 0;
		uint8 diagonal = 0;
		int gridU = 0;
		int gridV = 0;
	};

	struct SampledGridFaceRange
	{
		bool valid = false;
		int minU = 0;
		int maxU = 0;
		int minV = 0;
		int maxV = 0;
	};

	struct CachedSampledGridVertex
	{
		const RNCustomPlanetTerrainShape *shape = nullptr;
		uint32 revision = 0;
		uint32 cacheEpoch = 0;
		uint8 face = 0xff;
		int gridU = 0;
		int gridV = 0;
		double absoluteX = 0.0;
		double absoluteY = 0.0;
		double absoluteZ = 0.0;
		Vec3 normal = Vec3::sZero();
		bool valid = false;
	};

	struct SampledGridVertexCache
	{
		CachedSampledGridVertex entries[SampledGridVertexCacheSize];
	};

	struct SampledGridRowScratch
	{
		Array<Vec3> positions[2];
		Array<uint8> valid[2];
	};

	static uint32 GetTriangleSubShapeID(uint32 triangleID)
	{
		uint32 subShapeID = triangleID & TriangleSubShapeIDMask;
		if(subShapeID == TriangleSubShapeIDMask) subShapeID -= 1u;
		return subShapeID;
	}

	static uint32 GetTriangleSubShapeID(const SubShapeID &subShapeID)
	{
		SubShapeID remainder;
		uint32 triangleID = subShapeID.PopID(TriangleSubShapeIDBits, remainder);
		if(triangleID == TriangleSubShapeIDMask) triangleID -= 1u;
		return triangleID;
	}

	static bool IsSampledTriangleID(uint32 triangleID)
	{
		return (triangleID & SampledTriangleIDFlag) != 0;
	}

	static uint32 GetSampledGridVertexHash(uint32 revision, uint32 cacheEpoch, uint8 face, int gridU, int gridV)
	{
		uint32 hash = revision + 0x9e3779b9u;
		hash ^= cacheEpoch + 0x165667b1u + (hash << 6u) + (hash >> 2u);
		hash ^= static_cast<uint32>(face) + 0x85ebca6bu + (hash << 6u) + (hash >> 2u);
		hash ^= static_cast<uint32>(gridU) + 0xc2b2ae35u + (hash << 6u) + (hash >> 2u);
		hash ^= static_cast<uint32>(gridV) + 0x27d4eb2fu + (hash << 6u) + (hash >> 2u);
		hash ^= hash >> 16u;
		hash *= 0x7feb352du;
		hash ^= hash >> 15u;
		hash *= 0x846ca68bu;
		hash ^= hash >> 16u;
		return hash;
	}

	static Vec3 GetTriangleNormal(const Triangle &triangle)
	{
		return (triangle.vertices[1] - triangle.vertices[0]).Cross(triangle.vertices[2] - triangle.vertices[0]).NormalizedOr(Vec3::sZero());
	}

	static Mat44 GetShiftedTransform(Mat44Arg transform, Vec3Arg worldBase)
	{
		Mat44 shiftedTransform = transform;
		shiftedTransform.SetTranslation(transform.GetTranslation() - worldBase);
		return shiftedTransform;
	}

	static Mat44 GetShiftedReferenceTransform(Mat44Arg transform)
	{
		Mat44 shiftedTransform = transform;
		shiftedTransform.SetTranslation(Vec3::sZero());
		return shiftedTransform;
	}

	static PrecisionBase MakePrecisionBase(RVec3Arg localBase)
	{
		PrecisionBase base;
		base.x = localBase.GetX();
		base.y = localBase.GetY();
		base.z = localBase.GetZ();
		base.vector = Vec3(static_cast<float>(base.x), static_cast<float>(base.y), static_cast<float>(base.z));
		return base;
	}

	static bool Normalize(double &x, double &y, double &z)
	{
		const double length = sqrt(x * x + y * y + z * z);
		if(length <= 1.0e-8) return false;

		x /= length;
		y /= length;
		z /= length;
		return true;
	}

	static bool GetDirectionForPosition(Vec3Arg position, const PrecisionBase &localBase, const PrecisionBase &localOrigin, double &x, double &y, double &z)
	{
		x = static_cast<double>(position.GetX()) + localBase.x + localOrigin.x;
		y = static_cast<double>(position.GetY()) + localBase.y + localOrigin.y;
		z = static_cast<double>(position.GetZ()) + localBase.z + localOrigin.z;
		return Normalize(x, y, z);
	}

	static bool GetDirectionForOrigin(const PrecisionBase &localOrigin, double &x, double &y, double &z)
	{
		x = localOrigin.x;
		y = localOrigin.y;
		z = localOrigin.z;
		return Normalize(x, y, z);
	}

	static bool GetFaceCoordinatesOnFace(double x, double y, double z, uint8 face, double &u, double &v)
	{
		switch(face)
		{
			case 0:
				if(x <= 1.0e-8) return false;
				u = -z / x;
				v = y / x;
				break;
			case 1:
			{
				const double scale = -x;
				if(scale <= 1.0e-8) return false;
				u = z / scale;
				v = y / scale;
				break;
			}
			case 2:
				if(y <= 1.0e-8) return false;
				u = x / y;
				v = -z / y;
				break;
			case 3:
			{
				const double scale = -y;
				if(scale <= 1.0e-8) return false;
				u = x / scale;
				v = z / scale;
				break;
			}
			case 4:
				if(z <= 1.0e-8) return false;
				u = x / z;
				v = y / z;
				break;
			default:
			{
				const double scale = -z;
				if(scale <= 1.0e-8) return false;
				u = -x / scale;
				v = y / scale;
				break;
			}
		}

		return u >= -1.25 && u <= 1.25 && v >= -1.25 && v <= 1.25;
	}

	static Vec3 GetCubeSphereDirection(uint8 face, double u, double v)
	{
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		switch(face)
		{
			case 0:
				x = 1.0;
				y = v;
				z = -u;
				break;
			case 1:
				x = -1.0;
				y = v;
				z = u;
				break;
			case 2:
				x = u;
				y = 1.0;
				z = -v;
				break;
			case 3:
				x = u;
				y = -1.0;
				z = v;
				break;
			case 4:
				x = u;
				y = v;
				z = 1.0;
				break;
			default:
				x = -u;
				y = v;
				z = -1.0;
				break;
		}

		if(!Normalize(x, y, z)) return Vec3::sAxisY();
		return Vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
	}

	static double GetGridReferenceRadius(const PrecisionBase &localOrigin, float boundsRadius)
	{
		if(boundsRadius > 1.0f) return static_cast<double>(boundsRadius);

		const double radius = sqrt(localOrigin.x * localOrigin.x + localOrigin.y * localOrigin.y + localOrigin.z * localOrigin.z);
		return radius > 1.0 ? radius : 1.0;
	}

	static int GetGridCoordinate(double coordinateMeters)
	{
		const double scaledCoordinate = coordinateMeters / CollisionGridCellSize;
		int result = static_cast<int>(scaledCoordinate);
		if(static_cast<double>(result) > scaledCoordinate) result -= 1;
		return result;
	}

	static int GetNearestGridCoordinate(double coordinateMeters)
	{
		const double scaledCoordinate = coordinateMeters / CollisionGridCellSize;
		int result = GetGridCoordinate(coordinateMeters);
		if(scaledCoordinate - static_cast<double>(result) >= 0.5) result += 1;
		return result;
	}

	static double GetAbsoluteValue(double value)
	{
		return value < 0.0 ? -value : value;
	}

	static uint8 GetDominantCubeSphereFace(double x, double y, double z)
	{
		const double absX = GetAbsoluteValue(x);
		const double absY = GetAbsoluteValue(y);
		const double absZ = GetAbsoluteValue(z);
		if(absX >= absY && absX >= absZ) return x >= 0.0 ? 0 : 1;
		if(absY >= absZ) return y >= 0.0 ? 2 : 3;
		return z >= 0.0 ? 4 : 5;
	}

	static int GetGridBlockOrigin(int gridCoordinate)
	{
		int block = gridCoordinate / CollisionGridBlockCellCount;
		if(gridCoordinate < 0 && gridCoordinate % CollisionGridBlockCellCount != 0) block -= 1;
		return block * CollisionGridBlockCellCount;
	}

	static void GetSampledGridBlockOrigin(uint8 face, const PrecisionBase &localOrigin, double referenceRadius, int &originU, int &originV)
	{
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		double u = 0.0;
		double v = 0.0;
		if(GetDirectionForOrigin(localOrigin, x, y, z) && GetFaceCoordinatesOnFace(x, y, z, face, u, v))
		{
			originU = GetGridBlockOrigin(GetGridCoordinate(u * referenceRadius));
			originV = GetGridBlockOrigin(GetGridCoordinate(v * referenceRadius));
			return;
		}

		originU = 0;
		originV = 0;
	}

	static bool MakeSampledTriangleID(uint8 face, uint8 diagonal, int gridU, int gridV, const PrecisionBase &localOrigin, double referenceRadius, uint32 &id)
	{
		if(face >= 6 || diagonal > 1) return false;

		int originU = 0;
		int originV = 0;
		GetSampledGridBlockOrigin(face, localOrigin, referenceRadius, originU, originV);
		const int localU = gridU - originU;
		const int localV = gridV - originV;
		if(localU < 0 || localU >= CollisionGridBlockCellCount) return false;
		if(localV < 0 || localV >= CollisionGridBlockCellCount) return false;

		id = SampledTriangleIDFlag |
			 (static_cast<uint32>(face) << SampledTriangleFaceShift) |
			 (static_cast<uint32>(diagonal) << SampledTriangleDiagonalShift) |
			 (static_cast<uint32>(localU) << SampledTriangleGridUShift) |
			 (static_cast<uint32>(localV) << SampledTriangleGridVShift);
		return true;
	}

	static bool DecodeSampledTriangleID(uint32 id, const PrecisionBase &localOrigin, double referenceRadius, SampledGridKey &key)
	{
		if(!IsSampledTriangleID(id)) return false;

		key.face = static_cast<uint8>((id >> SampledTriangleFaceShift) & 0x7u);
		key.diagonal = static_cast<uint8>((id >> SampledTriangleDiagonalShift) & 0x1u);
		if(key.face >= 6) return false;

		int originU = 0;
		int originV = 0;
		GetSampledGridBlockOrigin(key.face, localOrigin, referenceRadius, originU, originV);
		key.gridU = originU + static_cast<int>((id >> SampledTriangleGridUShift) & SampledTriangleGridMask);
		key.gridV = originV + static_cast<int>((id >> SampledTriangleGridVShift) & SampledTriangleGridMask);
		return true;
	}

	static void IncludeSampledGridCoordinate(SampledGridFaceRange &range, int gridU, int gridV)
	{
		if(!range.valid)
		{
			range.valid = true;
			range.minU = gridU;
			range.maxU = gridU;
			range.minV = gridV;
			range.maxV = gridV;
			return;
		}

		if(gridU < range.minU) range.minU = gridU;
		if(gridU > range.maxU) range.maxU = gridU;
		if(gridV < range.minV) range.minV = gridV;
		if(gridV > range.maxV) range.maxV = gridV;
	}

	static bool DoesTriangleOverlapBox(const Triangle &triangle, const AABox &box)
	{
		Vec3 minimum = triangle.vertices[0];
		Vec3 maximum = triangle.vertices[0];
		for(uint i = 1; i < 3; i += 1)
		{
			const Vec3 vertex = triangle.vertices[i];
			minimum = Vec3(vertex.GetX() < minimum.GetX() ? vertex.GetX() : minimum.GetX(),
						   vertex.GetY() < minimum.GetY() ? vertex.GetY() : minimum.GetY(),
						   vertex.GetZ() < minimum.GetZ() ? vertex.GetZ() : minimum.GetZ());
			maximum = Vec3(vertex.GetX() > maximum.GetX() ? vertex.GetX() : maximum.GetX(),
						   vertex.GetY() > maximum.GetY() ? vertex.GetY() : maximum.GetY(),
						   vertex.GetZ() > maximum.GetZ() ? vertex.GetZ() : maximum.GetZ());
		}

		if(maximum.GetX() < box.mMin.GetX() || minimum.GetX() > box.mMax.GetX()) return false;
		if(maximum.GetY() < box.mMin.GetY() || minimum.GetY() > box.mMax.GetY()) return false;
		if(maximum.GetZ() < box.mMin.GetZ() || minimum.GetZ() > box.mMax.GetZ()) return false;
		return true;
	}

	PrecisionBase GetLocalOrigin() const
	{
		if(!_provider) return MakePrecisionBase(RVec3::sZero());

		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		_provider->GetPlanetTerrainLocalOrigin(x, y, z);
		return MakePrecisionBase(RVec3(x, y, z));
	}

	static PrecisionBase GetCollisionLocalBase(Mat44Arg shapeTransform, Mat44Arg planetTransform)
	{
		return MakePrecisionBase(RVec3(planetTransform.InversedRotationTranslation() * shapeTransform.GetTranslation()));
	}

	static PrecisionBase GetCastLocalBase(const ShapeCast &shapeCast, Mat44Arg planetTransform)
	{
		return MakePrecisionBase(RVec3(planetTransform.InversedRotationTranslation() * shapeCast.mCenterOfMassStart.GetTranslation()));
	}

	static PrecisionBase GetSimulationCollisionLocalBase(const Body &shapeBody, const Body &planetBody)
	{
		return MakePrecisionBase(planetBody.GetInverseCenterOfMassTransform() * shapeBody.GetCenterOfMassPosition());
	}

	static Vec3 GetOffsetPosition(double x, double y, double z, const PrecisionBase &localBase)
	{
		return Vec3(static_cast<float>(x - localBase.x),
					static_cast<float>(y - localBase.y),
					static_cast<float>(z - localBase.z));
	}

	static double GetPlanetLocalRadius(Vec3Arg position, const PrecisionBase &localBase, const PrecisionBase &localOrigin)
	{
		const double x = static_cast<double>(position.GetX()) + localBase.x + localOrigin.x;
		const double y = static_cast<double>(position.GetY()) + localBase.y + localOrigin.y;
		const double z = static_cast<double>(position.GetZ()) + localBase.z + localOrigin.z;
		return sqrt(x * x + y * y + z * z);
	}

	static Vec3 GetPlanetLocalDirection(Vec3Arg position, const PrecisionBase &localBase, const PrecisionBase &localOrigin)
	{
		const double x = static_cast<double>(position.GetX()) + localBase.x + localOrigin.x;
		const double y = static_cast<double>(position.GetY()) + localBase.y + localOrigin.y;
		const double z = static_cast<double>(position.GetZ()) + localBase.z + localOrigin.z;
		const double radius = sqrt(x * x + y * y + z * z);
		if(radius <= 1.0e-8) return Vec3::sAxisY();

		return Vec3(static_cast<float>(x / radius), static_cast<float>(y / radius), static_cast<float>(z / radius));
	}

	static AABox GetShapeCastBounds(const ShapeCast &shapeCast)
	{
		AABox bounds = shapeCast.mShapeWorldBounds;
		AABox endBounds = shapeCast.mShapeWorldBounds;
		endBounds.Translate(shapeCast.mDirection);
		bounds.Encapsulate(endBounds);
		return bounds;
	}

	static AABox GetRayBounds(const RayCast &ray, float maximumFraction)
	{
		AABox bounds(ray.mOrigin, ray.mOrigin);
		bounds.Encapsulate(ray.mOrigin + ray.mDirection * maximumFraction);
		return bounds;
	}

	static bool ShouldRayHitTriangle(const RayCast &ray, const RayCastSettings &settings, Vec3Arg triangleNormal)
	{
		if(settings.mBackFaceModeTriangles == EBackFaceMode::CollideWithBackFaces) return true;
		return ray.mDirection.Dot(triangleNormal) < 0.0f;
	}

	static AABox GetSurfaceQueryBox(const AABox &box, const PrecisionBase &localBase, const PrecisionBase &localOrigin, float sweepDistance)
	{
		if(sweepDistance <= 0.0f) return box;

		const Vec3 direction = GetPlanetLocalDirection(box.GetCenter(), localBase, localOrigin);
		AABox sweptBox = box;

		AABox outwardBox = box;
		outwardBox.Translate(direction * sweepDistance);
		sweptBox.Encapsulate(outwardBox);

		AABox inwardBox = box;
		inwardBox.Translate(direction * -sweepDistance);
		sweptBox.Encapsulate(inwardBox);

		float tangentXSq = 1.0f - direction.GetX() * direction.GetX();
		float tangentYSq = 1.0f - direction.GetY() * direction.GetY();
		float tangentZSq = 1.0f - direction.GetZ() * direction.GetZ();
		if(tangentXSq < 0.0f) tangentXSq = 0.0f;
		if(tangentYSq < 0.0f) tangentYSq = 0.0f;
		if(tangentZSq < 0.0f) tangentZSq = 0.0f;

		const float tangentX = Sqrt(tangentXSq);
		const float tangentY = Sqrt(tangentYSq);
		const float tangentZ = Sqrt(tangentZSq);
		sweptBox.ExpandBy(Vec3(tangentX, tangentY, tangentZ) * sweepDistance);

		return sweptBox;
	}

	static void TranslateResult(CollideShapeResult &result, Vec3Arg offset)
	{
		result.mContactPointOn1 += offset;
		result.mContactPointOn2 += offset;
		for(Vec3 &vertex : result.mShape1Face) vertex += offset;
		for(Vec3 &vertex : result.mShape2Face) vertex += offset;
	}

	static void BuildSurfaceFace(Vec3Arg center, Vec3Arg normal, float extent, CollideShapeResult::Face &face)
	{
		Vec3 tangent = normal.Cross(Vec3::sAxisX());
		if(tangent.LengthSq() <= 1.0e-6f) tangent = normal.Cross(Vec3::sAxisY());
		tangent = tangent.NormalizedOr(Vec3::sAxisZ());
		const Vec3 bitangent = normal.Cross(tangent).NormalizedOr(Vec3::sAxisX());

		face.resize(4);
		face[0] = center - tangent * extent - bitangent * extent;
		face[1] = center + tangent * extent - bitangent * extent;
		face[2] = center + tangent * extent + bitangent * extent;
		face[3] = center - tangent * extent + bitangent * extent;
	}

	class OffsetCollideShapeCollector final : public CollideShapeCollector
	{
	public:
		OffsetCollideShapeCollector(CollideShapeCollector &collector, Vec3Arg offset, const ConvexShape *convex, Vec3Arg scale, Mat44Arg transform, const CollideShapeSettings &settings) :
			CollideShapeCollector(collector),
			_collector(collector),
			_offset(offset),
			_convex(convex),
			_scale(scale),
			_transform(transform),
			_settings(settings)
		{
			// GJK's edge distance cannot bound the depth of the surface contact we build below.
			if(!collector.ShouldEarlyOut()) ResetEarlyOutFraction();
		}

		bool FinishTriangles()
		{
			_collectingTriangles = false;
			// A real ridge can have only an edge contact. Prefer face contacts whenever
			// available, but retain the deepest raw contact if no triangle supports a face.
			if(!_hasUsefulHit && _hasEdgeContact) AddHit(_edgeContact);
			UpdateEarlyOutFraction(_collector.GetEarlyOutFraction());
			return _hasUsefulHit;
		}

		void AddHit(const CollideShapeResult &result) override
		{
			CollideShapeResult surfaceResult = result;
			if(_collectingTriangles)
			{
				const CollideShapeResult::Face &triangle = result.mShape2Face;
				JPH_ASSERT(triangle.size() == 3);
				const Vec3 normal = (triangle[1] - triangle[0]).Cross(triangle[2] - triangle[0]).NormalizedOr(Vec3::sZero());
				if(normal.IsNearZero() || result.mPenetrationAxis.Dot(normal) >= 0.0f) return;
				if(!BuildSurfaceContact(surfaceResult, normal))
				{
					if(result.mPenetrationDepth >= -_settings.mMaxSeparationDistance && (!_hasEdgeContact || result.mPenetrationDepth > _edgeContact.mPenetrationDepth))
					{
						_edgeContact = result;
						_hasEdgeContact = true;
					}
					return;
				}
			}
			if(surfaceResult.mPenetrationDepth >= -_settings.mMaxSeparationDistance) _hasUsefulHit = true;
			if(-surfaceResult.mPenetrationDepth >= _collector.GetEarlyOutFraction()) return;

			if(_settings.mCollectFacesMode == ECollectFacesMode::NoFaces)
			{
				surfaceResult.mShape1Face.clear();
				surfaceResult.mShape2Face.clear();
			}
			RNCustomPlanetTerrainShape::TranslateResult(surfaceResult, _offset);
			_collector.AddHit(surfaceResult);
			if(_collector.ShouldEarlyOut()) ForceEarlyOut();
		}

	private:
		bool BuildSurfaceContact(CollideShapeResult &result, Vec3Arg normal) const
		{
			const CollideShapeResult::Face &triangle = result.mShape2Face;

			// Terrain edges are internal. Build the contact from the face toward the terrain,
			// not from GJK's closest point on a neighbouring triangle edge.
			const Vec3 localNormal = _transform.Multiply3x3Transposed(normal);
			CollideShapeResult::Face &face = result.mShape1Face;
			face.clear();
			_convex->GetSupportingFace(SubShapeID(), localNormal, _scale, _transform, face);

			if(face.size() >= 2)
			{
				CollideShapeResult::Face clipped;
				const Vec3 edge = face[1] - face[0];
				Vec3 faceNormal;
				if(face.size() >= 3)
				{
					ClipPolyVsPoly(triangle, face, -normal, clipped);
					faceNormal = edge.Cross(face[2] - face[0]);
				}
				else
				{
					ClipPolyVsEdge(triangle, face[0], face[1], -normal, clipped);
					faceNormal = edge.Cross(-normal).Cross(edge);
				}

				// Jolt otherwise falls back to the original edge point when clipping is empty.
				// Leave this candidate for FinishTriangles instead of mixing it into face contacts.
				const float denominator = normal.Dot(faceNormal);
				if(clipped.empty() || denominator == 0.0f) return false;

				result.mPenetrationDepth = -FLT_MAX;
				for(Vec3Arg point : clipped)
				{
					const float depth = (point - face[0]).Dot(faceNormal) / denominator;
					if(depth > result.mPenetrationDepth)
					{
						result.mPenetrationDepth = depth;
						result.mContactPointOn1 = point - normal * depth;
						result.mContactPointOn2 = point;
					}
				}
			}
			else
			{
				// Spheres and other point supports have no face to clip. Their support point
				// must project onto this triangle, within the existing collision tolerance.
				ConvexShape::SupportBuffer buffer;
				const ConvexShape::Support *support = _convex->GetSupportFunction(ConvexShape::ESupportMode::Default, buffer, _scale);
				result.mContactPointOn1 = _transform * support->GetSupport(-localNormal) - normal * support->GetConvexRadius();
				result.mPenetrationDepth = (triangle[0] - result.mContactPointOn1).Dot(normal);
				result.mContactPointOn2 = result.mContactPointOn1 + normal * result.mPenetrationDepth;
				uint32 closestSet;
				const Vec3 closest = ClosestPoint::GetClosestPointOnTriangle(triangle[0] - result.mContactPointOn2, triangle[1] - result.mContactPointOn2, triangle[2] - result.mContactPointOn2, closestSet);
				if(closest.LengthSq() > Square(_settings.mCollisionTolerance)) return false;
			}

			result.mPenetrationAxis = -normal;
			return result.mPenetrationDepth >= -_settings.mMaxSeparationDistance;
		}

		CollideShapeCollector &_collector;
		Vec3 _offset;
		const ConvexShape *_convex;
		Vec3 _scale;
		Mat44 _transform;
		const CollideShapeSettings &_settings;
		CollideShapeResult _edgeContact;
		bool _hasEdgeContact = false;
		bool _hasUsefulHit = false;
		bool _collectingTriangles = true;
	};

	class OffsetCastShapeCollector final : public CastShapeCollector
	{
	public:
		OffsetCastShapeCollector(CastShapeCollector &collector, Vec3Arg offset) :
			CastShapeCollector(collector),
			_collector(collector),
			_offset(offset)
		{}

		void AddHit(const ShapeCastResult &result) override
		{
			ShapeCastResult offsetResult = result;
			RNCustomPlanetTerrainShape::TranslateResult(offsetResult, _offset);
			_collector.AddHit(offsetResult);
			UpdateEarlyOutFraction(_collector.GetEarlyOutFraction());
		}

	private:
		CastShapeCollector &_collector;
		Vec3 _offset;
	};

	class ReversedCollideShapeCollector final : public CollideShapeCollector
	{
	public:
		explicit ReversedCollideShapeCollector(CollideShapeCollector &collector) :
			CollideShapeCollector(collector),
			_collector(collector)
		{}

		void AddHit(const CollideShapeResult &result) override
		{
			_collector.AddHit(result.Reversed());
			UpdateEarlyOutFraction(_collector.GetEarlyOutFraction());
		}

	private:
		CollideShapeCollector &_collector;
	};

	struct TriangleContext
	{
		Triangle triangles[MaxContextTriangles];
		uint count;
		uint index;
	};

	static_assert(sizeof(TriangleContext) <= sizeof(GetTrianglesContext), "Triangle context is too large.");

	static void CollideConvexVsPlanetTerrain(const ConvexShape *convex, const RNCustomPlanetTerrainShape *planet, Vec3Arg scale1, Vec3Arg scale2, Mat44Arg transform1, Mat44Arg transform2, const SubShapeID &subShapeID1, const SubShapeIDCreator &subShapeIDCreator2, const CollideShapeSettings &settings, CollideShapeCollector &collector, const PrecisionBase &localBase, Vec3Arg worldBase)
	{
		struct Visitor : public CollideConvexVsTriangles
		{
			using CollideConvexVsTriangles::CollideConvexVsTriangles;

			bool ShouldAbort() const
			{
				return mCollector.ShouldEarlyOut();
			}

			AABox GetQueryBounds() const
			{
				return mBoundsOf1InSpaceOf2;
			}

			void VisitTriangle(const Triangle &triangle, [[maybe_unused]] Vec3Arg normal)
			{
				Collide(triangle.vertices[0], triangle.vertices[1], triangle.vertices[2], 0, subShapeIDCreator.PushID(GetTriangleSubShapeID(triangle.id), TriangleSubShapeIDBits).GetID());
			}

			SubShapeIDCreator subShapeIDCreator;
		};

		const Mat44 shiftedTransform1 = GetShiftedTransform(transform1, worldBase);
		const Mat44 shiftedTransform2 = GetShiftedReferenceTransform(transform2);

		OffsetCollideShapeCollector offsetCollector(collector, worldBase, convex, scale1, shiftedTransform1, settings);
		if(!planet->_solidRecoveryOnly)
		{
			// Keep raw GJK candidates until the collector has validated the surface overlap.
			// Rebuild face contacts together with their points and depth; retain raw edge fallback.
			CollideShapeSettings terrainSettings = settings;
			terrainSettings.mActiveEdgeMode = EActiveEdgeMode::CollideWithAll;
			terrainSettings.mCollectFacesMode = ECollectFacesMode::CollectFaces;
			Visitor visitor(convex, scale1, scale2, shiftedTransform1, shiftedTransform2, subShapeID1, terrainSettings, offsetCollector);
			visitor.subShapeIDCreator = subShapeIDCreator2;
			planet->VisitTriangles(visitor.GetQueryBounds(), settings.mMaxSeparationDistance, localBase, visitor, MaxCollisionTriangles);
		}
		if(!offsetCollector.FinishTriangles())
		{
			planet->AddSolidRecoveryContact(convex, scale1, scale2, shiftedTransform1, shiftedTransform2, subShapeID1, subShapeIDCreator2.GetID(), settings, offsetCollector, localBase);
		}
	}

	static void sCollideConvexVsPlanetTerrain(const Shape *shape1, const Shape *shape2, Vec3Arg scale1, Vec3Arg scale2, Mat44Arg transform1, Mat44Arg transform2, const SubShapeIDCreator &subShapeIDCreator1, const SubShapeIDCreator &subShapeIDCreator2, const CollideShapeSettings &settings, CollideShapeCollector &collector, [[maybe_unused]] const ShapeFilter &shapeFilter)
	{
		JPH_ASSERT(shape1->GetType() == EShapeType::Convex);
		JPH_ASSERT(shape2->GetSubType() == EShapeSubType::User1);
		const ConvexShape *convex = static_cast<const ConvexShape *>(shape1);
		const RNCustomPlanetTerrainShape *planet = static_cast<const RNCustomPlanetTerrainShape *>(shape2);

		const PrecisionBase localBase = GetCollisionLocalBase(transform1, transform2);
		const Vec3 worldBase = transform2 * localBase.vector;
		CollideConvexVsPlanetTerrain(convex, planet, scale1, scale2, transform1, transform2, subShapeIDCreator1.GetID(), subShapeIDCreator2, settings, collector, localBase, worldBase);
	}

	static void sSimCollideBodyVsBody(const Body &body1, const Body &body2, Mat44Arg transform1, Mat44Arg transform2, CollideShapeSettings &settings, CollideShapeCollector &collector, const ShapeFilter &shapeFilter)
	{
		const Shape *shape1 = body1.GetShape();
		const Shape *shape2 = body2.GetShape();
		const bool terrainFirst = shape1 && shape1->GetSubType() == EShapeSubType::User1;
		const Shape *convexShape = terrainFirst ? shape2 : shape1;
		const Shape *terrainShape = terrainFirst ? shape1 : shape2;
		if(!convexShape || !terrainShape || convexShape->GetType() != EShapeType::Convex || terrainShape->GetSubType() != EShapeSubType::User1)
		{
			PhysicsSystem::sDefaultSimCollideBodyVsBody(body1, body2, transform1, transform2, settings, collector, shapeFilter);
			return;
		}

		const SubShapeIDCreator subShapeIDCreator;
		if(!shapeFilter.ShouldCollide(shape1, subShapeIDCreator.GetID(), shape2, subShapeIDCreator.GetID())) return;

		const ConvexShape *convex = static_cast<const ConvexShape *>(convexShape);
		const RNCustomPlanetTerrainShape *planet = static_cast<const RNCustomPlanetTerrainShape *>(terrainShape);
		const PrecisionBase localBase = terrainFirst ? GetSimulationCollisionLocalBase(body2, body1) : GetSimulationCollisionLocalBase(body1, body2);
		const Mat44 &convexTransform = terrainFirst ? transform2 : transform1;
		const Mat44 &terrainTransform = terrainFirst ? transform1 : transform2;
		ReversedCollideShapeCollector reversedCollector(collector);
		CollideShapeCollector &orderedCollector = terrainFirst ? static_cast<CollideShapeCollector &>(reversedCollector) : collector;
		if(body1.GetEnhancedInternalEdgeRemovalWithBody(body2))
		{
			settings.mActiveEdgeMode = EActiveEdgeMode::CollideWithAll;
			settings.mCollectFacesMode = ECollectFacesMode::CollectFaces;
		}
		// The terrain collector emits face contacts or one fallback, so no second edge filter is needed.
		CollideConvexVsPlanetTerrain(convex, planet, Vec3::sOne(), Vec3::sOne(), convexTransform, terrainTransform, subShapeIDCreator.GetID(), subShapeIDCreator, settings, orderedCollector, localBase, Vec3::sZero());
	}

	static void sCastConvexVsPlanetTerrain(const ShapeCast &shapeCast, const ShapeCastSettings &settings, const Shape *shape, Vec3Arg scale, [[maybe_unused]] const ShapeFilter &shapeFilter, Mat44Arg planetTransform, const SubShapeIDCreator &shapeSubShapeIDCreator, const SubShapeIDCreator &planetSubShapeIDCreator, CastShapeCollector &collector)
	{
		JPH_ASSERT(shapeCast.mShape->GetType() == EShapeType::Convex);
		JPH_ASSERT(shape->GetSubType() == EShapeSubType::User1);
		const RNCustomPlanetTerrainShape *planet = static_cast<const RNCustomPlanetTerrainShape *>(shape);

		struct Visitor : public CastConvexVsTriangles
		{
			using CastConvexVsTriangles::CastConvexVsTriangles;

			bool ShouldAbort() const
			{
				return mCollector.ShouldEarlyOut();
			}
		};

		const PrecisionBase localBase = GetCastLocalBase(shapeCast, planetTransform);
		const Vec3 worldBase = planetTransform * localBase.vector;
		const Mat44 shiftedPlanetTransform = GetShiftedReferenceTransform(planetTransform);
		const ShapeCast shiftedShapeCast = shapeCast.PostTranslated(-worldBase);
		OffsetCastShapeCollector offsetCollector(collector, worldBase);

		const AABox castBounds = GetShapeCastBounds(shiftedShapeCast);
		Visitor visitor(shiftedShapeCast, settings, scale, shiftedPlanetTransform, shapeSubShapeIDCreator, offsetCollector);
		struct StreamingVisitor
		{
			StreamingVisitor(Visitor &target, const SubShapeIDCreator &creator) : visitor(target), subShapeIDCreator(creator) {}

			bool ShouldAbort() const { return visitor.ShouldAbort(); }
			void VisitTriangle(const Triangle &triangle, [[maybe_unused]] Vec3Arg normal)
			{
				visitor.Cast(triangle.vertices[0], triangle.vertices[1], triangle.vertices[2], 0, subShapeIDCreator.PushID(GetTriangleSubShapeID(triangle.id), TriangleSubShapeIDBits).GetID());
			}

			Visitor &visitor;
			const SubShapeIDCreator &subShapeIDCreator;
		};

		StreamingVisitor streamingVisitor(visitor, planetSubShapeIDCreator);
		planet->VisitTriangles(castBounds, 0.0f, localBase, streamingVisitor, MaxCastTriangles);
	}

	template<class Visitor>
	void VisitRayTriangles(const RayCast &shiftedRay, float maximumFraction, const PrecisionBase &localBase, Visitor &visitor) const
	{
		if(maximumFraction <= 0.0f) return;
		if(shiftedRay.mDirection.LengthSq() <= 0.0f) return;

		AABox rayBounds = GetRayBounds(shiftedRay, maximumFraction);
		rayBounds.ExpandBy(Vec3::sReplicate(0.01f));
		VisitTriangles(rayBounds, 0.0f, localBase, visitor, MaxRayTriangles);
	}

	bool FindRayTriangleHit(const RayCast &ray, const RayCastSettings &settings, const SubShapeIDCreator &subShapeIDCreator, RayCastResult &hit) const
	{
		const PrecisionBase localBase = MakePrecisionBase(RVec3(ray.mOrigin));
		const RayCast shiftedRay(ray.mOrigin - localBase.vector, ray.mDirection);

		bool hasHit = false;
		struct Visitor
		{
			Visitor(const RayCast &sourceRay, const RayCastSettings &sourceSettings, const SubShapeIDCreator &creator, RayCastResult &targetHit, bool &targetHasHit) :
				ray(sourceRay), settings(sourceSettings), subShapeIDCreator(creator), hit(targetHit), hasHit(targetHasHit)
			{}

			bool ShouldAbort() const { return false; }
			void VisitTriangle(const Triangle &triangle, Vec3Arg normal)
			{
				if(!ShouldRayHitTriangle(ray, settings, normal)) return;

				const float fraction = RayTriangle(ray.mOrigin, ray.mDirection, triangle.vertices[0], triangle.vertices[1], triangle.vertices[2]);
				if(fraction >= hit.mFraction) return;

				hit.mFraction = fraction;
				hit.mSubShapeID2 = subShapeIDCreator.PushID(GetTriangleSubShapeID(triangle.id), TriangleSubShapeIDBits).GetID();
				hasHit = true;
			}

			const RayCast &ray;
			const RayCastSettings &settings;
			const SubShapeIDCreator &subShapeIDCreator;
			RayCastResult &hit;
			bool &hasHit;
		};

		Visitor visitor(shiftedRay, settings, subShapeIDCreator, hit, hasHit);
		VisitRayTriangles(shiftedRay, hit.mFraction, localBase, visitor);

		return hasHit;
	}

	void CastRayTriangles(const RayCast &ray, const RayCastSettings &settings, const SubShapeIDCreator &subShapeIDCreator, CastRayCollector &collector) const
	{
		const PrecisionBase localBase = MakePrecisionBase(RVec3(ray.mOrigin));
		const RayCast shiftedRay(ray.mOrigin - localBase.vector, ray.mDirection);

		struct Visitor
		{
			Visitor(const RayCast &sourceRay, const RayCastSettings &sourceSettings, const SubShapeIDCreator &creator, CastRayCollector &targetCollector) :
				ray(sourceRay), settings(sourceSettings), subShapeIDCreator(creator), collector(targetCollector)
			{}

			bool ShouldAbort() const { return collector.ShouldEarlyOut(); }
			void VisitTriangle(const Triangle &triangle, Vec3Arg normal)
			{
				if(!ShouldRayHitTriangle(ray, settings, normal)) return;

				const float fraction = RayTriangle(ray.mOrigin, ray.mDirection, triangle.vertices[0], triangle.vertices[1], triangle.vertices[2]);
				if(fraction >= collector.GetEarlyOutFraction()) return;

				RayCastResult result;
				result.mFraction = fraction;
				result.mSubShapeID2 = subShapeIDCreator.PushID(GetTriangleSubShapeID(triangle.id), TriangleSubShapeIDBits).GetID();
				collector.AddHit(result);
			}

			const RayCast &ray;
			const RayCastSettings &settings;
			const SubShapeIDCreator &subShapeIDCreator;
			CastRayCollector &collector;
		};

		Visitor visitor(shiftedRay, settings, subShapeIDCreator, collector);
		VisitRayTriangles(shiftedRay, collector.GetEarlyOutFraction(), localBase, visitor);
	}

	bool AddSolidRecoveryContact(const ConvexShape *convex, Vec3Arg scale1, Vec3Arg scale2, Mat44Arg shiftedTransform1, Mat44Arg shiftedTransform2, const SubShapeID &subShapeID1, const SubShapeID &subShapeID2, const CollideShapeSettings &settings, CollideShapeCollector &collector, const PrecisionBase &localBase) const
	{
		if(!_provider) return false;

		const PrecisionBase localOrigin = GetLocalOrigin();
		const double referenceRadius = GetGridReferenceRadius(localOrigin, _boundsRadius);
		const uint32 collisionRevision = _provider->GetPlanetTerrainCollisionRevision();
		const uint32 cacheEpoch = _provider->GetPlanetTerrainCollisionCacheEpoch();

		SampledGridKey key;
		if(!GetClosestSampledGridVertexKey(Vec3::sZero(), localBase, localOrigin, referenceRadius, key)) return false;

		Vec3 surfacePosition;
		Vec3 surfaceNormal;
		if(!BuildSampledGridVertex(key.face, key.gridU, key.gridV, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, surfacePosition, &surfaceNormal)) return false;

		const Vec3 surfaceNormalWorld = shiftedTransform2.Multiply3x3(surfaceNormal).NormalizedOr(Vec3::sAxisY());
		const Vec3 surfacePositionWorld = shiftedTransform2 * (scale2 * surfacePosition);
		const Vec3 normalInConvexSpace = shiftedTransform1.Multiply3x3Transposed(surfaceNormalWorld).NormalizedOr(Vec3::sAxisY());
		ConvexShape::SupportBuffer supportBuffer;
		const ConvexShape::Support *support = convex->GetSupportFunction(ConvexShape::ESupportMode::Default, supportBuffer, scale1);
		const Vec3 supportPoint = support->GetSupport(-normalInConvexSpace);
		const float convexRadius = support->GetConvexRadius();
		const Vec3 supportPointWorld = shiftedTransform1 * supportPoint;
		const float signedDistance = (supportPointWorld - surfacePositionWorld).Dot(surfaceNormalWorld);
		const float penetrationDepth = -signedDistance + convexRadius;
		if(penetrationDepth <= MinimumSolidRecoverySupportDepth) return false;
		if(-penetrationDepth >= collector.GetEarlyOutFraction()) return false;

		const Vec3 point1 = supportPointWorld - surfaceNormalWorld * convexRadius;
		const Vec3 point2 = supportPointWorld - surfaceNormalWorld * signedDistance;
		CollideShapeResult result(point1, point2, -surfaceNormalWorld, penetrationDepth, subShapeID1, subShapeID2, TransformedShape::sGetBodyID(collector.GetContext()));

		if(settings.mCollectFacesMode == ECollectFacesMode::CollectFaces)
		{
			convex->GetSupportingFace(SubShapeID(), normalInConvexSpace, scale1, shiftedTransform1, result.mShape1Face);
			if(!result.mShape1Face.empty())
			{
				float faceExtent = convex->GetLocalBounds().Scaled(scale1).GetExtent().Length();
				if(faceExtent < 1.0f) faceExtent = 1.0f;
				if(faceExtent > 16.0f) faceExtent = 16.0f;
				BuildSurfaceFace(point2, surfaceNormalWorld, faceExtent, result.mShape2Face);
			}
		}

		collector.AddHit(result);
		return true;
	}

	bool Sample(Vec3Arg direction, Vec3 &position, Vec3 &normal, float &radius) const
	{
		return Sample(direction, MakePrecisionBase(RVec3::sZero()), position, normal, radius);
	}

	bool GetTriangleBySubShapeID(const SubShapeID &subShapeID, const PrecisionBase &localBase, const PrecisionBase &localOrigin, Triangle &triangle) const
	{
		if(!_provider) return false;

		const uint32 triangleID = GetTriangleSubShapeID(subShapeID);
		if(IsSampledTriangleID(triangleID))
		{
			const double referenceRadius = GetGridReferenceRadius(localOrigin, _boundsRadius);
			const uint32 collisionRevision = _provider->GetPlanetTerrainCollisionRevision();
			const uint32 cacheEpoch = _provider->GetPlanetTerrainCollisionCacheEpoch();
			SampledGridKey key;
			if(!DecodeSampledTriangleID(triangleID, localOrigin, referenceRadius, key)) return false;
			return BuildSampledGridTriangle(key, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, triangle);
		}

		return false;
	}

	bool Sample(Vec3Arg direction, const PrecisionBase &localBase, const PrecisionBase &localOrigin, Vec3 &position, Vec3 &normal, float &radius) const
	{
		if(!_provider) return false;
		const Vec3 normalizedDirection = direction.NormalizedOr(Vec3::sAxisY());
		RN::JoltCustomPlanetTerrainSample sample;
		if(!_provider->SamplePlanetTerrain(normalizedDirection.GetX(), normalizedDirection.GetY(), normalizedDirection.GetZ(), sample)) return false;

		position = GetOffsetPosition(sample.positionX, sample.positionY, sample.positionZ, localBase);
		normal = Vec3(sample.normalX, sample.normalY, sample.normalZ).NormalizedOr(normalizedDirection);
		radius = sample.radius > 0.0f ? sample.radius : static_cast<float>(GetPlanetLocalRadius(position, localBase, localOrigin));
		return true;
	}

	bool Sample(Vec3Arg direction, const PrecisionBase &localBase, Vec3 &position, Vec3 &normal, float &radius) const
	{
		return Sample(direction, localBase, GetLocalOrigin(), position, normal, radius);
	}

	float GetSignedDistance(Vec3Arg point) const
	{
		const PrecisionBase localOrigin = GetLocalOrigin();
		const PrecisionBase localBase = MakePrecisionBase(RVec3::sZero());
		const Vec3 direction = GetPlanetLocalDirection(point, localBase, localOrigin);
		Vec3 position;
		Vec3 normal;
		float radius = 0.0f;
		if(!Sample(direction, position, normal, radius)) return FLT_MAX;
		return static_cast<float>(GetPlanetLocalRadius(point, localBase, localOrigin)) - radius;
	}

	static SampledGridRowScratch &GetSampledGridRowScratch()
	{
		static thread_local SampledGridRowScratch scratch;
		return scratch;
	}

	bool BuildSampledGridVertex(uint8 face, int gridU, int gridV, const PrecisionBase &localBase, const PrecisionBase &localOrigin, double referenceRadius, uint32 collisionRevision, uint32 cacheEpoch, Vec3 &position, Vec3 *normal = nullptr) const
	{
		static thread_local SampledGridVertexCache cache;

		const uint32 cacheIndex = GetSampledGridVertexHash(collisionRevision, cacheEpoch, face, gridU, gridV) & SampledGridVertexCacheMask;
		CachedSampledGridVertex &cachedVertex = cache.entries[cacheIndex];
		if(cachedVertex.shape == this &&
			cachedVertex.revision == collisionRevision &&
			cachedVertex.cacheEpoch == cacheEpoch &&
			cachedVertex.face == face &&
			cachedVertex.gridU == gridU &&
			cachedVertex.gridV == gridV)
		{
			if(!cachedVertex.valid) return false;

			position = GetOffsetPosition(cachedVertex.absoluteX - localOrigin.x,
										 cachedVertex.absoluteY - localOrigin.y,
										 cachedVertex.absoluteZ - localOrigin.z,
										 localBase);
			if(normal) *normal = cachedVertex.normal;
			return true;
		}

		const double u = static_cast<double>(gridU) * CollisionGridCellSize / referenceRadius;
		const double v = static_cast<double>(gridV) * CollisionGridCellSize / referenceRadius;
		const Vec3 direction = GetCubeSphereDirection(face, u, v);
		RN::JoltCustomPlanetTerrainSample sample;
		const bool valid = _provider && _provider->SamplePlanetTerrain(direction.GetX(), direction.GetY(), direction.GetZ(), sample);

		cachedVertex.shape = this;
		cachedVertex.revision = collisionRevision;
		cachedVertex.cacheEpoch = cacheEpoch;
		cachedVertex.face = face;
		cachedVertex.gridU = gridU;
		cachedVertex.gridV = gridV;
		cachedVertex.valid = valid;
		if(!valid) return false;

		cachedVertex.absoluteX = sample.positionX + localOrigin.x;
		cachedVertex.absoluteY = sample.positionY + localOrigin.y;
		cachedVertex.absoluteZ = sample.positionZ + localOrigin.z;
		cachedVertex.normal = Vec3(sample.normalX, sample.normalY, sample.normalZ).NormalizedOr(direction);
		position = GetOffsetPosition(sample.positionX, sample.positionY, sample.positionZ, localBase);
		if(normal) *normal = cachedVertex.normal;
		return true;
	}

	void FillSampledGridVertexRow(uint8 face, int minU, int gridV, uint vertexCount, const PrecisionBase &localBase, const PrecisionBase &localOrigin, double referenceRadius, uint32 collisionRevision, uint32 cacheEpoch, Array<Vec3> &positions, Array<uint8> &valid) const
	{
		positions.resize(vertexCount);
		valid.resize(vertexCount);
		for(uint i = 0; i < vertexCount; i += 1)
		{
			valid[i] = BuildSampledGridVertex(face, minU + static_cast<int>(i), gridV, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, positions[i]) ? 1 : 0;
		}
	}

	bool BuildSampledGridTriangleFromVertices(const SampledGridKey &key, const Vec3 &p00, const Vec3 &p10, const Vec3 &p01, const Vec3 &p11, const PrecisionBase &localOrigin, double referenceRadius, Triangle &triangle, Vec3 &normal) const
	{
		if(key.diagonal == 0)
		{
			triangle.vertices[0] = p00;
			triangle.vertices[1] = p10;
			triangle.vertices[2] = p01;
		}
		else
		{
			triangle.vertices[0] = p10;
			triangle.vertices[1] = p11;
			triangle.vertices[2] = p01;
		}

		if(!MakeSampledTriangleID(key.face, key.diagonal, key.gridU, key.gridV, localOrigin, referenceRadius, triangle.id)) return false;
		// All cube-sphere faces use the same outward u/v winding, which positive radial terrain samples preserve.
		normal = (triangle.vertices[1] - triangle.vertices[0]).Cross(triangle.vertices[2] - triangle.vertices[0]);
		const float normalLengthSq = normal.LengthSq();
		if(normalLengthSq <= MinimumTriangleNormalLengthSq) return false;
		normal /= Sqrt(normalLengthSq);
		return true;
	}

	bool BuildSampledGridTriangle(const SampledGridKey &key, const PrecisionBase &localBase, const PrecisionBase &localOrigin, double referenceRadius, uint32 collisionRevision, uint32 cacheEpoch, Triangle &triangle) const
	{
		Vec3 p00;
		Vec3 p10;
		Vec3 p01;
		Vec3 p11;
		if(!BuildSampledGridVertex(key.face, key.gridU, key.gridV, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, p00)) return false;
		if(!BuildSampledGridVertex(key.face, key.gridU + 1, key.gridV, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, p10)) return false;
		if(!BuildSampledGridVertex(key.face, key.gridU, key.gridV + 1, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, p01)) return false;
		if(!BuildSampledGridVertex(key.face, key.gridU + 1, key.gridV + 1, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, p11)) return false;
		Vec3 normal;
		return BuildSampledGridTriangleFromVertices(key, p00, p10, p01, p11, localOrigin, referenceRadius, triangle, normal);
	}

	void IncludeSampledGridPoint(Vec3Arg point, const PrecisionBase &localBase, const PrecisionBase &localOrigin, double referenceRadius, SampledGridFaceRange ranges[6]) const
	{
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		if(!GetDirectionForPosition(point, localBase, localOrigin, x, y, z)) return;

		for(uint8 face = 0; face < 6; face += 1)
		{
			double u = 0.0;
			double v = 0.0;
			if(!GetFaceCoordinatesOnFace(x, y, z, face, u, v)) continue;

			const int gridU = GetGridCoordinate(u * referenceRadius);
			const int gridV = GetGridCoordinate(v * referenceRadius);
			IncludeSampledGridCoordinate(ranges[face], gridU, gridV);
		}
	}

	void ExpandSampledGridRangeToBlock(SampledGridFaceRange &range, uint8 face, const PrecisionBase &localOrigin, double referenceRadius) const
	{
		if(!range.valid) return;

		range.minU -= CollisionGridQueryPaddingCells;
		range.maxU += CollisionGridQueryPaddingCells;
		range.minV -= CollisionGridQueryPaddingCells;
		range.maxV += CollisionGridQueryPaddingCells;

		int originU = 0;
		int originV = 0;
		GetSampledGridBlockOrigin(face, localOrigin, referenceRadius, originU, originV);
		const int maxBlockU = originU + CollisionGridBlockCellCount - 1;
		const int maxBlockV = originV + CollisionGridBlockCellCount - 1;
		if(range.minU < originU) range.minU = originU;
		if(range.maxU > maxBlockU) range.maxU = maxBlockU;
		if(range.minV < originV) range.minV = originV;
		if(range.maxV > maxBlockV) range.maxV = maxBlockV;

		if(range.minU > range.maxU || range.minV > range.maxV) range.valid = false;
	}

	bool GetClosestSampledGridVertexKey(Vec3Arg point, const PrecisionBase &localBase, const PrecisionBase &localOrigin, double referenceRadius, SampledGridKey &key) const
	{
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		if(!GetDirectionForPosition(point, localBase, localOrigin, x, y, z)) return false;

		const uint8 face = GetDominantCubeSphereFace(x, y, z);
		double u = 0.0;
		double v = 0.0;
		if(!GetFaceCoordinatesOnFace(x, y, z, face, u, v)) return false;

		key.face = face;
		key.diagonal = 0;
		key.gridU = GetNearestGridCoordinate(u * referenceRadius);
		key.gridV = GetNearestGridCoordinate(v * referenceRadius);
		return true;
	}

	static double GetMinimumProjectedBoxRadius(const AABox &box, Vec3Arg direction, const PrecisionBase &localBase, const PrecisionBase &localOrigin)
	{
		const Vec3 center = box.GetCenter();
		const Vec3 extent = box.GetExtent();
		const double centerX = static_cast<double>(center.GetX()) + localBase.x + localOrigin.x;
		const double centerY = static_cast<double>(center.GetY()) + localBase.y + localOrigin.y;
		const double centerZ = static_cast<double>(center.GetZ()) + localBase.z + localOrigin.z;
		const double projectedCenterRadius = centerX * static_cast<double>(direction.GetX()) +
											 centerY * static_cast<double>(direction.GetY()) +
											 centerZ * static_cast<double>(direction.GetZ());
		const double projectedExtent = GetAbsoluteValue(static_cast<double>(direction.GetX())) * static_cast<double>(extent.GetX()) +
									   GetAbsoluteValue(static_cast<double>(direction.GetY())) * static_cast<double>(extent.GetY()) +
									   GetAbsoluteValue(static_cast<double>(direction.GetZ())) * static_cast<double>(extent.GetZ());
		return projectedCenterRadius - projectedExtent;
	}

	bool ShouldSkipAboveSurfaceQuery(const AABox &box, const PrecisionBase &localBase, const PrecisionBase &localOrigin, double referenceRadius, uint32 collisionRevision, uint32 cacheEpoch, float queryExpansion) const
	{
		SampledGridKey key;
		if(!GetClosestSampledGridVertexKey(box.GetCenter(), localBase, localOrigin, referenceRadius, key)) return false;

		Vec3 surfacePosition;
		if(!BuildSampledGridVertex(key.face, key.gridU, key.gridV, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, surfacePosition)) return false;

		const Vec3 direction = GetPlanetLocalDirection(box.GetCenter(), localBase, localOrigin);
		const double minimumBoxRadius = GetMinimumProjectedBoxRadius(box, direction, localBase, localOrigin);
		const double surfaceRadius = GetPlanetLocalRadius(surfacePosition, localBase, localOrigin);
		const double clearance = minimumBoxRadius - surfaceRadius;
		return clearance > static_cast<double>(queryExpansion + AboveSurfaceQueryMargin);
	}

	template<class Visitor>
	uint VisitSampledGridTriangles(const AABox &box, const PrecisionBase &localBase, const PrecisionBase &localOrigin, double referenceRadius, uint32 collisionRevision, uint32 cacheEpoch, Visitor &visitor, uint maximumTriangleCount) const
	{
		if(maximumTriangleCount == 0) return 0;

		SampledGridFaceRange ranges[6];

		IncludeSampledGridPoint(box.GetCenter(), localBase, localOrigin, referenceRadius, ranges);
		for(uint x = 0; x < 2; x += 1)
		{
			for(uint y = 0; y < 2; y += 1)
			{
				for(uint z = 0; z < 2; z += 1)
				{
					const Vec3 point(x == 0 ? box.mMin.GetX() : box.mMax.GetX(),
									 y == 0 ? box.mMin.GetY() : box.mMax.GetY(),
									 z == 0 ? box.mMin.GetZ() : box.mMax.GetZ());
					IncludeSampledGridPoint(point, localBase, localOrigin, referenceRadius, ranges);
				}
			}
		}

		uint triangleCount = 0;
		SampledGridRowScratch &scratch = GetSampledGridRowScratch();
		for(uint8 face = 0; face < 6; face += 1)
		{
			if(triangleCount >= maximumTriangleCount || visitor.ShouldAbort()) return triangleCount;

			ExpandSampledGridRangeToBlock(ranges[face], face, localOrigin, referenceRadius);
			if(!ranges[face].valid) continue;

			const uint vertexCount = static_cast<uint>(ranges[face].maxU - ranges[face].minU + 2);
			Array<Vec3> *vertexRows = scratch.positions;
			Array<uint8> *validRows = scratch.valid;
			FillSampledGridVertexRow(face, ranges[face].minU, ranges[face].minV, vertexCount, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, vertexRows[0], validRows[0]);
			FillSampledGridVertexRow(face, ranges[face].minU, ranges[face].minV + 1, vertexCount, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, vertexRows[1], validRows[1]);

			for(int gridV = ranges[face].minV; gridV <= ranges[face].maxV; gridV += 1)
			{
				if(triangleCount >= maximumTriangleCount || visitor.ShouldAbort()) return triangleCount;

				const uint lowerRowIndex = static_cast<uint>(gridV - ranges[face].minV) & 0x1u;
				const uint upperRowIndex = 1u - lowerRowIndex;
				if(gridV != ranges[face].minV)
				{
					FillSampledGridVertexRow(face, ranges[face].minU, gridV + 1, vertexCount, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, vertexRows[upperRowIndex], validRows[upperRowIndex]);
				}

				for(int gridU = ranges[face].minU; gridU <= ranges[face].maxU; gridU += 1)
				{
					const uint column = static_cast<uint>(gridU - ranges[face].minU);
					if(!validRows[lowerRowIndex][column] || !validRows[lowerRowIndex][column + 1] || !validRows[upperRowIndex][column] || !validRows[upperRowIndex][column + 1]) continue;

					const Vec3 &p00 = vertexRows[lowerRowIndex][column];
					const Vec3 &p10 = vertexRows[lowerRowIndex][column + 1];
					const Vec3 &p01 = vertexRows[upperRowIndex][column];
					const Vec3 &p11 = vertexRows[upperRowIndex][column + 1];

					for(uint8 diagonal = 0; diagonal < 2; diagonal += 1)
					{
						if(triangleCount >= maximumTriangleCount || visitor.ShouldAbort()) return triangleCount;

						SampledGridKey key;
						key.face = face;
						key.diagonal = diagonal;
						key.gridU = gridU;
						key.gridV = gridV;

						Triangle triangle;
						Vec3 normal;
						if(!BuildSampledGridTriangleFromVertices(key, p00, p10, p01, p11, localOrigin, referenceRadius, triangle, normal)) continue;
						if(!DoesTriangleOverlapBox(triangle, box)) continue;

						visitor.VisitTriangle(triangle, normal);
						triangleCount += 1;
					}
				}
			}
		}
		return triangleCount;
	}

	template<class Visitor>
	uint VisitTriangles(const AABox &box, float maxSeparationDistance, const PrecisionBase &localBase, Visitor &visitor, uint maximumTriangleCount) const
	{
		if(!_provider) return 0;

		const float radialExpansion = maxSeparationDistance > 0.0f ? maxSeparationDistance : 0.0f;
		const PrecisionBase localOrigin = GetLocalOrigin();
		const double referenceRadius = GetGridReferenceRadius(localOrigin, _boundsRadius);
		const uint32 collisionRevision = _provider->GetPlanetTerrainCollisionRevision();
		const uint32 cacheEpoch = _provider->GetPlanetTerrainCollisionCacheEpoch();
		float padding = 0.01f;
		if(maxSeparationDistance > 0.0f) padding += maxSeparationDistance;
		if(ShouldSkipAboveSurfaceQuery(box, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, radialExpansion + padding)) return 0;

		AABox queryBox = GetSurfaceQueryBox(box, localBase, localOrigin, radialExpansion);
		queryBox.ExpandBy(Vec3::sReplicate(padding));
		return VisitSampledGridTriangles(queryBox, localBase, localOrigin, referenceRadius, collisionRevision, cacheEpoch, visitor, maximumTriangleCount);
	}

	void CollectTriangles(const AABox &box, float maxSeparationDistance, Array<Triangle> &triangles, uint maximumTriangleCount) const
	{
		struct Collector
		{
			explicit Collector(Array<Triangle> &target) : triangles(target) {}

			bool ShouldAbort() const { return false; }
			void VisitTriangle(const Triangle &triangle, [[maybe_unused]] Vec3Arg normal) { triangles.push_back(triangle); }

			Array<Triangle> &triangles;
		};

		if(triangles.size() >= maximumTriangleCount) return;
		Collector collector(triangles);
		VisitTriangles(box, maxSeparationDistance, MakePrecisionBase(RVec3::sZero()), collector, maximumTriangleCount - static_cast<uint>(triangles.size()));
	}

	RN::JoltCustomPlanetTerrainInternalProvider *_provider;
	float _boundsRadius;
	bool _solidRecoveryOnly;
};

JPH_NAMESPACE_END

namespace RN
{
	namespace JoltCustomPlanetTerrainInternal
	{
		JPH::Shape *CreateShape(JoltCustomPlanetTerrainInternalProvider *provider, bool solidRecoveryOnly)
		{
			return new JPH::RNCustomPlanetTerrainShape(provider, solidRecoveryOnly);
		}

		void RegisterShape()
		{
			JPH::RNCustomPlanetTerrainShape::sRegister();
		}

		void InstallSimulationCollideBodyVsBody(JPH::PhysicsSystem *physicsSystem)
		{
			JPH::RNCustomPlanetTerrainShape::sInstallSimulationCollideBodyVsBody(physicsSystem);
		}
	}
}
