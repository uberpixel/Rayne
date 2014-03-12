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
	
	Decal::Decal()
	: _angle("angle", 180.0f, &Decal::GetAngle, &Decal::SetAngle)
	{
		Initialize();
	}
	
	Decal::Decal(Texture *texture)
	: _angle("angle", 180.0f, &Decal::GetAngle, &Decal::SetAngle)
	{
		Initialize();
		SetTexture(texture);
	}
	
	Decal::Decal(Texture *texture, const Vector3 &position)
	: _angle("angle", 180.0f, &Decal::GetAngle, &Decal::SetAngle)
	{
		Initialize();
		SetTexture(texture);
		SetPosition(position);
	}
	
	Decal::Decal(const Decal *other) :
		SceneNode(other),
		_angle("angle", 180.0f, &Decal::GetAngle, &Decal::SetAngle)
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
		_material->SetDepthWrite(false);
		_material->SetPolygonOffset(true);
		_material->SetPolygonOffsetFactor(-1.0f);
		_material->SetPolygonOffsetUnits(0.5f);
		
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
		
		Matrix projection = Matrix::WithProjectionOrthogonal(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f) * GetWorldTransform().GetInverse();
		
		std::vector<Vector3> vertices;
		std::vector<Vector3> normals;
		std::vector<Vector2> texcoords;
		
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
										vertices.push_back(v0);
										vertices.push_back(v1);
										vertices.push_back(v2);
										normals.push_back(normalTransform*(*normalsIterator.Seek(indices[i])));
										normals.push_back(normalTransform*(*normalsIterator.Seek(indices[i+1])));
										normals.push_back(normalTransform*(*normalsIterator.Seek(indices[i+1])));
										
										Vector4 proj0 = projection * Vector4(v0, 1.0);
										proj0 /= proj0.w;
										proj0 *= 0.5;
										proj0 += 0.5;
										texcoords.push_back(Vector2(proj0));
										
										Vector4 proj1 = projection * Vector4(v1, 1.0);
										proj1 /= proj1.w;
										proj1 *= 0.5;
										proj1 += 0.5;
										texcoords.push_back(Vector2(proj1));
										
										Vector4 proj2 = projection * Vector4(v2, 1.0);
										proj2 /= proj2.w;
										proj2 *= 0.5;
										proj2 += 0.5;
										texcoords.push_back(Vector2(proj2));
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
		
/*		std::vector<Vector3> finalVertices;
		
		for(int i = 0; i < vertices.size(); i += 3)
		{
			
		}*/
		
		_mesh->SetVerticesCount(vertices.size());
		Mesh::Chunk chunk = _mesh->GetChunk();
		Mesh::ElementIterator<Vector3> verticesIterator = chunk.GetIterator<Vector3>(MeshFeature::Vertices);
		Mesh::ElementIterator<Vector3> normalsIterator = chunk.GetIterator<Vector3>(MeshFeature::Normals);
		Mesh::ElementIterator<Vector2> texcoordsIterator = chunk.GetIterator<Vector2>(MeshFeature::UVSet0);
		
		for(int i = 0; i < vertices.size(); i++)
		{
			*verticesIterator.Seek(i) = vertices[i];
			*normalsIterator.Seek(i) = normals[i];
			*texcoordsIterator.Seek(i) = texcoords[i];
		}
		
		chunk.CommitChanges();
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
