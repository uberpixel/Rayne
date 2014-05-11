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
		Point(uint8 d = 0) : 
			density(d), 
			smoothDensity(d) 
		{}
		
		uint8 density;
		uint8 smoothDensity;
	};
	
	class PointGrid
	{
	public:
		RNAPI PointGrid(uint32 resolutionX = 64, uint32 resolutionY = 32, uint32 resolutionZ = 64);
		RNAPI ~PointGrid();
		
		RNAPI void SetVoxel(uint32 x, uint32 y, uint32 z, const Point &voxel);
		RNAPI uint8 GetVoxel(uint32 x, uint32 y, uint32 z) const;
		RNAPI uint8 GetVoxel(const Vector3 &position) const;
		RNAPI uint8 GetSmooth(const Vector3 &position) const;
		
		RNAPI void ApplyBlur(Vector3 from, Vector3 to, uint8 radius);
		
		RNAPI uint32 GetResolutionX() const { return _resolutionX; }
		RNAPI uint32 GetResolutionY() const { return _resolutionY; }
		RNAPI uint32 GetResolutionZ() const { return _resolutionZ; }
		
		RNAPI uint32 GetID(const Vector3 &position) const;
		
	private:
		Point *_voxels;
		
		uint32 _resolutionX;
		uint32 _resolutionY;
		uint32 _resolutionZ;
	};
}

#endif /* defined(__Rayne__RNPointGrid__) */
