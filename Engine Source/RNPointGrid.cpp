//
//  RNPointGrid.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPointGrid.h"

namespace RN
{
	PointGrid::PointGrid(uint32 resolutionX, uint32 resolutionY, uint32 resolutionZ) :
	_resolutionX(resolutionX),
	_resolutionY(resolutionY),
	_resolutionZ(resolutionZ)
	{
		_voxels = new Point[_resolutionX * _resolutionY * _resolutionZ];
	}
	
	PointGrid::~PointGrid()
	{
		delete[] _voxels;
	}
	
	void PointGrid::SetVoxel(uint32 x, uint32 y, uint32 z, const Point &voxel)
	{
		if(x >= _resolutionX || y >= _resolutionY || z >= _resolutionZ)
			return;
		
		_voxels[x * _resolutionY * _resolutionZ + y * _resolutionZ + z] = voxel;
	}
	
	const Point PointGrid::GetVoxel(uint32 x, uint32 y, uint32 z) const
	{
		if(x >= _resolutionX || y >= _resolutionY || z >= _resolutionZ)
			return Point();
		
		return _voxels[x * _resolutionY * _resolutionZ + y * _resolutionZ + z];
	}
}
