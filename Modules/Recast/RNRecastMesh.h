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
		RCAPI RecastMesh(Model *model);
		RCAPI RecastMesh(Array *meshes);
		RCAPI RecastMesh(Mesh *mesh);
		RCAPI ~RecastMesh();
		
		RCAPI void AddMesh(Mesh *mesh);
		
		RCAPI dtNavMesh *GetDetourNavMesh();
		RCAPI dtNavMeshQuery *GetDetourQuery();
		
		RCAPI static RecastMesh *WithModel(Model *model);
	private:
		bool _isDirty;
		
		rcPolyMeshDetail *_detailMesh;
		rcPolyMesh *_polyMesh;
		dtNavMesh *_navMesh;
		dtNavMeshQuery *_navMeshQuery;
		
		RNDeclareMetaAPI(RecastMesh, RCAPI)
	};
}

#endif /* defined(__RAYNE_RECASTMESH_H_) */
