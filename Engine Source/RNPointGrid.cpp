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
	
	uint8 PointGrid::GetVoxel(uint32 x, uint32 y, uint32 z) const
	{
		if(x >= _resolutionX || y >= _resolutionY || z >= _resolutionZ)
			return 0;
		
		return _voxels[x * _resolutionY * _resolutionZ + y * _resolutionZ + z].density;
	}
	
	uint8 PointGrid::GetVoxel(const Vector3 position) const
	{
		if(position.x < 0 || position.y < 0 || position.z < 0 || position.x >= _resolutionX || position.y >= _resolutionY || position.z >= _resolutionZ)
			return 0;
		
		return _voxels[static_cast<uint32>(position.x) * _resolutionY * _resolutionZ + static_cast<uint32>(position.y) * _resolutionZ + static_cast<uint32>(position.z)].density;
	}
	
	void PointGrid::ApplyBlur(Vector3 from, Vector3 to, uint8 radius)
	{
		if(from.GetMin() < 0.0f)
			from = Vector3(0.0f);
		
		if(to.GetMax() < 0.0f)
			to = Vector3(0.0f);
		
		if(from.x >= _resolutionX)
			from.x = _resolutionX;
		if(from.y >= _resolutionY)
			from.y = _resolutionY;
		if(from.z >= _resolutionZ)
			from.z = _resolutionZ;
		
		if(to.x >= _resolutionX)
			to.x = _resolutionX;
		if(to.y >= _resolutionY)
			to.y = _resolutionY;
		if(to.z >= _resolutionZ)
			to.z = _resolutionZ;
		
		if(from == Vector3(0.0f) && to == Vector3(0.0f))
			to = Vector3(_resolutionX - 1.0f, _resolutionY - 1.0f, _resolutionZ - 1.0f);
		
		uint8 diameter = radius * 2 + 1;
		for(int x = from.x; x <= to.x; x++)
		{
			for(int y = from.y; y <= to.y; y++)
			{
				for(int z = from.z; z <= to.z; z++)
				{
					uint32 density = 0;
					
					for(int k = -radius; k <= radius; k++)
					{
						if((x + k) >= 0 && (x + k) < _resolutionX)
							density += _voxels[(x + k) * _resolutionY * _resolutionZ + y * _resolutionZ + z].density;
					}
					_voxels[x * _resolutionY * _resolutionZ + y * _resolutionZ + z].smoothDensity = density / diameter;
				}
			}
		}
		
		for(int x = from.x; x <= to.x; x++)
		{
			for(int y = from.y; y <= to.y; y++)
			{
				for(int z = from.z; z <= to.z; z++)
				{
					uint32 density = 0;
					
					for(int k = -radius; k <= radius; k++)
					{
						if((y + k) >= 0 && (y + k) < _resolutionY)
							density += _voxels[x * _resolutionY * _resolutionZ + (y + k) * _resolutionZ + z].smoothDensity;
					}
					_voxels[x * _resolutionY * _resolutionZ + y * _resolutionZ + z].smoothDensity = density / diameter;
				}
			}
		}
		
		for(int x = from.x; x <= to.x; x++)
		{
			for(int y = from.y; y <= to.y; y++)
			{
				for(int z = from.z; z <= to.z; z++)
				{
					uint32 density = 0;
					
					for(int k = -radius; k <= radius; k++)
					{
						if((z + k) >= 0 && (z + k) < _resolutionZ)
							density += _voxels[x * _resolutionY * _resolutionZ + y * _resolutionZ + z + k].smoothDensity;
					}
					_voxels[x * _resolutionY * _resolutionZ + y * _resolutionZ + z].smoothDensity = density / diameter;
				}
			}
		}
	}
	
	uint8 PointGrid::GetSmooth(Vector3 position) const
	{
		if(position.x < 0 || position.y < 0 || position.z < 0 || position.x >= _resolutionX || position.y >= _resolutionY || position.z >= _resolutionZ)
			return 0;
		
		return _voxels[static_cast<uint32>(position.x) * _resolutionY * _resolutionZ + static_cast<uint32>(position.y) * _resolutionZ + static_cast<uint32>(position.z)].smoothDensity;
	}
}
