//
//  RNRecastMesh.h
//  Rayne-Recast
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RECASTMESH_H_
#define __RAYNE_RECASTMESH_H_

#include "RNRecast.h"

class rcPolyMesh;
class rcPolyMeshDetail;
class dtNavMesh;
class dtNavMeshQuery;

namespace RN
{
	class RecastMesh : public Object
	{
	public:
        class Configuration
        {
        public:
            Configuration();
            
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
            
            int maxVertsPerPolygon;
        };
        
		RCAPI RecastMesh(Model *model, const Configuration &configuration);
		RCAPI RecastMesh(Array *meshes, const Configuration &configuration);
		RCAPI RecastMesh(Mesh *mesh, const Configuration &configuration);
		RCAPI ~RecastMesh();
		
		RCAPI void AddMesh(Mesh *mesh);
		
		RCAPI dtNavMesh *GetDetourNavMesh();
		RCAPI dtNavMeshQuery *GetDetourQuery();
        
        RCAPI void WritePolymeshToOBJFile(RN::String *fileName);
        RCAPI void WriteDetailMeshToOBJFile(RN::String *fileName);
		
		RCAPI static RecastMesh *WithModel(Model *model, const Configuration &configuration = Configuration());
	private:
		bool _isDirty;
        Configuration _configuration;
		
		rcPolyMeshDetail *_detailMesh;
		rcPolyMesh *_polyMesh;
		dtNavMesh *_navMesh;
		dtNavMeshQuery *_navMeshQuery;
		
		RNDeclareMetaAPI(RecastMesh, RCAPI)
	};
}

#endif /* defined(__RAYNE_RECASTMESH_H_) */
