//
//  RNRecastMesh.cpp
//  Rayne-Recast
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRecastMesh.h"
#include "Recast.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "RecastDump.h"

#include "RNRecastWorld.h"

namespace RN
{
	RNDefineMeta(RecastMesh, Object)

    struct DebugDumpFile : public duFileIO
    {
    public:
        DebugDumpFile(RN::String *fileName)
        {
            _file = RN::File::WithName(fileName, RN::File::Mode::Write);
            _file->Retain();
        }
        
        ~DebugDumpFile()
        {
            _file->Release();
        }
        
        bool isWriting() const override
        {
            return true;
        }
        
        bool isReading() const override
        {
            return false;
        }
        
        bool write(const void* ptr, const size_t size) override
        {
            _file->Write(ptr, size);
        }
        
        bool read(void* ptr, const size_t size) override
        {
            _file->Read(ptr, size);
        }
        
    private:
        RN::File *_file;
    };

    RecastMesh::Configuration::Configuration() : cellSize(0.1f), cellHeight(0.1f), agentHeight(1.8f), agentRadius(0.2f), agentMaxClimb(0.4f), agentMaxSlope(45.0f), regionMinSize(2.0f), regionMergeSize(20.0f), edgeMaxLength(12.0f), edgeMaxError(0.3f), detailSampleDist(6.0f), detailSampleMaxError(1.0f), maxVertsPerPolygon(6)
    {
        float cellSize;
        float cellHeight;
        
        float agentHeight;
        float agentRadius;
        float agentMaxClimb;
        float agentMaxSlope;
        
        float regionMinSize;
        float regionMergeSize;
        
        float edgeMaxLength;
        float edgeMaxError;
        
        float detailSampleDist;
        float detailSampleMaxError;
    }

	RecastMesh::RecastMesh(Model *model, const Configuration &configuration) : _isDirty(false), _navMesh(nullptr), _navMeshQuery(nullptr)
	{
		//TODO: Fix this
		RN_ASSERT(model->GetLODStage(0)->GetCount() == 1, "Currently only one mesh per navmesh model allowed!");
        
        _configuration = configuration;
		AddMesh(model->GetLODStage(0)->GetMeshAtIndex(0));
	}
	
	RecastMesh::RecastMesh(Array *meshes, const Configuration &configuration) : _isDirty(false), _navMesh(nullptr), _navMeshQuery(nullptr)
	{
        _configuration = configuration;
		AddMesh(meshes->GetFirstObject<Mesh>());
	}
	
	RecastMesh::RecastMesh(Mesh *mesh, const Configuration &configuration) : _isDirty(false), _navMesh(nullptr), _navMeshQuery(nullptr)
	{
        _configuration = configuration;
		AddMesh(mesh);
	}
	
	RecastMesh::~RecastMesh()
	{
		if(_navMesh)
		{
			dtFreeNavMesh(_navMesh);
			_navMesh = nullptr;
		}
		
		if(_navMeshQuery)
		{
			dtFreeNavMeshQuery(_navMeshQuery);
			_navMeshQuery = nullptr;
		}
	}
	
