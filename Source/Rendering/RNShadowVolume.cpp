//
//  RNShadowVolume.cpp
//  Rayne
//
//  Copyright 2010 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNShadowVolume.h"
#include "RNModel.h"

namespace RN
{
	RNDefineMeta(ShadowVolume, Asset)
	
	bool ShadowVolumeEdge::operator== (const ShadowVolumeEdge &other)
	{
		if(position[0].GetSquaredDistance(other.position[0]) < 0.0001 && position[1].GetSquaredDistance(other.position[1]) < 0.0001)
		{
			lastaddor = false;
			return true;
		}

		if(position[0].GetSquaredDistance(other.position[1]) < 0.0001 && position[1].GetSquaredDistance(other.position[0]) < 0.0001)
		{
			lastaddor = true;
			return true;
		}

		return false;
	}

	ShadowVolume::ShadowVolume() : _faceCount(0), _edgeCount(0), _faces(nullptr), _edges(nullptr)
	{
		
	}
	
	void ShadowVolume::SetModel(Model *model, unsigned int lod)
	{
		Model::LODStage *lodStage = model->GetLODStage(lod);

		//Get vertex and face count
		unsigned int indexnum = 0;
		for(int i = 0; i < lodStage->GetCount(); i++)
		{
			indexnum += lodStage->GetMeshAtIndex(i)->GetIndicesCount();
		}

		_faceCount = indexnum/3;
		_faces = new ShadowVolumeFace[_faceCount];

		//Create array of unique edges and initialize the faces
		std::vector<ShadowVolumeEdge> edgesvec;
		ShadowVolumeEdge tempedge1;
		ShadowVolumeEdge tempedge2;
		ShadowVolumeEdge tempedge3;
		unsigned int edgeincl1;
		unsigned int edgeincl2;
		unsigned int edgeincl3;
		unsigned int realn = 0;
		for(int i = 0; i < lodStage->GetCount(); i++)
		{
			Mesh *mesh = lodStage->GetMeshAtIndex(i);
			const Mesh::VertexAttribute *vertexAttribute = mesh->GetAttribute(Mesh::VertexAttribute::Feature::Vertices);
			if(!vertexAttribute || vertexAttribute->GetType() != PrimitiveType::Vector3)
			{
				return;
			}
			
			Mesh::Chunk chunk = mesh->GetTrianglesChunk();
			Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);
			
			for(size_t i = 0; i < mesh->GetIndicesCount() / 3; i++)
			{
				if(i > 0) vertexIterator++;
				const Vector3 &vertex1 = *(vertexIterator++);
				const Vector3 &vertex2 = *(vertexIterator++);
				const Vector3 &vertex3 = *(vertexIterator);
				
				tempedge1.position[0] = vertex1;
				tempedge1.position[1] = vertex2;
				tempedge1.counter[0] = 1;
				tempedge1.counter[1] = 0;
				
				tempedge2.position[0] = vertex2;
				tempedge2.position[1] = vertex3;
				tempedge2.counter[0] = 1;
				tempedge2.counter[1] = 0;
				
				tempedge3.position[0] = vertex3;
				tempedge3.position[1] = vertex1;
				tempedge3.counter[0] = 1;
				tempedge3.counter[1] = 0;
				
				CalculateFaceNormal(&_faces[realn], vertex1, vertex3, vertex2);
				_faces[realn].flipped[0] = false;
				_faces[realn].flipped[1] = false;
				_faces[realn].flipped[2] = false;
				
				edgeincl1 = -1;
				edgeincl2 = -1;
				edgeincl3 = -1;
				
				for(int x = 0; x < edgesvec.size(); x++)
				{
					if(edgesvec[x] == tempedge1)
					{
						edgeincl1 = x;
						_faces[realn].flipped[0] = edgesvec[x].lastaddor;
						if(edgesvec[x].lastaddor)
							edgesvec[x].counter[1] += 1;
						else
							edgesvec[x].counter[0] += 1;
						
						break;
					}
				}
				for(int x = 0; x < edgesvec.size(); x++)
				{
					if(edgesvec[x] == tempedge2)
					{
						edgeincl2 = x;
						_faces[realn].flipped[1] = edgesvec[x].lastaddor;
						if(edgesvec[x].lastaddor)
							edgesvec[x].counter[1] += 1;
						else
							edgesvec[x].counter[0] += 1;
						break;
					}
				}
				for(int x = 0; x < edgesvec.size(); x++)
				{
					if(edgesvec[x] == tempedge3)
					{
						edgeincl3 = x;
						_faces[realn].flipped[2] = edgesvec[x].lastaddor;
						if(edgesvec[x].lastaddor)
							edgesvec[x].counter[1] += 1;
						else
							edgesvec[x].counter[0] += 1;
						break;
					}
				}
				
				if(edgeincl1 == -1)
				{
					edgesvec.push_back(tempedge1);
					_faces[realn].edge[0] = edgesvec.size()-1;
				}else
				{
					_faces[realn].edge[0] = edgeincl1;
				}
				if(edgeincl2 == -1)
				{
					edgesvec.push_back(tempedge2);
					_faces[realn].edge[1] = edgesvec.size()-1;
				}else
				{
					_faces[realn].edge[1] = edgeincl2;
				}
				if(edgeincl3 == -1)
				{
					edgesvec.push_back(tempedge3);
					_faces[realn].edge[2] = edgesvec.size()-1;
				}else
				{
					_faces[realn].edge[2] = edgeincl3;
				}
				
				_faces[realn].position = (vertex1 + vertex2 + vertex3) / 3.0f;
				
				realn += 1;
			}
		}

