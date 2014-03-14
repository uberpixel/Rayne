//
//  RNDecal.cpp
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNDecal.h"
#include "RNResourceCoordinator.h"
#include "RNWorld.h"
#include "RNEntity.h"

namespace RN
{
	RNDefineMeta(Decal)
	
	Decal::Decal(bool tangents)
	: _angle("angle", 180.0f, &Decal::GetAngle, &Decal::SetAngle), _tangents(tangents)
	{
		Initialize();
	}
	
	Decal::Decal(Texture *texture, bool tangents)
	: _angle("angle", 180.0f, &Decal::GetAngle, &Decal::SetAngle), _tangents(tangents)
	{
		Initialize();
		SetTexture(texture);
	}
	
	Decal::Decal(Texture *texture, const Vector3 &position, bool tangents)
	: _angle("angle", 180.0f, &Decal::GetAngle, &Decal::SetAngle), _tangents(tangents)
	{
		Initialize();
		SetTexture(texture);
		SetPosition(position);
	}
	
	Decal::Decal(const Decal *other) :
		SceneNode(other),
		_angle("angle", other->GetAngle(), &Decal::GetAngle, &Decal::SetAngle),
		_tangents(other->_tangents)
	{
		Decal *temp = const_cast<Decal *>(other);
		LockGuard<Object *> lock(temp);
		
		Initialize();
		
		_material->Release();
		_material = other->_material->Copy();
	}
	
	Decal::~Decal()
	{
		_mesh->Release();
		_material->Release();
	}
	
	
	void Decal::Initialize()
	{
		AddObservable(&_angle);
		_angleCos = cosf(Math::DegreesToRadians(_angle/2.0f));
		
		_material = new Material();
		_material->SetShader(ResourceCoordinator::GetSharedInstance()->GetResourceWithName<Shader>(kRNResourceKeyTexture1Shader, nullptr));
		_material->SetBlending(true);
		_material->SetBlendMode(Material::BlendMode::SourceAlpha, Material::BlendMode::OneMinusSourceAlpha);
		_material->SetAlphaBlendMode(Material::BlendMode::Zero, Material::BlendMode::One);
		_material->SetDepthWrite(false);
		_material->SetPolygonOffset(true);
		_material->SetPolygonOffsetFactor(-2.0f);
		_material->SetPolygonOffsetUnits(1.0f);
		_material->SetCullMode(Material::CullMode::None);
		
		MeshDescriptor vertexDescriptor(MeshFeature::Vertices);
		vertexDescriptor.elementMember = 3;
		vertexDescriptor.elementSize   = sizeof(Vector3);
		
		MeshDescriptor normalDescriptor(MeshFeature::Normals);
		normalDescriptor.elementMember = 3;
		normalDescriptor.elementSize   = sizeof(Vector3);
		
		MeshDescriptor texcoordDescriptor(MeshFeature::UVSet0);
		texcoordDescriptor.elementMember = 2;
		texcoordDescriptor.elementSize   = sizeof(Vector2);

		std::vector<MeshDescriptor> descriptors = { vertexDescriptor, normalDescriptor, texcoordDescriptor };
		
		if(_tangents)
		{
			MeshDescriptor tangentDescriptor(MeshFeature::Tangents);
			tangentDescriptor.elementMember = 4;
			tangentDescriptor.elementSize   = sizeof(Vector4);
			
			descriptors.push_back(tangentDescriptor);
		}
		
		_mesh = new Mesh(descriptors, 0, 0);
		_mesh->SetDrawMode(Mesh::DrawMode::Triangles);
		_mesh->Retain();
		
		UpdateMesh();
	}
	
	void Decal::SetTexture(Texture *texture, float scaleFactor)
	{
		_material->RemoveTextures();
		
		if(texture)
		{
			_material->AddTexture(texture);
		}
	}
	
	Texture *Decal::GetTexture() const
	{
		const Array *array = _material->GetTextures();
		return (array->GetCount() > 0) ? array->GetObjectAtIndex<Texture>(0) : nullptr;
	}
	
	void Decal::SetAngle(float angle)
	{
		_angle = angle;
		_angleCos = cosf(Math::DegreesToRadians(_angle/2.0f));
		UpdateMesh();
	}
	