	void RecastMesh::AddMesh(Mesh *mesh)
	{
		const Mesh::VertexAttribute *vertexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
		if (!vertexAttribute || vertexAttribute->GetType() != PrimitiveType::Vector3)
		{
			return;
		}
		
		
		RecastWorld *world = RecastWorld::GetInstance();
		rcContext *recastContext = world->GetRecastContext();
		
		const AABB &bounds = mesh->GetBoundingBox();
		const float* bmin = &bounds.minExtend.x;
		const float* bmax = &bounds.maxExtend.x;
		
		const uint32 vertexCount = mesh->GetVerticesCount();
		const uint32 indexCount = mesh->GetIndicesCount();
		
		float m_totalBuildTimeMs;
		
		
		
		//
		// Step 1. Initialize build config.
		//
		rcConfig cfg;
		memset(&cfg, 0, sizeof(cfg));
		cfg.cs = _configuration.cellSize;
		cfg.ch = _configuration.cellHeight;
		cfg.walkableSlopeAngle = _configuration.agentMaxSlope;
		cfg.walkableHeight = (int)ceilf(_configuration.agentHeight / cfg.ch);
		cfg.walkableClimb = (int)floorf(_configuration.agentMaxClimb / cfg.ch);
		cfg.walkableRadius = (int)ceilf(_configuration.agentRadius / cfg.cs);
		cfg.maxEdgeLen = (int)(_configuration.edgeMaxLength / cfg.cs);
		cfg.maxSimplificationError = _configuration.edgeMaxError;
		cfg.minRegionArea = (int)rcSqr(_configuration.regionMinSize);		// Note: area = size*size
		cfg.mergeRegionArea = (int)rcSqr(_configuration.regionMergeSize);	// Note: area = size*size
		cfg.maxVertsPerPoly = (int)_configuration.maxVertsPerPolygon;
		cfg.detailSampleDist = _configuration.detailSampleDist < 0.9f ? 0 : cfg.cs * _configuration.detailSampleDist;
		cfg.detailSampleMaxError = _configuration.detailSampleMaxError * cfg.cs;
		
/*
		_partitionType = Watershed;*/
		
		// Set the area where the navigation will be build.
		// Here the bounds of the input mesh are used, but the
		// area could be specified by an user defined box, etc.
		rcVcopy(cfg.bmin, bmin);
		rcVcopy(cfg.bmax, bmax);
		rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
		
		// Reset build times gathering.
		recastContext->resetTimers();
		
		// Start the build process.
		recastContext->startTimer(RC_TIMER_TOTAL);
		
		recastContext->log(RC_LOG_PROGRESS, "Building navigation:");
		recastContext->log(RC_LOG_PROGRESS, " - %d x %d cells", cfg.width, cfg.height);
		recastContext->log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK indices", vertexCount/1000.0f, indexCount/1000.0f);
		
		//
		// Step 2. Rasterize input polygon soup.
		//
		
		// Allocate voxel heightfield where we rasterize our input data to.
		rcHeightfield *solid = rcAllocHeightfield();
		if(!solid)
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'solid'.");
			return;
		}
		if(!rcCreateHeightfield(recastContext, *solid, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Could not create solid heightfield.");
			return;
		}
		
		// Allocate array that can hold triangle area types.
		// If you have multiple meshes you need to process, allocate
		// and array which can hold the max number of triangles you need to process.
		unsigned char *triareas = new unsigned char[indexCount/3];
		if(!triareas)
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'm_triareas' (%d).", indexCount/3);
			return;
		}
		
		// Find triangles which are walkable based on their slope and rasterize them.
		// If your input data is multiple meshes, you can transform them here, calculate
		// the are type for each of the meshes and rasterize them.
		memset(triareas, 0, vertexCount/3*sizeof(unsigned char));
		
