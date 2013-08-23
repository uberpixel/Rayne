//
//  RBShape.cpp
//  rayne-bullet
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#include "RBShape.h"

namespace RN
{
	namespace bullet
	{
		RNDeclareMeta(Shape)
		RNDeclareMeta(SphereShape)
		RNDeclareMeta(MultiSphereShape)
		RNDeclareMeta(BoxShape)
		RNDeclareMeta(CylinderShape)
		RNDeclareMeta(CapsuleShape)
		RNDeclareMeta(StaticPlaneShape)
		RNDeclareMeta(TriangelMeshShape)
		
		Shape::Shape()
		{
			_shape = 0;
		}
		
		Shape::Shape(btCollisionShape *shape)
		{
			_shape = shape;
		}
		
		Shape::~Shape()
		{
			delete _shape;
		}
		
		
		Vector3 Shape::CalculateLocalInertia(float mass)
		{
			btVector3 inertia;
			_shape->calculateLocalInertia(mass, inertia);
			
			return Vector3(inertia.x(), inertia.y(), inertia.z());
		}
		
		void Shape::SetScale(const Vector3& scale)
		{
			_shape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
		}
		
		
		SphereShape::SphereShape(float radius)
		{
			_shape = new btSphereShape(radius);
		}
		
		SphereShape *SphereShape::WithRadius(float radius)
		{
			SphereShape *shape = new SphereShape(radius);
			return shape->Autorelease();
		}
		
		
		MultiSphereShape::MultiSphereShape(const Vector3 *positions, float *radii, int count)
		{
			btVector3 *btPositions = new btVector3[count];
			for(int i=0; i<count; i++)
			{
				btPositions[i] = btVector3(positions[i].x, positions[i].y, positions[i].z);
			}
			
			_shape = new btMultiSphereShape(btPositions, radii, count);
			delete [] btPositions;
		}
		
		MultiSphereShape *MultiSphereShape::WithHeight(float height, float width)
		{
			Vector3 positions[2] = { Vector3(0.0, height * 0.5f - width, 0.0f), Vector3(0.0f, -height * 0.5f + width, 0.0f) };
			float radii[2] = { width, width };
			
			MultiSphereShape *shape = new MultiSphereShape(positions, radii, 2);
			return shape->Autorelease();
		}
		
		
		BoxShape::BoxShape(const Vector3& halfExtents)
		{
			_shape = new btBoxShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
		}
		
		BoxShape *BoxShape::WithHalfExtents(const Vector3& halfExtents)
		{
			BoxShape *shape = new BoxShape(halfExtents);
			return shape->Autorelease();
		}
		
		
		CylinderShape::CylinderShape(const Vector3& halfExtents)
		{
			_shape = new btCylinderShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
		}
		
		CylinderShape *CylinderShape::WithHalfExtents(const Vector3& halfExtents)
		{
			CylinderShape *shape = new CylinderShape(halfExtents);
			return shape->Autorelease();
		}
		
		
		CapsuleShape::CapsuleShape(float radius, float height)
		{
			_shape = new btCapsuleShape(radius, height);
		}
		
		CapsuleShape *CapsuleShape::WithRadius(float radius, float height)
		{
			CapsuleShape *shape = new CapsuleShape(radius, height);
			return shape->Autorelease();
		}
		
		
		StaticPlaneShape::StaticPlaneShape(const Vector3& normal, float constant)
		{
			_shape = new btStaticPlaneShape(btVector3(normal.x, normal.y, normal.z), constant);
		}
		
		StaticPlaneShape *StaticPlaneShape::WithNormal(const Vector3& normal, float constant)
		{
			StaticPlaneShape *shape = new StaticPlaneShape(normal, constant);
			return shape->Autorelease();
		}
		
		
		TriangelMeshShape::TriangelMeshShape(Model *model)
		{
			_triangleMesh = new btTriangleMesh();
			
			uint32 meshes = model->GetMeshCount(0);
			for(uint32 i=0; i<meshes; i++)
			{
				Mesh *mesh = model->GetMeshAtIndex(0, i);
				AddMesh(mesh);
			}
			
			_shape = new btBvhTriangleMeshShape(_triangleMesh, true);
		}
		
		TriangelMeshShape::TriangelMeshShape(Mesh *mesh)
		{
			_triangleMesh = new btTriangleMesh();
			
			AddMesh(mesh);
			
			_shape = new btBvhTriangleMeshShape(_triangleMesh, true);
		}
		
		TriangelMeshShape::TriangelMeshShape(const Array& meshes)
		{
			_triangleMesh = new btTriangleMesh();
			
			uint32 count = static_cast<uint32>(meshes.GetCount());
			for(uint32 i=0; i<count; i++)
			{
				Mesh *mesh = meshes.GetObjectAtIndex<Mesh>(i);
				AddMesh(mesh);
			}
			
			_shape = new btBvhTriangleMeshShape(_triangleMesh, true);
		}
		
		TriangelMeshShape::~TriangelMeshShape()
		{
			delete _triangleMesh;
		}
		
		void TriangelMeshShape::AddMesh(Mesh *mesh)
		{
			MeshDescriptor *iDescriptor = mesh->GetDescriptor(kMeshFeatureIndices);
			
			for(size_t i=0; i<iDescriptor->elementCount; i+=3)
			{
				Vector3 *vertex0 = mesh->GetElement<Vector3>(kMeshFeatureVertices, *mesh->GetElement<uint16>(kMeshFeatureIndices, i + 0));
				Vector3 *vertex1 = mesh->GetElement<Vector3>(kMeshFeatureVertices, *mesh->GetElement<uint16>(kMeshFeatureIndices, i + 1));
				Vector3 *vertex2 = mesh->GetElement<Vector3>(kMeshFeatureVertices, *mesh->GetElement<uint16>(kMeshFeatureIndices, i + 2));
				
				_triangleMesh->addTriangle(btVector3(vertex0->x, vertex0->y, vertex0->z), btVector3(vertex1->x, vertex1->y, vertex1->z), btVector3(vertex2->x, vertex2->y, vertex2->z));
			}
		}
	}
}