	void Decal::UpdateMesh()
	{
		AABB box;
		box.position = GetWorldPosition();
		box.maxExtend = GetWorldScale();
		box.minExtend = -GetWorldScale();
		box.SetRotation(GetWorldRotation());
		SetBoundingBox(box);
		SetBoundingSphere(Sphere(box));
		World *world = World::GetActiveWorld();
		world->ApplyNodes();
		std::vector<SceneNode *> nodes = world->GetSceneManager()->GetSceneNodes(box);
		
		Matrix projection = Matrix::WithProjectionOrthogonal(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
		projection.Scale(Vector3(0.5f, 0.5f, 0.5f));
		projection.Translate(Vector3(1.0f, -1.0, -1.0));
		projection *= GetWorldTransform().GetInverse();
		Matrix invprojection = projection.GetInverse();
		
		std::vector<Vector3> vertices;
		std::vector<Vector3> normals;
		
		for(SceneNode *node : nodes)
		{
			Entity *entity = node->Downcast<Entity>();
			
			if(entity && entity->GetRenderGroup() != 31)
			{
				Model *model = entity->GetModel();
				size_t meshcount = model->GetMeshCount(0);
				
				Matrix transform = entity->GetWorldTransform();
				Matrix normalTransform = entity->GetWorldRotation().GetRotationMatrix();
				
				for(int m = 0; m < meshcount; m++)
				{
					Mesh *mesh = model->GetMeshAtIndex(0, m);
					
					AABB meshAABB = mesh->GetBoundingBox();
					meshAABB *= entity->GetWorldScale();
					meshAABB.SetRotation(entity->GetWorldRotation());
					meshAABB.position += entity->GetWorldPosition();
					if(!meshAABB.Intersects(box))
						continue;
					if(!mesh->SupportsFeature(MeshFeature::Vertices))
						continue;
					if(!mesh->SupportsFeature(MeshFeature::Normals))
						continue;
					
					Mesh::Chunk chunk = mesh->GetChunk();
					const MeshDescriptor *verticesDescriptor = mesh->GetDescriptorForFeature(MeshFeature::Vertices);
					const MeshDescriptor *normalsDescriptor = mesh->GetDescriptorForFeature(MeshFeature::Normals);
					
					if(verticesDescriptor->elementMember == 3 && normalsDescriptor->elementMember == 3)
					{
						Mesh::ElementIterator<Vector3> vertexIterator = chunk.GetIterator<Vector3>(MeshFeature::Vertices);
						Mesh::ElementIterator<Vector3> normalsIterator = chunk.GetIterator<Vector3>(MeshFeature::Normals);
						
						if(mesh->SupportsFeature(MeshFeature::Indices))
						{
							const MeshDescriptor *indicesDescriptor = mesh->GetDescriptorForFeature(MeshFeature::Indices);
							if(indicesDescriptor->elementSize == 2)
							{
								const uint16 *indices = mesh->GetIndicesData<uint16>();
								for(int i = 0; i < mesh->GetIndicesCount(); i += 3)
								{
									Vector3 v0 = transform * (*vertexIterator.Seek(indices[i]));
									Vector3 v1 = transform * (*vertexIterator.Seek(indices[i+1]));
									Vector3 v2 = transform * (*vertexIterator.Seek(indices[i+2]));
									
									AABB triangleaabb;
									triangleaabb.minExtend.x = std::min({v0.x, v1.x, v2.x});
									triangleaabb.minExtend.y = std::min({v0.y, v1.y, v2.y});
									triangleaabb.minExtend.z = std::min({v0.z, v1.z, v2.z});
									
									triangleaabb.maxExtend.x = std::max({v0.x, v1.x, v2.x});
									triangleaabb.maxExtend.y = std::max({v0.y, v1.y, v2.y});
									triangleaabb.maxExtend.z = std::max({v0.z, v1.z, v2.z});
									
									Vector3 facenormal = (v2-v0).GetCrossProduct(v1-v0);
									
									if(triangleaabb.Intersects(box) && facenormal.GetDotProduct(GetForward()) > _angleCos)
									{
										normals.push_back(normalTransform*(*normalsIterator.Seek(indices[i])));
										normals.push_back(normalTransform*(*normalsIterator.Seek(indices[i+1])));
										normals.push_back(normalTransform*(*normalsIterator.Seek(indices[i+1])));
										
										Vector4 proj0 = projection * Vector4(v0, 1.0);
										vertices.push_back(Vector3(proj0));
										
										Vector4 proj1 = projection * Vector4(v1, 1.0);
										vertices.push_back(Vector3(proj1));
										
										Vector4 proj2 = projection * Vector4(v2, 1.0);
										vertices.push_back(Vector3(proj2));
									}
								}
							}
						}
					}
					else
					{
						break;
					}
				}
			}
		}
		
		std::vector<Vector3> otherVertices;
		std::vector<Vector3> otherNormals;
		
		std::vector<Vector3> tempVertices;
		std::vector<Vector3> tempNormals;
		
		//Left
		for(int i = 0; i < vertices.size(); i += 3)
		{
			tempVertices.clear();
			tempNormals.clear();
			if(vertices[i].x < 0.0f || vertices[i+1].x < 0.0f || vertices[i+2].x < 0.0f)
			{
				if(vertices[i].x < 0.0f && vertices[i+1].x < 0.0f && vertices[i+2].x < 0.0f)
				{
					continue;
				}
				
				if(vertices[i].x < 0.0f && vertices[i+1].x < 0.0f)
				{
					float blend0 = vertices[i+2].x/(vertices[i+2].x-vertices[i].x);
					float blend1 = vertices[i+2].x/(vertices[i+2].x-vertices[i+1].x);
					
					Vector3 v0 = vertices[i+2].GetLerp(vertices[i], blend0);
					Vector3 v1 = vertices[i+2].GetLerp(vertices[i+1], blend1);
					Vector3 n0 = normals[i+2].GetLerp(normals[i], blend0);
					Vector3 n1 = normals[i+2].GetLerp(normals[i+1], blend1);
					
					tempVertices.push_back(v0);
					tempVertices.push_back(v1);
					tempVertices.push_back(vertices[i+2]);
					
					tempNormals.push_back(n0);
					tempNormals.push_back(n1);
					tempNormals.push_back(normals[i+2]);
				}
				else if(vertices[i+1].x < 0.0f && vertices[i+2].x < 0.0f)
				{
					float blend0 = vertices[i].x/(vertices[i].x-vertices[i+1].x);
					float blend1 = vertices[i].x/(vertices[i].x-vertices[i+2].x);
					
					Vector3 v0 = vertices[i].GetLerp(vertices[i+1], blend0);
					Vector3 v1 = vertices[i].GetLerp(vertices[i+2], blend1);
					Vector3 n0 = normals[i].GetLerp(normals[i+1], blend0);
					Vector3 n1 = normals[i].GetLerp(normals[i+2], blend1);
					
					tempVertices.push_back(v0);
					tempVertices.push_back(v1);
					tempVertices.push_back(vertices[i]);
					
					tempNormals.push_back(n0);
					tempNormals.push_back(n1);
					tempNormals.push_back(normals[i]);
				}
				else if(vertices[i].x < 0.0f && vertices[i+2].x < 0.0f)
				{
					float blend0 = vertices[i+1].x/(vertices[i+1].x-vertices[i].x);
					float blend1 = vertices[i+1].x/(vertices[i+1].x-vertices[i+2].x);
					
					Vector3 v0 = vertices[i+1].GetLerp(vertices[i], blend0);
					Vector3 v1 = vertices[i+1].GetLerp(vertices[i+2], blend1);
					Vector3 n0 = normals[i+1].GetLerp(normals[i], blend0);
					Vector3 n1 = normals[i+1].GetLerp(normals[i+2], blend1);
					
					tempVertices.push_back(v1);
					tempVertices.push_back(v0);
					tempVertices.push_back(vertices[i+1]);
					
					tempNormals.push_back(n1);
					tempNormals.push_back(n0);
					tempNormals.push_back(normals[i+1]);
				}
				else
				{
					if(vertices[i].x < 0.0f)
					{
						float blend0 = vertices[i+1].x/(vertices[i+1].x-vertices[i].x);
						float blend1 = vertices[i+2].x/(vertices[i+2].x-vertices[i].x);
						
						Vector3 v0 = vertices[i+1].GetLerp(vertices[i], blend0);
						Vector3 v1 = vertices[i+2].GetLerp(vertices[i], blend1);
						Vector3 n0 = normals[i+1].GetLerp(normals[i], blend0);
						Vector3 n1 = normals[i+2].GetLerp(normals[i], blend1);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(vertices[i+1]);
						tempVertices.push_back(vertices[i+2]);
						
						tempVertices.push_back(v1);
						tempVertices.push_back(v0);
						tempVertices.push_back(vertices[i+2]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(normals[i+1]);
						tempNormals.push_back(normals[i+2]);
						
						tempNormals.push_back(n1);
						tempNormals.push_back(n0);
						tempNormals.push_back(normals[i+2]);
					}
					else if(vertices[i+1].x < 0.0f)
					{
						float blend0 = vertices[i].x/(vertices[i].x-vertices[i+1].x);
						float blend1 = vertices[i+2].x/(vertices[i+2].x-vertices[i+1].x);
						
						Vector3 v0 = vertices[i].GetLerp(vertices[i+1], blend0);
						Vector3 v1 = vertices[i+2].GetLerp(vertices[i+1], blend1);
						Vector3 n0 = normals[i].GetLerp(normals[i+1], blend0);
						Vector3 n1 = normals[i+2].GetLerp(normals[i+1], blend1);
						
						tempVertices.push_back(vertices[i]);
						tempVertices.push_back(v0);
						tempVertices.push_back(vertices[i+2]);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(v1);
						tempVertices.push_back(vertices[i+2]);
						
						tempNormals.push_back(normals[i]);
						tempNormals.push_back(n0);
						tempNormals.push_back(normals[i+2]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(n1);
						tempNormals.push_back(normals[i+2]);
					}
					else
					{
						float blend0 = vertices[i+1].x/(vertices[i+1].x-vertices[i+2].x);
						float blend1 = vertices[i].x/(vertices[i].x-vertices[i+2].x);
						
						Vector3 v0 = vertices[i+1].GetLerp(vertices[i+2], blend0);
						Vector3 v1 = vertices[i].GetLerp(vertices[i+2], blend1);
						Vector3 n0 = normals[i+1].GetLerp(normals[i+2], blend0);
						Vector3 n1 = normals[i].GetLerp(normals[i+2], blend1);
						
						tempVertices.push_back(vertices[i+1]);
						tempVertices.push_back(v0);
						tempVertices.push_back(vertices[i]);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(v1);
						tempVertices.push_back(vertices[i]);
						
						tempNormals.push_back(normals[i+1]);
						tempNormals.push_back(n0);
						tempNormals.push_back(normals[i]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(n1);
						tempNormals.push_back(normals[i]);
					}
				}
			}
			else
			{
				tempVertices.push_back(vertices[i]);
				tempVertices.push_back(vertices[i+1]);
				tempVertices.push_back(vertices[i+2]);
				
				tempNormals.push_back(normals[i]);
				tempNormals.push_back(normals[i+1]);
				tempNormals.push_back(normals[i+2]);
			}
			
			otherVertices.insert(otherVertices.end(), tempVertices.begin(), tempVertices.end());
			otherNormals.insert(otherNormals.end(), tempNormals.begin(), tempNormals.end());
		}
		
		vertices.clear();
		normals.clear();
		
		//Right
		for(int i = 0; i < otherVertices.size(); i += 3)
		{
			tempVertices.clear();
			tempNormals.clear();
			if(otherVertices[i].x > 1.0f || otherVertices[i+1].x > 1.0f || otherVertices[i+2].x > 1.0f)
			{
				if(otherVertices[i].x > 1.0f && otherVertices[i+1].x > 1.0f && otherVertices[i+2].x > 1.0f)
				{
					continue;
				}
				
				if(otherVertices[i].x > 1.0f && otherVertices[i+1].x > 1.0f)
				{
					float blend0 = (1.0f-otherVertices[i+2].x)/(otherVertices[i].x-otherVertices[i+2].x);
					float blend1 = (1.0f-otherVertices[i+2].x)/(otherVertices[i+1].x-otherVertices[i+2].x);
					
					Vector3 v0 = otherVertices[i+2].GetLerp(otherVertices[i], blend0);
					Vector3 v1 = otherVertices[i+2].GetLerp(otherVertices[i+1], blend1);
					Vector3 n0 = otherNormals[i+2].GetLerp(otherNormals[i], blend0);
					Vector3 n1 = otherNormals[i+2].GetLerp(otherNormals[i+1], blend1);
					
					tempVertices.push_back(v0);
					tempVertices.push_back(v1);
					tempVertices.push_back(otherVertices[i+2]);
					
					tempNormals.push_back(n0);
					tempNormals.push_back(n1);
					tempNormals.push_back(otherNormals[i+2]);
				}
				else if(otherVertices[i+1].x > 1.0f && otherVertices[i+2].x > 1.0f)
				{
					float blend0 = (1.0f-otherVertices[i].x)/(otherVertices[i+1].x-otherVertices[i].x);
					float blend1 = (1.0f-otherVertices[i].x)/(otherVertices[i+2].x-otherVertices[i].x);
					
					Vector3 v0 = otherVertices[i].GetLerp(otherVertices[i+1], blend0);
					Vector3 v1 = otherVertices[i].GetLerp(otherVertices[i+2], blend1);
					Vector3 n0 = otherNormals[i].GetLerp(otherNormals[i+1], blend0);
					Vector3 n1 = otherNormals[i].GetLerp(otherNormals[i+2], blend1);
					
					tempVertices.push_back(v0);
					tempVertices.push_back(v1);
					tempVertices.push_back(otherVertices[i]);
					
					tempNormals.push_back(n0);
					tempNormals.push_back(n1);
					tempNormals.push_back(otherNormals[i]);
				}
				else if(otherVertices[i].x > 1.0f && otherVertices[i+2].x > 1.0f)
				{
					float blend0 = (1.0f-otherVertices[i+1].x)/(otherVertices[i].x-otherVertices[i+1].x);
					float blend1 = (1.0f-otherVertices[i+1].x)/(otherVertices[i+2].x-otherVertices[i+1].x);
					
					Vector3 v0 = otherVertices[i+1].GetLerp(otherVertices[i], blend0);
					Vector3 v1 = otherVertices[i+1].GetLerp(otherVertices[i+2], blend1);
					Vector3 n0 = otherNormals[i+1].GetLerp(otherNormals[i], blend0);
					Vector3 n1 = otherNormals[i+1].GetLerp(otherNormals[i+2], blend1);
					
					tempVertices.push_back(v1);
					tempVertices.push_back(v0);
					tempVertices.push_back(otherVertices[i+1]);
					
					tempNormals.push_back(n1);
					tempNormals.push_back(n0);
					tempNormals.push_back(otherNormals[i+1]);
				}
				else
				{
					if(otherVertices[i].x > 1.0f)
					{
						float blend0 = (1.0-otherVertices[i+1].x)/(otherVertices[i].x-otherVertices[i+1].x);
						float blend1 = (1.0-otherVertices[i+2].x)/(otherVertices[i].x-otherVertices[i+2].x);
						
						Vector3 v0 = otherVertices[i+1].GetLerp(otherVertices[i], blend0);
						Vector3 v1 = otherVertices[i+2].GetLerp(otherVertices[i], blend1);
						Vector3 n0 = otherNormals[i+1].GetLerp(otherNormals[i], blend0);
						Vector3 n1 = otherNormals[i+2].GetLerp(otherNormals[i], blend1);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i+1]);
						tempVertices.push_back(otherVertices[i+2]);
						
						tempVertices.push_back(v1);
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i+2]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i+1]);
						tempNormals.push_back(otherNormals[i+2]);
						
						tempNormals.push_back(n1);
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i+2]);
					}
					else if(otherVertices[i+1].x > 1.0f)
					{
						float blend0 = (1.0-otherVertices[i].x)/(otherVertices[i+1].x-otherVertices[i].x);
						float blend1 = (1.0-otherVertices[i+2].x)/(otherVertices[i+1].x-otherVertices[i+2].x);
						
						Vector3 v0 = otherVertices[i].GetLerp(otherVertices[i+1], blend0);
						Vector3 v1 = otherVertices[i+2].GetLerp(otherVertices[i+1], blend1);
						Vector3 n0 = otherNormals[i].GetLerp(otherNormals[i+1], blend0);
						Vector3 n1 = otherNormals[i+2].GetLerp(otherNormals[i+1], blend1);
						
						tempVertices.push_back(otherVertices[i]);
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i+2]);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(v1);
						tempVertices.push_back(otherVertices[i+2]);
						