		Mesh::Chunk chunk = mesh->GetChunk();
		Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);
		float *vertices = new float[vertexCount*3];
		for(size_t i = 0; i < vertexCount; i++)
		{
			const Vector3 &vertex = *vertexIterator;
			vertices[i*3+0] = vertex.x;
			vertices[i*3+1] = vertex.y;
			vertices[i*3+2] = vertex.z;
			
			if(i < vertexCount-1)
				vertexIterator++;
		}
		
        //This being a signed int is kinda shit, but that's what recast wants and I guess the range is big enough to not be a problem in any normal game case
        int *indices = new int[indexCount];
        if(mesh->GetAttribute(Mesh::VertexAttribute::Feature::Indices)->GetType() == PrimitiveType::Uint16)
        {
            Mesh::ElementIterator<uint16> indexIterator = chunk.GetIterator<uint16>(Mesh::VertexAttribute::Feature::Indices);
            for(size_t i = 0; i < indexCount; i++)
            {
                indices[i] = *indexIterator;
                
                if(i < indexCount-1)
                    indexIterator++;
            }
        }
        else
        {
            Mesh::ElementIterator<uint32> indexIterator = chunk.GetIterator<uint32>(Mesh::VertexAttribute::Feature::Indices);
            for(size_t i = 0; i < indexCount; i++)
            {
                indices[i] = *indexIterator;
                
                if(i < indexCount-1)
                    indexIterator++;
            }
        }
		
		rcMarkWalkableTriangles(recastContext, cfg.walkableSlopeAngle, vertices, vertexCount, indices, indexCount/3, triareas);
		if(!rcRasterizeTriangles(recastContext, vertices, vertexCount, indices, triareas, indexCount/3, *solid, cfg.walkableClimb))
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Could not rasterize triangles.");
			return;
		}
		
		delete[] vertices;
		delete[] indices;
		delete[] triareas;
		triareas = 0;
		
		//
		// Step 3. Filter walkables surfaces.
		//
		
		// Once all geoemtry is rasterized, we do initial pass of filtering to
		// remove unwanted overhangs caused by the conservative rasterization
		// as well as filter spans where the character cannot possibly stand.
		rcFilterLowHangingWalkableObstacles(recastContext, cfg.walkableClimb, *solid);
		rcFilterLedgeSpans(recastContext, cfg.walkableHeight, cfg.walkableClimb, *solid);
		rcFilterWalkableLowHeightSpans(recastContext, cfg.walkableHeight, *solid);
		
		
		//
		// Step 4. Partition walkable surface to simple regions.
		//
		
		// Compact the heightfield so that it is faster to handle from now on.
		// This will result more cache coherent data as well as the neighbours
		// between walkable cells will be calculated.
		rcCompactHeightfield *chf = rcAllocCompactHeightfield();
		if(!chf)
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'chf'.");
			return;
		}
		if(!rcBuildCompactHeightfield(recastContext, cfg.walkableHeight, cfg.walkableClimb, *solid, *chf))
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Could not build compact data.");
			return;
		}
		
		rcFreeHeightField(solid);
		solid = 0;
		
		// Erode the walkable area by agent radius.
		if(!rcErodeWalkableArea(recastContext, cfg.walkableRadius, *chf))
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Could not erode.");
			return;
		}
		
		// (Optional) Mark areas.
/*		const ConvexVolume* vols = m_geom->getConvexVolumes();
		for(int i  = 0; i < m_geom->getConvexVolumeCount(); ++i)
			rcMarkConvexPolyArea(recastContext, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);
		*/
		
		// Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
		// There are 3 martitioning methods, each with some pros and cons:
		// 1) Watershed partitioning
		//   - the classic Recast partitioning
		//   - creates the nicest tessellation
		//   - usually slowest
		//   - partitions the heightfield into nice regions without holes or overlaps
		//   - the are some corner cases where this method creates produces holes and overlaps
		//      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
		//      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
		//   * generally the best choice if you precompute the nacmesh, use this if you have large open areas
		// 2) Monotone partioning
		//   - fastest
		//   - partitions the heightfield into regions without holes and overlaps (guaranteed)
		//   - creates long thin polygons, which sometimes causes paths with detours
		//   * use this if you want fast navmesh generation
		// 3) Layer partitoining
		//   - quite fast
		//   - partitions the heighfield into non-overlapping regions
		//   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
		//   - produces better triangles than monotone partitioning
		//   - does not have the corner cases of watershed partitioning
		//   - can be slow and create a bit ugly tessellation (still better than monotone)
		//     if you have large open areas with small obstacles (not a problem if you use tiles)
		//   * good choice to use for tiled navmesh with medium and small sized tiles
		
//		if(m_partitionType == SAMPLE_PARTITION_WATERSHED)
		{
			// Prepare for region partitioning, by calculating distance field along the walkable surface.
			if(!rcBuildDistanceField(recastContext, *chf))
			{
				recastContext->log(RC_LOG_ERROR, "buildNavigation: Could not build distance field.");
				return;
			}
			
			// Partition the walkable surface into simple regions without holes.
			if(!rcBuildRegions(recastContext, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea))
			{
				recastContext->log(RC_LOG_ERROR, "buildNavigation: Could not build watershed regions.");
				return;
			}
		}
