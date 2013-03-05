//
//  RNSkeleton.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_SKELETON_H__
#define __RAYNE_SKELETON_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNMesh.h"
#include "RNMaterial.h"
#include "RNArray.h"
#include "RNQuaternion.h"

namespace RN
{
	class Skeleton : public Object
	{
	public:
		Skeleton();
		Skeleton(const std::string& path);
		
		virtual ~Skeleton();
		
		static Skeleton *WithFile(const std::string& path);
		static Skeleton *Empty();
		
	private:
		struct MeshGroup
		{
			std::string name;
			Mesh *mesh;
			Material *material;
		};
		
		void ReadSkeletonVersion1(File *file);
		void ReadBVH(File *file);
	};
}

#endif /* __RAYNE_MODEL_H__ */
