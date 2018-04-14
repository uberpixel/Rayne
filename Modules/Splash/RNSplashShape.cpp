//
//  RNSplashShape.cpp
//  Rayne-Splash
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNSplashShape.h"
#include "RNSplashQuickhull.h"

namespace RN
{
	RNDefineMeta(SplashShape, Object)
//	RNDefineMeta(SplashSphereShape, SplashShape)
//	RNDefineMeta(SplashBoxShape, SplashShape)
	RNDefineMeta(SplashConvexHullShape, SplashShape)
	RNDefineMeta(SplashCompoundShape, SplashShape)
		
	SplashShape::SplashShape()
	{
		
	}
		
	SplashShape::~SplashShape()
	{

	}
		
/*	SplashSphereShape::SplashSphereShape(float radius)
	{
		
	}
		
	SplashSphereShape *SplashSphereShape::WithRadius(float radius)
	{
		SplashSphereShape *shape = new SplashSphereShape(radius);
		return shape->Autorelease();
	}
		
		
	SplashBoxShape::SplashBoxShape(const Vector3 &halfExtents)
	{

	}
		
	SplashBoxShape *SplashBoxShape::WithHalfExtents(const Vector3 &halfExtents)
	{
		SplashBoxShape *shape = new SplashBoxShape(halfExtents);
		return shape->Autorelease();
	}*/


	SplashConvexHullShape::SplashConvexHullShape(Mesh *mesh)
	{
		const Mesh::VertexAttribute *vertexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
		if (!vertexAttribute || vertexAttribute->GetType() != PrimitiveType::Vector3)
		{
			return;
		}

		Mesh::Chunk chunk = mesh->GetChunk();
		Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);

		std::vector<Vector3> vertices;
		vertices.reserve(mesh->GetVerticesCount());
		for(size_t i = 0; i < mesh->GetVerticesCount(); i++)
		{
			if(i > 0)
			{
				vertexIterator++;
			}
			const Vector3 &vertex = *vertexIterator;

			bool skip = false;
			for(const Vector3 &other : vertices)
			{
				if(vertex.GetDistance(other) < k::EpsilonFloat*5.0f)
				{
					skip = true;
				}
			}

			if(!skip)
			{
				vertices.push_back(vertex);
			}
		}

		_hull.SetVertices(vertices);
	}

	SplashConvexHullShape *SplashConvexHullShape::WithMesh(Mesh *mesh)
	{
		SplashConvexHullShape *shape = new SplashConvexHullShape(mesh);
		return shape->Autorelease();
	}


	
	SplashCompoundShape::SplashCompoundShape()
	{
		
	}

	SplashCompoundShape::SplashCompoundShape(Model *model)
	{
		Model::LODStage *lodStage = model->GetLODStage(0);
		size_t meshes = lodStage->GetCount();
		for (size_t i = 0; i < meshes; i++)
		{
			Mesh *mesh = lodStage->GetMeshAtIndex(i);
			SplashConvexHullShape *shape = SplashConvexHullShape::WithMesh(mesh);
			AddChild(shape, Vector3(), Quaternion());
		}
	}

	SplashCompoundShape::SplashCompoundShape(const Array *meshes)
	{
		meshes->Enumerate<Mesh>([&](Mesh *mesh, size_t index, bool &stop) {
			SplashConvexHullShape *shape = SplashConvexHullShape::WithMesh(mesh);
			AddChild(shape, Vector3(), Quaternion());
		});
	}
		
	SplashCompoundShape::~SplashCompoundShape()
	{
		for(auto shape : _shapes)
		{
			shape->Release();
		}
	}
		
	void SplashCompoundShape::AddChild(SplashShape *shape, const RN::Vector3 &position, const RN::Quaternion &rotation)
	{
		_shapes.push_back(shape->Retain());
	}

	SplashCompoundShape *SplashCompoundShape::WithModel(Model *model)
	{
		SplashCompoundShape *shape = new SplashCompoundShape(model);
		return shape->Autorelease();
	}
}