/*		else if(m_partitionType == SAMPLE_PARTITION_MONOTONE)
		{
			// Partition the walkable surface into simple regions without holes.
			// Monotone partitioning does not need distancefield.
			if(!rcBuildRegionsMonotone(recastContext, *chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea))
			{
				recastContext->log(RC_LOG_ERROR, "buildNavigation: Could not build monotone regions.");
				return;
			}
		}
		else // SAMPLE_PARTITION_LAYERS
		{
			// Partition the walkable surface into simple regions without holes.
			if(!rcBuildLayerRegions(recastContext, *chf, 0, m_cfg.minRegionArea))
			{
				recastContext->log(RC_LOG_ERROR, "buildNavigation: Could not build layer regions.");
				return;
			}
		}*/
		
		//
		// Step 5. Trace and simplify region contours.
		//
		
		// Create contours.
		rcContourSet *cset = rcAllocContourSet();
		if(!cset)
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'cset'.");
			return;
		}
		if(!rcBuildContours(recastContext, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset))
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Could not create contours.");
			return;
		}
		
		//
		// Step 6. Build polygons mesh from contours.
		//
		
		// Build polygon navmesh from the contours.
		_polyMesh = rcAllocPolyMesh();
		if(!_polyMesh)
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmesh'.");
			return;
		}
		if(!rcBuildPolyMesh(recastContext, *cset, cfg.maxVertsPerPoly, *_polyMesh))
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Could not triangulate contours.");
			return;
		}
		
		//
		// Step 7. Create detail mesh which allows to access approximate height on each polygon.
		//
		
		_detailMesh = rcAllocPolyMeshDetail();
		if(!_detailMesh)
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Out of memory 'pmdtl'.");
			return;
		}
		
		if(!rcBuildPolyMeshDetail(recastContext, *_polyMesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *_detailMesh))
		{
			recastContext->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
			return;
		}
		
		rcFreeCompactHeightfield(chf);
		chf = 0;
		rcFreeContourSet(cset);
		cset = 0;
		
		recastContext->stopTimer(RC_TIMER_TOTAL);
		
		// Show performance stats.
