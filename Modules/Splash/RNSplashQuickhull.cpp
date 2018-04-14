//
//  RNSplashQuickhull.cpp
//  Rayne-Quickhull
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSplashQuickhull.h"

namespace RN
{
	SplashQuickhull::SplashQuickhull()
	{

	}

	SplashQuickhull::SplashQuickhull(const std::vector<Vector3> &vertices)
	{
		SetVertices(vertices);
	}

	void SplashQuickhull::SetVertices(const std::vector<Vector3> &vertices)
	{
		_vertices.clear();
		_indices.clear();
		_center = Vector3();

		uint32 extremeIndices[6] = { 0, 0, 0, 0, 0, 0 };

		//Find one extreme along each coordinate axis, as these are guaranteed part of the convex hull
		for(uint32 i = 0; i < vertices.size(); i++)
		{
			if(vertices[extremeIndices[0]].x < vertices[i].x)
			{
				extremeIndices[0] = i;
			}
			if(vertices[extremeIndices[1]].x > vertices[i].x)
			{
				extremeIndices[1] = i;
			}

			if(vertices[extremeIndices[2]].y < vertices[i].y)
			{
				extremeIndices[2] = i;
			}
			if(vertices[extremeIndices[3]].y > vertices[i].y)
			{
				extremeIndices[3] = i;
			}

			if(vertices[extremeIndices[4]].z < vertices[i].z)
			{
				extremeIndices[4] = i;
			}
			if(vertices[extremeIndices[5]].z > vertices[i].z)
			{
				extremeIndices[5] = i;
			}
		}

		//Find other points that are also extremes along the same axes
		std::vector<uint32> extremeIndicesList[6];
		for(uint32 i = 0; i < vertices.size(); i++)
		{
			if(fabsf(vertices[extremeIndices[0]].x - vertices[i].x) < k::EpsilonFloat)
			{
				extremeIndicesList[0].push_back(i);
			}
			if(fabsf(vertices[extremeIndices[1]].x - vertices[i].x) < k::EpsilonFloat)
			{
				extremeIndicesList[1].push_back(i);
			}

			if(fabsf(vertices[extremeIndices[2]].y - vertices[i].y) < k::EpsilonFloat)
			{
				extremeIndicesList[2].push_back(i);
			}
			if(fabsf(vertices[extremeIndices[3]].y - vertices[i].y) < k::EpsilonFloat)
			{
				extremeIndicesList[3].push_back(i);
			}

			if(fabsf(vertices[extremeIndices[4]].z - vertices[i].z) < k::EpsilonFloat)
			{
				extremeIndicesList[4].push_back(i);
			}
			if(fabsf(vertices[extremeIndices[5]].z - vertices[i].z) < k::EpsilonFloat)
			{
				extremeIndicesList[5].push_back(i);
			}
		}

		uint32 finalIndices[4] = { (extremeIndicesList[0])[0], -1, -1, -1 };
		uint8 currentIndex = 1;
		for(int i = 1; i < 6 && currentIndex < 4; i++)
		{
			for(uint32 index : extremeIndicesList[i])
			{
				bool isNew = true;
				for(int n = 0; n < currentIndex; n++)
				{
					if(index == finalIndices[n])
					{
						isNew = false;
						break;
					}
				}
				if(isNew)
				{
					finalIndices[currentIndex] = index;
					currentIndex += 1;
					break;
				}
			}
		}

		RN_ASSERT(currentIndex > 3, "Not enough extreme points found (needs at least 4 vertices building a volume)");

		//Populate _vertices array with some first known hull vertices
		_vertices.reserve(vertices.size());
		_vertices.push_back(vertices[finalIndices[0]]);
		_vertices.push_back(vertices[finalIndices[1]]);
		_vertices.push_back(vertices[finalIndices[2]]);
		_vertices.push_back(vertices[finalIndices[3]]);

		//Use indices into _vertices array (the numbers in the end)
		std::vector<HullPlane> planes;
		planes.push_back(HullPlane{ Plane::WithTriangle(_vertices[0], _vertices[2], _vertices[1]), 0, 2, 1 });
		planes.push_back(HullPlane{ Plane::WithTriangle(_vertices[0], _vertices[3], _vertices[2]), 0, 3, 2 });
		planes.push_back(HullPlane{ Plane::WithTriangle(_vertices[0], _vertices[1], _vertices[3]), 0, 1, 3 });
		planes.push_back(HullPlane{ Plane::WithTriangle(_vertices[1], _vertices[2], _vertices[3]), 1, 2, 3 });

		size_t previousIndexSet = 0;
		size_t currentIndexSet = 1;
		std::vector<uint32> tempIndices[2];
		tempIndices[0].reserve(vertices.size());
		tempIndices[1].reserve(vertices.size());

		//Populate list with index to each vertex, except the already found extreme points
		for(uint32 i= 0; i < vertices.size(); i++)
		{
			if(i == finalIndices[0] || i == finalIndices[1] || i == finalIndices[2] || i == finalIndices[3])
			{
				continue;
			}

			tempIndices[previousIndexSet].push_back(i);
		}

		//Breadth first loop over all planes, refining them while dismissing as many vertices as possible (Could stop early for hull with lower vertex count)
		size_t currentPlaneIndex = 0;
		uint32 lastAddedVertexIndex = -1;
		std::map<uint32, bool> outdatedPlaneMap;
		while(currentPlaneIndex < planes.size())
		{
			//Create list of all vertices outside of the current hull
			tempIndices[currentIndexSet].clear();
			for(uint32 i = 0; i < tempIndices[previousIndexSet].size(); i++)
			{
				bool isOutside = false;
				uint32 index = (tempIndices[previousIndexSet])[i];
				if(index == lastAddedVertexIndex)
					continue;

				for(uint32 p = 0; p < planes.size(); p++)
				{
					if(planes[p].plane.GetDistance(vertices[index]) > 0.0f)
					{
						isOutside = true;
						break;
					}
				}

				if(isOutside)
				{
					tempIndices[currentIndexSet].push_back(index);
				}
			}

			//find the vertex furthest away from current plane
			float maxDistance = 0.0f;
			uint32 bestIndex = -1;
			for(uint32 i = 0; i < tempIndices[currentIndexSet].size(); i++)
			{
				uint32 index = (tempIndices[currentIndexSet])[i];
				float distance = planes[currentPlaneIndex].plane.GetDistance(vertices[index]);
				if(distance > maxDistance)
				{
					maxDistance = distance;
					bestIndex = index;
					break;
				}
			}

			if(bestIndex != -1)
			{
				outdatedPlaneMap[currentPlaneIndex] = true;
				lastAddedVertexIndex = bestIndex;

				uint32 newIndex = _vertices.size();
				planes.push_back(HullPlane{ Plane::WithTriangle(_vertices[planes[currentPlaneIndex].indices[0]], _vertices[planes[currentPlaneIndex].indices[1]], vertices[bestIndex]), planes[currentPlaneIndex].indices[0], planes[currentPlaneIndex].indices[1], newIndex });
				planes.push_back(HullPlane{ Plane::WithTriangle(_vertices[planes[currentPlaneIndex].indices[2]], _vertices[planes[currentPlaneIndex].indices[0]], vertices[bestIndex]), planes[currentPlaneIndex].indices[2], planes[currentPlaneIndex].indices[0], newIndex });
				planes.push_back(HullPlane{ Plane::WithTriangle(_vertices[planes[currentPlaneIndex].indices[1]], _vertices[planes[currentPlaneIndex].indices[2]], vertices[bestIndex]), planes[currentPlaneIndex].indices[1], planes[currentPlaneIndex].indices[2], newIndex });
				_vertices.push_back(vertices[bestIndex]);
			}

			size_t temp = currentIndexSet;
			currentIndexSet = previousIndexSet;
			previousIndexSet = temp;

			currentPlaneIndex += 1;
		}

		//Find center (could be implemented into previous loop for more speed, but this keeps things cleaner...)
		for(const Vector3 &vertex : _vertices)
		{
			_center += vertex;
		}
		_center /= static_cast<float>(_vertices.size());

		//Generate indices (resulting in 2*n-4 triangles)
		_indices.reserve((2 * _vertices.size() - 4) * 3);
		uint32 counter = 0;
		for(auto planeIt = planes.rbegin(); planeIt != planes.rend(); ++planeIt)
		{
			if(outdatedPlaneMap.find(counter) == outdatedPlaneMap.end())
			{
				for(int i = 0; i < 3; i++)
				{
					uint32 index = planeIt->indices[i];
					_indices.push_back(index);
				}
			}

			counter += 1;
		}
	}
}