						tempNormals.push_back(otherNormals[i]);
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i+2]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(n1);
						tempNormals.push_back(otherNormals[i+2]);
					}
					else
					{
						float blend0 = (1.0-otherVertices[i].x)/(otherVertices[i+2].x-otherVertices[i].x);
						float blend1 = (1.0-otherVertices[i+1].x)/(otherVertices[i+2].x-otherVertices[i+1].x);
						
						Vector3 v0 = otherVertices[i].GetLerp(otherVertices[i+2], blend0);
						Vector3 v1 = otherVertices[i+1].GetLerp(otherVertices[i+2], blend1);
						Vector3 n0 = otherNormals[i].GetLerp(otherNormals[i+2], blend0);
						Vector3 n1 = otherNormals[i+1].GetLerp(otherNormals[i+2], blend1);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i]);
						tempVertices.push_back(otherVertices[i+1]);
						
						tempVertices.push_back(v1);
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i+1]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i]);
						tempNormals.push_back(otherNormals[i+1]);
						
						tempNormals.push_back(n1);
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i+1]);
					}
				}
			}
			else
			{
				tempVertices.push_back(otherVertices[i]);
				tempVertices.push_back(otherVertices[i+1]);
				tempVertices.push_back(otherVertices[i+2]);
				
				tempNormals.push_back(otherNormals[i]);
				tempNormals.push_back(otherNormals[i+1]);
				tempNormals.push_back(otherNormals[i+2]);
			}
			
			vertices.insert(vertices.end(), tempVertices.begin(), tempVertices.end());
			normals.insert(normals.end(), tempNormals.begin(), tempNormals.end());
		}
		
		otherVertices.clear();
		otherNormals.clear();
		
		//Bottom
		for(int i = 0; i < vertices.size(); i += 3)
		{
			tempVertices.clear();
			tempNormals.clear();
			if(vertices[i].y < 0.0f || vertices[i+1].y < 0.0f || vertices[i+2].y < 0.0f)
			{
				if(vertices[i].y < 0.0f && vertices[i+1].y < 0.0f && vertices[i+2].y < 0.0f)
				{
					continue;
				}
				
				if(vertices[i].y < 0.0f && vertices[i+1].y < 0.0f)
				{
					float blend0 = vertices[i+2].y/(vertices[i+2].y-vertices[i].y);
					float blend1 = vertices[i+2].y/(vertices[i+2].y-vertices[i+1].y);
					
					Vector3 v0 = vertices[i+2].GetLerp(vertices[i], blend0);
					Vector3 v1 = vertices[i+2].GetLerp(vertices[i+1], blend1);
					Vector3 n0 = normals[i+2].GetLerp(normals[i], blend0);
					Vector3 n1 = normals[i+2].GetLerp(normals[i+1], blend1);
					
					tempVertices.push_back(v0);
					tempVertices.push_back(v1);
					tempVertices.push_back(vertices[i+2]);
					
					tempNormals.push_back(n0);
					tempNormals.push_back(n1);
					tempNormals.push_back(normals[i+2]);
				}
				else if(vertices[i+1].y < 0.0f && vertices[i+2].y < 0.0f)
				{
					float blend0 = vertices[i].y/(vertices[i].y-vertices[i+1].y);
					float blend1 = vertices[i].y/(vertices[i].y-vertices[i+2].y);
					
					Vector3 v0 = vertices[i].GetLerp(vertices[i+1], blend0);
					Vector3 v1 = vertices[i].GetLerp(vertices[i+2], blend1);
					Vector3 n0 = normals[i].GetLerp(normals[i+1], blend0);
					Vector3 n1 = normals[i].GetLerp(normals[i+2], blend1);
					
					tempVertices.push_back(v0);
					tempVertices.push_back(v1);
					tempVertices.push_back(vertices[i]);
					
					tempNormals.push_back(n0);
					tempNormals.push_back(n1);
					tempNormals.push_back(normals[i]);
				}
				else if(vertices[i].y < 0.0f && vertices[i+2].y < 0.0f)
				{
					float blend0 = vertices[i+1].y/(vertices[i+1].y-vertices[i].y);
					float blend1 = vertices[i+1].y/(vertices[i+1].y-vertices[i+2].y);
					
					Vector3 v0 = vertices[i+1].GetLerp(vertices[i], blend0);
					Vector3 v1 = vertices[i+1].GetLerp(vertices[i+2], blend1);
					Vector3 n0 = normals[i+1].GetLerp(normals[i], blend0);
					Vector3 n1 = normals[i+1].GetLerp(normals[i+2], blend1);
					
					tempVertices.push_back(v1);
					tempVertices.push_back(v0);
					tempVertices.push_back(vertices[i+1]);
					
					tempNormals.push_back(n1);
					tempNormals.push_back(n0);
					tempNormals.push_back(normals[i+1]);
				}
				else
				{
					if(vertices[i].y < 0.0f)
					{
						float blend0 = vertices[i+1].y/(vertices[i+1].y-vertices[i].y);
						float blend1 = vertices[i+2].y/(vertices[i+2].y-vertices[i].y);
						
						Vector3 v0 = vertices[i+1].GetLerp(vertices[i], blend0);
						Vector3 v1 = vertices[i+2].GetLerp(vertices[i], blend1);
						Vector3 n0 = normals[i+1].GetLerp(normals[i], blend0);
						Vector3 n1 = normals[i+2].GetLerp(normals[i], blend1);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(vertices[i+1]);
						tempVertices.push_back(vertices[i+2]);
						
						tempVertices.push_back(v1);
						tempVertices.push_back(v0);
						tempVertices.push_back(vertices[i+2]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(normals[i+1]);
						tempNormals.push_back(normals[i+2]);
						
						tempNormals.push_back(n1);
						tempNormals.push_back(n0);
						tempNormals.push_back(normals[i+2]);
					}
					else if(vertices[i+1].y < 0.0f)
					{
						float blend0 = vertices[i].y/(vertices[i].y-vertices[i+1].y);
						float blend1 = vertices[i+2].y/(vertices[i+2].y-vertices[i+1].y);
						
						Vector3 v0 = vertices[i].GetLerp(vertices[i+1], blend0);
						Vector3 v1 = vertices[i+2].GetLerp(vertices[i+1], blend1);
						Vector3 n0 = normals[i].GetLerp(normals[i+1], blend0);
						Vector3 n1 = normals[i+2].GetLerp(normals[i+1], blend1);
						
						tempVertices.push_back(vertices[i]);
						tempVertices.push_back(v0);
						tempVertices.push_back(vertices[i+2]);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(v1);
						tempVertices.push_back(vertices[i+2]);
						
						tempNormals.push_back(normals[i]);
						tempNormals.push_back(n0);
						tempNormals.push_back(normals[i+2]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(n1);
						tempNormals.push_back(normals[i+2]);
					}
					else
					{
						float blend0 = vertices[i+1].y/(vertices[i+1].y-vertices[i+2].y);
						float blend1 = vertices[i].y/(vertices[i].y-vertices[i+2].y);
						
						Vector3 v0 = vertices[i+1].GetLerp(vertices[i+2], blend0);
						Vector3 v1 = vertices[i].GetLerp(vertices[i+2], blend1);
						Vector3 n0 = normals[i+1].GetLerp(normals[i+2], blend0);
						Vector3 n1 = normals[i].GetLerp(normals[i+2], blend1);
						
						tempVertices.push_back(vertices[i+1]);
						tempVertices.push_back(v0);
						tempVertices.push_back(vertices[i]);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(v1);
						tempVertices.push_back(vertices[i]);
						
						tempNormals.push_back(normals[i+1]);
						tempNormals.push_back(n0);
						tempNormals.push_back(normals[i]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(n1);
						tempNormals.push_back(normals[i]);
					}
				}
			}
			else
			{
				tempVertices.push_back(vertices[i]);
				tempVertices.push_back(vertices[i+1]);
				tempVertices.push_back(vertices[i+2]);
				
				tempNormals.push_back(normals[i]);
				tempNormals.push_back(normals[i+1]);
				tempNormals.push_back(normals[i+2]);
			}
			
			otherVertices.insert(otherVertices.end(), tempVertices.begin(), tempVertices.end());
			otherNormals.insert(otherNormals.end(), tempNormals.begin(), tempNormals.end());
		}
		
		vertices.clear();
		normals.clear();
		
		//Top
		for(int i = 0; i < otherVertices.size(); i += 3)
		{
			tempVertices.clear();
			tempNormals.clear();
			if(otherVertices[i].y > 1.0f || otherVertices[i+1].y > 1.0f || otherVertices[i+2].y > 1.0f)
			{
				if(otherVertices[i].y > 1.0f && otherVertices[i+1].y > 1.0f && otherVertices[i+2].y > 1.0f)
				{
					continue;
				}
				
				if(otherVertices[i].y > 1.0f && otherVertices[i+1].y > 1.0f)
				{
					float blend0 = (1.0f-otherVertices[i+2].y)/(otherVertices[i].y-otherVertices[i+2].y);
					float blend1 = (1.0f-otherVertices[i+2].y)/(otherVertices[i+1].y-otherVertices[i+2].y);
					
					Vector3 v0 = otherVertices[i+2].GetLerp(otherVertices[i], blend0);
					Vector3 v1 = otherVertices[i+2].GetLerp(otherVertices[i+1], blend1);
					Vector3 n0 = otherNormals[i+2].GetLerp(otherNormals[i], blend0);
					Vector3 n1 = otherNormals[i+2].GetLerp(otherNormals[i+1], blend1);
					
					tempVertices.push_back(v0);
					tempVertices.push_back(v1);
					tempVertices.push_back(otherVertices[i+2]);
					
					tempNormals.push_back(n0);
					tempNormals.push_back(n1);
					tempNormals.push_back(otherNormals[i+2]);
				}
				else if(otherVertices[i+1].y > 1.0f && otherVertices[i+2].y > 1.0f)
				{
					float blend0 = (1.0f-otherVertices[i].y)/(otherVertices[i+1].y-otherVertices[i].y);
					float blend1 = (1.0f-otherVertices[i].y)/(otherVertices[i+2].y-otherVertices[i].y);
					
					Vector3 v0 = otherVertices[i].GetLerp(otherVertices[i+1], blend0);
					Vector3 v1 = otherVertices[i].GetLerp(otherVertices[i+2], blend1);
					Vector3 n0 = otherNormals[i].GetLerp(otherNormals[i+1], blend0);
					Vector3 n1 = otherNormals[i].GetLerp(otherNormals[i+2], blend1);
					
					tempVertices.push_back(v0);
					tempVertices.push_back(v1);
					tempVertices.push_back(otherVertices[i]);
					
					tempNormals.push_back(n0);
					tempNormals.push_back(n1);
					tempNormals.push_back(otherNormals[i]);
				}
				else if(otherVertices[i].y > 1.0f && otherVertices[i+2].y > 1.0f)
				{
					float blend0 = (1.0f-otherVertices[i+1].y)/(otherVertices[i].y-otherVertices[i+1].y);
					float blend1 = (1.0f-otherVertices[i+1].y)/(otherVertices[i+2].y-otherVertices[i+1].y);
					
					Vector3 v0 = otherVertices[i+1].GetLerp(otherVertices[i], blend0);
					Vector3 v1 = otherVertices[i+1].GetLerp(otherVertices[i+2], blend1);
					Vector3 n0 = otherNormals[i+1].GetLerp(otherNormals[i], blend0);
					Vector3 n1 = otherNormals[i+1].GetLerp(otherNormals[i+2], blend1);
					
					tempVertices.push_back(v1);
					tempVertices.push_back(v0);
					tempVertices.push_back(otherVertices[i+1]);
					
					tempNormals.push_back(n1);
					tempNormals.push_back(n0);
					tempNormals.push_back(otherNormals[i+1]);
				}
				else
				{
					if(otherVertices[i].y > 1.0f)
					{
						float blend0 = (1.0-otherVertices[i+1].y)/(otherVertices[i].y-otherVertices[i+1].y);
						float blend1 = (1.0-otherVertices[i+2].y)/(otherVertices[i].y-otherVertices[i+2].y);
						
						Vector3 v0 = otherVertices[i+1].GetLerp(otherVertices[i], blend0);
						Vector3 v1 = otherVertices[i+2].GetLerp(otherVertices[i], blend1);
						Vector3 n0 = otherNormals[i+1].GetLerp(otherNormals[i], blend0);
						Vector3 n1 = otherNormals[i+2].GetLerp(otherNormals[i], blend1);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i+1]);
						tempVertices.push_back(otherVertices[i+2]);
						
						tempVertices.push_back(v1);
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i+2]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i+1]);
						tempNormals.push_back(otherNormals[i+2]);
						
						tempNormals.push_back(n1);
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i+2]);
					}
					else if(otherVertices[i+1].y > 1.0f)
					{
						float blend0 = (1.0-otherVertices[i].y)/(otherVertices[i+1].y-otherVertices[i].y);
						float blend1 = (1.0-otherVertices[i+2].y)/(otherVertices[i+1].y-otherVertices[i+2].y);
						
						Vector3 v0 = otherVertices[i].GetLerp(otherVertices[i+1], blend0);
						Vector3 v1 = otherVertices[i+2].GetLerp(otherVertices[i+1], blend1);
						Vector3 n0 = otherNormals[i].GetLerp(otherNormals[i+1], blend0);
						Vector3 n1 = otherNormals[i+2].GetLerp(otherNormals[i+1], blend1);
						
						tempVertices.push_back(otherVertices[i]);
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i+2]);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(v1);
						tempVertices.push_back(otherVertices[i+2]);
						
						tempNormals.push_back(otherNormals[i]);
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i+2]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(n1);
						tempNormals.push_back(otherNormals[i+2]);
					}
					else
					{
						float blend0 = (1.0-otherVertices[i].y)/(otherVertices[i+2].y-otherVertices[i].y);
						float blend1 = (1.0-otherVertices[i+1].y)/(otherVertices[i+2].y-otherVertices[i+1].y);
						
						Vector3 v0 = otherVertices[i].GetLerp(otherVertices[i+2], blend0);
						Vector3 v1 = otherVertices[i+1].GetLerp(otherVertices[i+2], blend1);
						Vector3 n0 = otherNormals[i].GetLerp(otherNormals[i+2], blend0);
						Vector3 n1 = otherNormals[i+1].GetLerp(otherNormals[i+2], blend1);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i]);
						tempVertices.push_back(otherVertices[i+1]);
						
						tempVertices.push_back(v1);
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i+1]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i]);
						tempNormals.push_back(otherNormals[i+1]);
						
						tempNormals.push_back(n1);
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i+1]);
					}
				}
			}
			else
			{
				tempVertices.push_back(otherVertices[i]);
				tempVertices.push_back(otherVertices[i+1]);
				tempVertices.push_back(otherVertices[i+2]);
				
				tempNormals.push_back(otherNormals[i]);
				tempNormals.push_back(otherNormals[i+1]);
				tempNormals.push_back(otherNormals[i+2]);
			}
			
			vertices.insert(vertices.end(), tempVertices.begin(), tempVertices.end());
			normals.insert(normals.end(), tempNormals.begin(), tempNormals.end());
		}
		
		otherVertices.clear();
		otherNormals.clear();
		
		//Back
		for(int i = 0; i < vertices.size(); i += 3)
		{
			tempVertices.clear();
			tempNormals.clear();
			if(vertices[i].z < 0.0f || vertices[i+1].z < 0.0f || vertices[i+2].z < 0.0f)
			{
				if(vertices[i].z < 0.0f && vertices[i+1].z < 0.0f && vertices[i+2].z < 0.0f)
				{
					continue;
				}
				
				if(vertices[i].z < 0.0f && vertices[i+1].z < 0.0f)
				{
					float blend0 = vertices[i+2].z/(vertices[i+2].z-vertices[i].z);
					float blend1 = vertices[i+2].z/(vertices[i+2].z-vertices[i+1].z);
					
					Vector3 v0 = vertices[i+2].GetLerp(vertices[i], blend0);
					Vector3 v1 = vertices[i+2].GetLerp(vertices[i+1], blend1);
					Vector3 n0 = normals[i+2].GetLerp(normals[i], blend0);
					Vector3 n1 = normals[i+2].GetLerp(normals[i+1], blend1);
					
					tempVertices.push_back(v0);
					tempVertices.push_back(v1);
					tempVertices.push_back(vertices[i+2]);
					
					tempNormals.push_back(n0);
					tempNormals.push_back(n1);
					tempNormals.push_back(normals[i+2]);
				}
				else if(vertices[i+1].z < 0.0f && vertices[i+2].z < 0.0f)
				{
					float blend0 = vertices[i].z/(vertices[i].z-vertices[i+1].z);
					float blend1 = vertices[i].z/(vertices[i].z-vertices[i+2].z);
					
					Vector3 v0 = vertices[i].GetLerp(vertices[i+1], blend0);
					Vector3 v1 = vertices[i].GetLerp(vertices[i+2], blend1);
					Vector3 n0 = normals[i].GetLerp(normals[i+1], blend0);
					Vector3 n1 = normals[i].GetLerp(normals[i+2], blend1);
					
					tempVertices.push_back(v0);
					tempVertices.push_back(v1);
					tempVertices.push_back(vertices[i]);
					
					tempNormals.push_back(n0);
					tempNormals.push_back(n1);
					tempNormals.push_back(normals[i]);
				}
				else if(vertices[i].z < 0.0f && vertices[i+2].z < 0.0f)
				{
					float blend0 = vertices[i+1].z/(vertices[i+1].z-vertices[i].z);
					float blend1 = vertices[i+1].z/(vertices[i+1].z-vertices[i+2].z);
					
					Vector3 v0 = vertices[i+1].GetLerp(vertices[i], blend0);
					Vector3 v1 = vertices[i+1].GetLerp(vertices[i+2], blend1);
					Vector3 n0 = normals[i+1].GetLerp(normals[i], blend0);
					Vector3 n1 = normals[i+1].GetLerp(normals[i+2], blend1);
					
					tempVertices.push_back(v1);
					tempVertices.push_back(v0);
					tempVertices.push_back(vertices[i+1]);
					
					tempNormals.push_back(n1);
					tempNormals.push_back(n0);
					tempNormals.push_back(normals[i+1]);
				}
				else
				{
					if(vertices[i].z < 0.0f)
					{
						float blend0 = vertices[i+1].z/(vertices[i+1].z-vertices[i].z);
						float blend1 = vertices[i+2].z/(vertices[i+2].z-vertices[i].z);
						
						Vector3 v0 = vertices[i+1].GetLerp(vertices[i], blend0);
						Vector3 v1 = vertices[i+2].GetLerp(vertices[i], blend1);
						Vector3 n0 = normals[i+1].GetLerp(normals[i], blend0);
						Vector3 n1 = normals[i+2].GetLerp(normals[i], blend1);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(vertices[i+1]);
						tempVertices.push_back(vertices[i+2]);
						
						tempVertices.push_back(v1);
						tempVertices.push_back(v0);
						tempVertices.push_back(vertices[i+2]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(normals[i+1]);
						tempNormals.push_back(normals[i+2]);
						
						tempNormals.push_back(n1);
						tempNormals.push_back(n0);
						tempNormals.push_back(normals[i+2]);
					}
					else if(vertices[i+1].z < 0.0f)
					{
						float blend0 = vertices[i].z/(vertices[i].z-vertices[i+1].z);
						float blend1 = vertices[i+2].z/(vertices[i+2].z-vertices[i+1].z);
						
						Vector3 v0 = vertices[i].GetLerp(vertices[i+1], blend0);
						Vector3 v1 = vertices[i+2].GetLerp(vertices[i+1], blend1);
						Vector3 n0 = normals[i].GetLerp(normals[i+1], blend0);
						Vector3 n1 = normals[i+2].GetLerp(normals[i+1], blend1);
						
						tempVertices.push_back(vertices[i]);
						tempVertices.push_back(v0);
						tempVertices.push_back(vertices[i+2]);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(v1);
						tempVertices.push_back(vertices[i+2]);
						
						tempNormals.push_back(normals[i]);
						tempNormals.push_back(n0);
						tempNormals.push_back(normals[i+2]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(n1);
						tempNormals.push_back(normals[i+2]);
					}
					else
					{
						float blend0 = vertices[i+1].z/(vertices[i+1].z-vertices[i+2].z);
						float blend1 = vertices[i].z/(vertices[i].z-vertices[i+2].z);
						
						Vector3 v0 = vertices[i+1].GetLerp(vertices[i+2], blend0);
						Vector3 v1 = vertices[i].GetLerp(vertices[i+2], blend1);
						Vector3 n0 = normals[i+1].GetLerp(normals[i+2], blend0);
						Vector3 n1 = normals[i].GetLerp(normals[i+2], blend1);
						
						tempVertices.push_back(vertices[i+1]);
						tempVertices.push_back(v0);
						tempVertices.push_back(vertices[i]);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(v1);
						tempVertices.push_back(vertices[i]);
						
						tempNormals.push_back(normals[i+1]);
						tempNormals.push_back(n0);
						tempNormals.push_back(normals[i]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(n1);
						tempNormals.push_back(normals[i]);
					}
				}
			}
			else
			{
				tempVertices.push_back(vertices[i]);
				tempVertices.push_back(vertices[i+1]);
				tempVertices.push_back(vertices[i+2]);
				
				tempNormals.push_back(normals[i]);
				tempNormals.push_back(normals[i+1]);
				tempNormals.push_back(normals[i+2]);
			}
			
			otherVertices.insert(otherVertices.end(), tempVertices.begin(), tempVertices.end());
			otherNormals.insert(otherNormals.end(), tempNormals.begin(), tempNormals.end());
		}
		
		vertices.clear();
		normals.clear();
		
		//Front
		for(int i = 0; i < otherVertices.size(); i += 3)
		{
			tempVertices.clear();
			tempNormals.clear();
			
			if(otherVertices[i].z > 1.0f || otherVertices[i+1].z > 1.0f || otherVertices[i+2].z > 1.0f)
			{
				if(otherVertices[i].z > 1.0f && otherVertices[i+1].z > 1.0f && otherVertices[i+2].z > 1.0f)
				{
					continue;
				}
				
				if(otherVertices[i].z > 1.0f && otherVertices[i+1].z > 1.0f)
				{
					float blend0 = (1.0f-otherVertices[i+2].z)/(otherVertices[i].z-otherVertices[i+2].z);
					float blend1 = (1.0f-otherVertices[i+2].z)/(otherVertices[i+1].z-otherVertices[i+2].z);
					
					Vector3 v0 = otherVertices[i+2].GetLerp(otherVertices[i], blend0);
					Vector3 v1 = otherVertices[i+2].GetLerp(otherVertices[i+1], blend1);
					Vector3 n0 = otherNormals[i+2].GetLerp(otherNormals[i], blend0);
					Vector3 n1 = otherNormals[i+2].GetLerp(otherNormals[i+1], blend1);
					
					tempVertices.push_back(v0);
					tempVertices.push_back(v1);
					tempVertices.push_back(otherVertices[i+2]);
					
					tempNormals.push_back(n0);
					tempNormals.push_back(n1);
					tempNormals.push_back(otherNormals[i+2]);
				}
				else if(otherVertices[i+1].z > 1.0f && otherVertices[i+2].z > 1.0f)
				{
					float blend0 = (1.0f-otherVertices[i].z)/(otherVertices[i+1].z-otherVertices[i].z);
					float blend1 = (1.0f-otherVertices[i].z)/(otherVertices[i+2].z-otherVertices[i].z);
					
					Vector3 v0 = otherVertices[i].GetLerp(otherVertices[i+1], blend0);
					Vector3 v1 = otherVertices[i].GetLerp(otherVertices[i+2], blend1);
					Vector3 n0 = otherNormals[i].GetLerp(otherNormals[i+1], blend0);
					Vector3 n1 = otherNormals[i].GetLerp(otherNormals[i+2], blend1);
					
					tempVertices.push_back(v0);
					tempVertices.push_back(v1);
					tempVertices.push_back(otherVertices[i]);
					
					tempNormals.push_back(n0);
					tempNormals.push_back(n1);
					tempNormals.push_back(otherNormals[i]);
				}
				else if(otherVertices[i].z > 1.0f && otherVertices[i+2].z > 1.0f)
				{
					float blend0 = (1.0f-otherVertices[i+1].z)/(otherVertices[i].z-otherVertices[i+1].z);
					float blend1 = (1.0f-otherVertices[i+1].z)/(otherVertices[i+2].z-otherVertices[i+1].z);
					
					Vector3 v0 = otherVertices[i+1].GetLerp(otherVertices[i], blend0);
					Vector3 v1 = otherVertices[i+1].GetLerp(otherVertices[i+2], blend1);
					Vector3 n0 = otherNormals[i+1].GetLerp(otherNormals[i], blend0);
					Vector3 n1 = otherNormals[i+1].GetLerp(otherNormals[i+2], blend1);
					
					tempVertices.push_back(v1);
					tempVertices.push_back(v0);
					tempVertices.push_back(otherVertices[i+1]);
					
					tempNormals.push_back(n1);
					tempNormals.push_back(n0);
					tempNormals.push_back(otherNormals[i+1]);
				}
				else
				{
					if(otherVertices[i].z > 1.0f)
					{
						float blend0 = (1.0-otherVertices[i+1].z)/(otherVertices[i].z-otherVertices[i+1].z);
						float blend1 = (1.0-otherVertices[i+2].z)/(otherVertices[i].z-otherVertices[i+2].z);
						
						Vector3 v0 = otherVertices[i+1].GetLerp(otherVertices[i], blend0);
						Vector3 v1 = otherVertices[i+2].GetLerp(otherVertices[i], blend1);
						Vector3 n0 = otherNormals[i+1].GetLerp(otherNormals[i], blend0);
						Vector3 n1 = otherNormals[i+2].GetLerp(otherNormals[i], blend1);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i+1]);
						tempVertices.push_back(otherVertices[i+2]);
						
						tempVertices.push_back(v1);
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i+2]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i+1]);
						tempNormals.push_back(otherNormals[i+2]);
						
						tempNormals.push_back(n1);
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i+2]);
					}
					else if(otherVertices[i+1].z > 1.0f)
					{
						float blend0 = (1.0-otherVertices[i].z)/(otherVertices[i+1].z-otherVertices[i].z);
						float blend1 = (1.0-otherVertices[i+2].z)/(otherVertices[i+1].z-otherVertices[i+2].z);
						
						Vector3 v0 = otherVertices[i].GetLerp(otherVertices[i+1], blend0);
						Vector3 v1 = otherVertices[i+2].GetLerp(otherVertices[i+1], blend1);
						Vector3 n0 = otherNormals[i].GetLerp(otherNormals[i+1], blend0);
						Vector3 n1 = otherNormals[i+2].GetLerp(otherNormals[i+1], blend1);
						
						tempVertices.push_back(otherVertices[i]);
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i+2]);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(v1);
						tempVertices.push_back(otherVertices[i+2]);
						
						tempNormals.push_back(otherNormals[i]);
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i+2]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(n1);
						tempNormals.push_back(otherNormals[i+2]);
					}
					else
					{
						float blend0 = (1.0-otherVertices[i].z)/(otherVertices[i+2].z-otherVertices[i].z);
						float blend1 = (1.0-otherVertices[i+1].z)/(otherVertices[i+2].z-otherVertices[i+1].z);
						
						Vector3 v0 = otherVertices[i].GetLerp(otherVertices[i+2], blend0);
						Vector3 v1 = otherVertices[i+1].GetLerp(otherVertices[i+2], blend1);
						Vector3 n0 = otherNormals[i].GetLerp(otherNormals[i+2], blend0);
						Vector3 n1 = otherNormals[i+1].GetLerp(otherNormals[i+2], blend1);
						
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i]);
						tempVertices.push_back(otherVertices[i+1]);
						
						tempVertices.push_back(v1);
						tempVertices.push_back(v0);
						tempVertices.push_back(otherVertices[i+1]);
						
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i]);
						tempNormals.push_back(otherNormals[i+1]);
						
						tempNormals.push_back(n1);
						tempNormals.push_back(n0);
						tempNormals.push_back(otherNormals[i+1]);
					}
				}
			}
			else
			{
				tempVertices.push_back(otherVertices[i]);
				tempVertices.push_back(otherVertices[i+1]);
				tempVertices.push_back(otherVertices[i+2]);
				
				tempNormals.push_back(otherNormals[i]);
				tempNormals.push_back(otherNormals[i+1]);
				tempNormals.push_back(otherNormals[i+2]);
			}
			
			vertices.insert(vertices.end(), tempVertices.begin(), tempVertices.end());
			normals.insert(normals.end(), tempNormals.begin(), tempNormals.end());
		}
		
		_mesh->SetVerticesCount(vertices.size());
		Mesh::Chunk chunk = _mesh->GetChunk();
		Mesh::ElementIterator<Vector3> verticesIterator = chunk.GetIterator<Vector3>(MeshFeature::Vertices);
		Mesh::ElementIterator<Vector3> normalsIterator = chunk.GetIterator<Vector3>(MeshFeature::Normals);
		Mesh::ElementIterator<Vector2> texcoordsIterator = chunk.GetIterator<Vector2>(MeshFeature::UVSet0);
		
		for(int i = 0; i < vertices.size(); i++)
		{
			*verticesIterator.Seek(i) = invprojection * vertices[i];
			*normalsIterator.Seek(i) = normals[i];
			*texcoordsIterator.Seek(i) = Vector2(vertices[i]);
		}
		
		chunk.CommitChanges();
		
		if(_tangents)
			_mesh->GenerateTangents();
	}
	
	void Decal::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		SceneNode::DidUpdate(changeSet);
		
		if(changeSet & ChangeSet::Position)
		{
			UpdateMesh();
		}
	}
	
	void Decal::Render(Renderer *renderer, Camera *camera)
	{
		SceneNode::Render(renderer, camera);
		
		_transform = Matrix();
		
		RenderingObject object;
		FillRenderingObject(object);
		
		object.mesh = _mesh;
		object.material = _material;
		object.transform = &_transform;

		renderer->RenderObject(object);
	}
	
	Hit Decal::CastRay(const Vector3 &position, const Vector3 &direction, Hit::HitMode mode)
	{
		Hit hit;
		
		if(!GetBoundingSphere().IntersectsRay(position, direction))
			return hit;
		
		hit.position = GetWorldPosition();
		hit.distance = hit.position.GetDistance(position);
		hit.node = this;
		
		return hit;
	}
}