//		duLogBuildTimes(*recastContext, recastContext->getAccumulatedTime(RC_TIMER_TOTAL));
		recastContext->log(RC_LOG_PROGRESS, ">> Polymesh: %d vertices  %d polygons", _polyMesh->nverts, _polyMesh->npolys);
		
		m_totalBuildTimeMs = recastContext->getAccumulatedTime(RC_TIMER_TOTAL)/1000.0f;
		
		_isDirty = true;
	}
	
	dtNavMesh *RecastMesh::GetDetourNavMesh()
	{
		if(!_isDirty)
		{
			return _navMesh;
		}
		
		if(_navMesh)
		{
			dtFreeNavMesh(_navMesh);
			_navMesh = nullptr;
		}
		
		// At this point the navigation mesh data is ready, you can access it from m_pmesh.
		// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.
		
		//
		// (Optional) Step 8. Create Detour data from Recast poly mesh.
		//
		
		// The GUI may allow more max points per polygon than Detour can handle.
		// Only build the detour navmesh if we do not exceed the limit.
		unsigned char* navData = 0;
		int navDataSize = 0;
		
		// Update poly flags from areas.
		for(int i = 0; i < _polyMesh->npolys; ++i)
		{
			_polyMesh->areas[i] = 0;
			_polyMesh->flags[i] = 1;
		}
/*		 if(_polyMesh->areas[i] == RC_WALKABLE_AREA)
		 _polyMesh->areas[i] = SAMPLE_POLYAREA_GROUND;
		 
		 if(_polyMesh->areas[i] == SAMPLE_POLYAREA_GROUND ||
		 _polyMesh->areas[i] == SAMPLE_POLYAREA_GRASS ||
		 _polyMesh->areas[i] == SAMPLE_POLYAREA_ROAD)
		 {
		 _polyMesh->flags[i] = SAMPLE_POLYFLAGS_WALK;
		 }
		 else if(_polyMesh->areas[i] == SAMPLE_POLYAREA_WATER)
		 {
		 _polyMesh->flags[i] = SAMPLE_POLYFLAGS_SWIM;
		 }
		 else if(_polyMesh->areas[i] == SAMPLE_POLYAREA_DOOR)
		 {
		 _polyMesh->flags[i] = SAMPLE_POLYFLAGS_WALK | SAMPLE_POLYFLAGS_DOOR;
		 }
		 }*/
		
		
		dtNavMeshCreateParams params;
		memset(&params, 0, sizeof(params));
		params.verts = _polyMesh->verts;
		params.vertCount = _polyMesh->nverts;
		params.polys = _polyMesh->polys;
		params.polyAreas = _polyMesh->areas;
		params.polyFlags = _polyMesh->flags;
		params.polyCount = _polyMesh->npolys;
		params.nvp = _polyMesh->nvp;
		params.detailMeshes = _detailMesh->meshes;
		params.detailVerts = _detailMesh->verts;
		params.detailVertsCount = _detailMesh->nverts;
		params.detailTris = _detailMesh->tris;
		params.detailTriCount = _detailMesh->ntris;
		//			params.offMeshConVerts = m_geom->getOffMeshConnectionVerts();
		//			params.offMeshConRad = m_geom->getOffMeshConnectionRads();
		//			params.offMeshConDir = m_geom->getOffMeshConnectionDirs();
		//			params.offMeshConAreas = m_geom->getOffMeshConnectionAreas();
		//			params.offMeshConFlags = m_geom->getOffMeshConnectionFlags();
		//			params.offMeshConUserID = m_geom->getOffMeshConnectionId();
		//			params.offMeshConCount = m_geom->getOffMeshConnectionCount();
		params.walkableHeight = _configuration.agentHeight;
		params.walkableRadius = _configuration.agentRadius;
		params.walkableClimb = _configuration.agentMaxClimb;
		rcVcopy(params.bmin, _polyMesh->bmin);
		rcVcopy(params.bmax, _polyMesh->bmax);
		params.cs = _configuration.cellSize;
		params.ch = _configuration.cellHeight;
		params.buildBvTree = true;
		
		if(!dtCreateNavMeshData(&params, &navData, &navDataSize))
		{
			RNDebug("Could not build Detour navmesh.");
			return nullptr;
		}
		
		_navMesh = dtAllocNavMesh();
		if(!_navMesh)
		{
			dtFree(navData);
			RNDebug("Could not create Detour navmesh.");
			return nullptr;
		}
		
		dtStatus status = _navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
		if(dtStatusFailed(status))
		{
			dtFree(navData);
			RNDebug("Could not init Detour navmesh.");
			return nullptr;
		}
		
		_isDirty = false;
		return _navMesh;
	}
	
	dtNavMeshQuery *RecastMesh::GetDetourQuery()
	{
		if(!_isDirty)
			return _navMeshQuery;
		
		if(!_navMeshQuery)
			_navMeshQuery = dtAllocNavMeshQuery();
		
		if(!_navMeshQuery)
		{
			RNDebug("Could not create Detour navmesh query.");
			return nullptr;
		}
		
		dtStatus status = _navMeshQuery->init(GetDetourNavMesh(), 2048);
		if(dtStatusFailed(status))
		{
			RNDebug("Could not init Detour navmesh query");
			return nullptr;
		}
		
		return _navMeshQuery;
	}

    void RecastMesh::WritePolymeshToOBJFile(RN::String *fileName)
    {
        RN_ASSERT(_polyMesh, "No mesh to write!");
        
        DebugDumpFile file(fileName);
        duDumpPolyMeshToObj(*_polyMesh, &file);
    }

    void RecastMesh::WriteDetailMeshToOBJFile(RN::String *fileName)
    {
        RN_ASSERT(_detailMesh, "No mesh to write!");
        
        DebugDumpFile file(fileName);
        duDumpPolyMeshDetailToObj(*_detailMesh, &file);
    }
	
	RecastMesh *RecastMesh::WithModel(Model *model, const Configuration &configuration)
	{
		RecastMesh *mesh = new RecastMesh(model, configuration);
		return mesh->Autorelease();
	}
}