		_edges = new ShadowVolumeEdge[edgesvec.size()];
		memcpy(_edges, &edgesvec[0], edgesvec.size()*sizeof(ShadowVolumeEdge));
		_edgeCount = edgesvec.size();
	}

	ShadowVolume::~ShadowVolume()
	{
		if(_faces) delete[] _faces;
		if(_edges) delete[] _edges;
	}
	
	Mesh *ShadowVolume::GenerateMesh() const
	{
		Mesh *mesh = new Mesh({ Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::Vertices, PrimitiveType::Vector3),
			Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::Normals, PrimitiveType::Vector3),
			Mesh::VertexAttribute(Mesh::VertexAttribute::Feature::Indices, PrimitiveType::Uint32) }, _edgeCount*4, _edgeCount*6);
		
		return mesh->Autorelease();
	}

	void ShadowVolume::UpdateMesh(SceneNode *node, Mesh *mesh, Light *light)
	{
		uint32 *edgeCounter[2];
		edgeCounter[0] = new uint32[_edgeCount];
		edgeCounter[1] = new uint32[_edgeCount];
		for(uint32 edgeIndex = 0; edgeIndex < _edgeCount; edgeIndex++)
		{
			edgeCounter[0][edgeIndex] = _edges[edgeIndex].counter[0];
			edgeCounter[1][edgeIndex] = _edges[edgeIndex].counter[1];
		}
		
		Vector3 lightPosition = light->GetWorldPosition();
		Vector3 lightDirection = light->GetForward();
		lightDirection.Normalize();
		Vector3 normal;
		
		//Find silhouette
		for(unsigned int n = 0; n < _faceCount; n++)
		{
			normal = /*object->matnormal * */_faces[n].normal;
			if(light->GetType() != Light::Type::DirectionalLight)
			{
				lightDirection = lightPosition - _faces[n].position;
			}
			
			if(normal.GetDotProduct(lightDirection) <= 0.0)
			{
				if(_faces[n].flipped[0])
					edgeCounter[_faces[n].edge[0]][1] -= 1;
				else
					edgeCounter[_faces[n].edge[0]][0] -= 1;
				
				if(_faces[n].flipped[1])
					edgeCounter[_faces[n].edge[1]][1] -= 1;
				else
					edgeCounter[_faces[n].edge[1]][0] -= 1;
				
				if(_faces[n].flipped[2])
					edgeCounter[_faces[n].edge[2]][1] -= 1;
				else
					edgeCounter[_faces[n].edge[2]][0] -= 1;
			}
		}

		unsigned int finalEdgeCount = 0;
		for(int i = 0; i < _edgeCount; i++)
		{
			if(edgeCounter[i][0] + edgeCounter[i][1] == 1)
			{
				finalEdgeCount += 1;
			}
		}

		//Update mesh
		mesh->BeginChanges();
		Mesh::Chunk chunk = mesh->GetChunk();
		
		Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Vertices);
		Mesh::ElementIterator<Vector3> normalsIterator = chunk.GetIterator<Vector3>(Mesh::VertexAttribute::Feature::Normals);
		Mesh::ElementIterator<RN::uint32> indexIterator = chunk.GetIterator<RN::uint32>(Mesh::VertexAttribute::Feature::Indices);
		
		uint32 indexCounter = 0;
		for(int i = 0; i < finalEdgeCount; i++)
		{
			if(edgeCounter[i][0] + edgeCounter[i][1] == 1)
			{
				if(edgeCounter[i][0] == 1)
				{
					*vertexIterator++ = _edges[i].position[0];
					*vertexIterator++ = _edges[i].position[1];
					*vertexIterator++ = _edges[i].position[0];
					*vertexIterator++ = _edges[i].position[1];
					
					*normalsIterator++ = Vector3();
					*normalsIterator++ = Vector3();
					
					if(light->GetType() == Light::Type::DirectionalLight)
					{
						*normalsIterator++ = -lightDirection;
						*normalsIterator++ = -lightDirection;
					}
					else
					{
						*normalsIterator++ = _edges[i].position[0] - lightPosition;
						*normalsIterator++ = _edges[i].position[1] - lightPosition;
					}
				}else
				{
					*vertexIterator++ = _edges[i].position[1];
					*vertexIterator++ = _edges[i].position[0];
					*vertexIterator++ = _edges[i].position[1];
					*vertexIterator++ = _edges[i].position[0];
					
					*normalsIterator++ = Vector3();
					*normalsIterator++ = Vector3();
					
					if(light->GetType() == Light::Type::DirectionalLight)
					{
						*normalsIterator++ = -lightDirection;
						*normalsIterator++ = -lightDirection;
					}
					else
					{
						*normalsIterator++ = _edges[i].position[1] - lightPosition;
						*normalsIterator++ = _edges[i].position[0] - lightPosition;
					}
				}
				
				*indexIterator++ = indexCounter * 4 + 0;
				*indexIterator++ = indexCounter * 4 + 1;
				*indexIterator++ = indexCounter * 4 + 2;
				*indexIterator++ = indexCounter * 4 + 2;
				*indexIterator++ = indexCounter * 4 + 1;
				*indexIterator++ = indexCounter * 4 + 3;
				
				indexCounter += 1;
			}
		}
		
		for(int i = finalEdgeCount; i < _edgeCount; i++)
		{
			*indexIterator++ = 0;
			*indexIterator++ = 0;
			*indexIterator++ = 0;
			*indexIterator++ = 0;
			*indexIterator++ = 0;
			*indexIterator++ = 0;
		}
		
		//TODO:Make this less ugly... these variables should get set when changing things with the iterator or something
		mesh->changedVertices = true;
		mesh->changedIndices = true;
		mesh->EndChanges();
	}

	void ShadowVolume::CalculateFaceNormal(ShadowVolumeFace *face, const Vector3 &position1, const Vector3 &position2, const Vector3 &position3)
	{
		float a[3];
		float b[3];
		float normal[3];

		a[0] = position1.x - position2.x;
		a[1] = position1.y - position2.y;
		a[2] = position1.z - position2.z;

		b[0] = position2.x - position3.x;
		b[1] = position2.y - position3.y;
		b[2] = position2.z - position3.z;

		normal[0] = (a[1] * b[2]) - (a[2] * b[1]);
		normal[1] = (a[2] * b[0]) - (a[0] * b[2]);
		normal[2] = (a[0] * b[1]) - (a[1] * b[0]);

		float len = normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2];
		if(len == 0.0f) len = 1.0;
		len = Math::Sqrt(len);
		normal[0] /= -len;
		normal[1] /= -len;
		normal[2] /= -len;

		face->normal.x = normal[0];
		face->normal.y = normal[1];
		face->normal.z = normal[2];
	}
}

