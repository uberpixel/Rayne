//
//  RNPointGrid.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __Rayne__RNPointGrid__
#define __Rayne__RNPointGrid__

#include "RNVector.h"

namespace RN
{
	struct Point
	{
	public:
		Point(uint8 d=0) : density(d), smoothDensity(d) {}
		uint8 density;
		uint8 smoothDensity;
	};
	
	class PointGrid
	{
	public:
		PointGrid(uint32 resolutionX=64, uint32 resolutionY=32, uint32 resolutionZ=64);
		~PointGrid();
		
		void SetVoxel(uint32 x, uint32 y, uint32 z, const Point &voxel);
		uint8 GetVoxel(uint32 x, uint32 y, uint32 z) const;
		uint8 GetVoxel(const Vector3 position) const;
		uint8 GetSmooth(Vector3 position) const;
		
		void ApplyBlur(Vector3 from, Vector3 to, uint8 radius);
		
		uint32 GetResolutionX() const { return _resolutionX; }
		uint32 GetResolutionY() const { return _resolutionY; }
		uint32 GetResolutionZ() const { return _resolutionZ; }
		
	private:
		Point *_voxels;
		
		uint32 _resolutionX;
		uint32 _resolutionY;
		uint32 _resolutionZ;
	};
}

#endif /* defined(__Rayne__RNPointGrid__) */
