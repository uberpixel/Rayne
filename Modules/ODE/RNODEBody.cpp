//
//  RNODEBody.cpp
//  Rayne-ODE
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNODEBody.h"
#include "RNODEWorld.h"
#include "RNODEInternals.h"

#include <ode/ode.h>
#include "../../../Games/Swords/Builds/Oculus/Rayne/Modules/ODE/include/RNODEShape.h"

namespace RN
{
	RNDefineMeta(ODEBody, ODECollisionObject)
	
	ODEBody::ODEBody(ODEShape *shape, float mass) :
		_shape(shape->Retain()), _body(nullptr), _mass(mass)
	{
		dMass odeMass;
		if(shape->IsKindOfClass(ODESphereShape::GetMetaClass()))
		{
			ODESphereShape *sphereShape = shape->Downcast<ODESphereShape>();
			_geometry = dCreateSphere(ODEWorld::GetSharedInstance()->GetODESpace(), sphereShape->GetRadius());

			if(_mass > k::EpsilonFloat)
			{
				dMassSetSphereTotal(&odeMass, _mass, sphereShape->GetRadius());
			}
		}
		else if(shape->IsKindOfClass(ODEBoxShape::GetMetaClass()))
		{
			ODEBoxShape *boxShape = shape->Downcast<ODEBoxShape>();
			_geometry = dCreateBox(ODEWorld::GetSharedInstance()->GetODESpace(), boxShape->GetHalfExtends().x*2.0f, boxShape->GetHalfExtends().y*2.0f, boxShape->GetHalfExtends().z*2.0f);

			if(_mass > k::EpsilonFloat)
			{
				dMassSetBoxTotal(&odeMass, _mass, boxShape->GetHalfExtends().x*2.0f, boxShape->GetHalfExtends().y*2.0f, boxShape->GetHalfExtends().z*2.0f);
			}
		}
		else if(shape->IsKindOfClass(ODEStaticPlaneShape::GetMetaClass()))
		{
			ODEStaticPlaneShape *planeShape = shape->Downcast<ODEStaticPlaneShape>();
			_geometry = dCreatePlane(ODEWorld::GetSharedInstance()->GetODESpace(), planeShape->GetPlane().x, planeShape->GetPlane().y, planeShape->GetPlane().z, planeShape->GetPlane().w);
			_mass = 0.0f;
		}
		else if(shape->IsKindOfClass(ODETriangleMeshShape::GetMetaClass()))
		{
			ODETriangleMeshShape *triangleMeshShape = shape->Downcast<ODETriangleMeshShape>();
			dxTriMeshData *triangleMeshData = dGeomTriMeshDataCreate(); //TODO: NEEDS TO BE DELETED SOMEWHERE!
			dGeomTriMeshDataBuildSingle(triangleMeshData, triangleMeshShape->GetVertices(), sizeof(Vector3), triangleMeshShape->GetVertexCount(), triangleMeshShape->GetIndices(), triangleMeshShape->GetIndexCount(), sizeof(uint32) * 3);// , triangleMeshShape->GetNormals());

			_geometry = dCreateTriMesh(ODEWorld::GetSharedInstance()->GetODESpace(), triangleMeshData, nullptr, nullptr, nullptr);

			if(_mass > k::EpsilonFloat)
			{
				dMassSetTrimeshTotal(&odeMass, _mass, _geometry);
			}
		}
		else if(shape->IsKindOfClass(ODECompoundShape::GetMetaClass()))
		{
			RN_ASSERT(false, "ODECompoundShape is not supported yet.");
		}

		if(_mass > k::EpsilonFloat)
		{
			_body = dBodyCreate(ODEWorld::GetSharedInstance()->GetODEWorld());
//			dMassTranslate(&odeMass, -odeMass.c[0], -odeMass.c[1], -odeMass.c[2]);
			dBodySetMass(_body, &odeMass);
			dGeomSetBody(_geometry, _body);

			dBodySetFiniteRotationMode(_body, 1);
		}
	}
		
	ODEBody::~ODEBody()
	{
		_shape->Release();
	}
	
		
	ODEBody *ODEBody::WithShape(ODEShape *shape, float mass)
	{
		ODEBody *body = new ODEBody(shape, mass);
		return body->Autorelease();
	}
	
/*	btCollisionObject *BulletRigidBody::GetBulletCollisionObject() const
	{
		return _rigidBody;
	}*/
		
	void ODEBody::SetMass(float mass)
	{
		
	}
	void ODEBody::SetLinearVelocity(const Vector3 &velocity)
	{
//		_rigidBody->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
	}
	void ODEBody::SetAngularVelocity(const Vector3 &velocity)
	{
//		_rigidBody->setAngularVelocity(btVector3(velocity.x, velocity.y, velocity.z));
	}
	void ODEBody::SetCCDMotionThreshold(float threshold)
	{
//		_rigidBody->setCcdMotionThreshold(threshold);
	}
	void ODEBody::SetCCDSweptSphereRadius(float radius)
	{
//		_rigidBody->setCcdSweptSphereRadius(radius);
	}
		
	void ODEBody::SetGravity(const RN::Vector3 &gravity)
	{
//		_rigidBody->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
	}
		
	void ODEBody::SetDamping(float linear, float angular)
	{
//		_rigidBody->setDamping(linear, angular);
	}

	void ODEBody::SetAllowDeactivation(bool canDeactivate)
	{
//		_rigidBody->forceActivationState(canDeactivate?ACTIVE_TAG:DISABLE_DEACTIVATION);
	}
		
	Vector3 ODEBody::GetLinearVelocity() const
	{
		if(!_body)
			return Vector3();

		const dReal *velocity = dBodyGetLinearVel(_body);
		return Vector3(velocity[0], velocity[1], velocity[2]);
	}
	Vector3 ODEBody::GetAngularVelocity() const
	{
		if(!_body)
			return Vector3();

		const dReal *velocity = dBodyGetAngularVel(_body);
		return Vector3(velocity[0], velocity[1], velocity[2]);
	}
		
		
	Vector3 ODEBody::GetCenterOfMass() const
	{
//		const btVector3& center = _rigidBody->getCenterOfMassPosition();
		return Vector3();// center.x(), center.y(), center.z());
	}
	Matrix ODEBody::GetCenterOfMassTransform() const
	{
/*		const btTransform& transform = _rigidBody->getCenterOfMassTransform();
			
		btQuaternion rotation = transform.getRotation();
		btVector3 position    = transform.getOrigin();*/
			
		Matrix matrix;
			
//		matrix.Translate(Vector3(position.x(), position.y(), position.z()));
//		matrix.Rotate(Quaternion(rotation.x(), rotation.y(), rotation.z(), rotation.w()));
			
		return matrix;
	}
		
		
	void ODEBody::ApplyForce(const Vector3 &force)
	{
//		_rigidBody->applyCentralForce(btVector3(force.x, force.y, force.z));
	}
	void ODEBody::ApplyForce(const Vector3 &force, const Vector3 &origin)
	{
//		_rigidBody->applyForce(btVector3(force.x, force.y, force.z), btVector3(origin.x, origin.y, origin.z));
	}
	void ODEBody::ClearForces()
	{
//		_rigidBody->clearForces();
	}
		
	void ODEBody::ApplyTorque(const Vector3 &torque)
	{
//		_rigidBody->applyTorque(btVector3(torque.x, torque.y, torque.z));
	}
	void ODEBody::ApplyTorqueImpulse(const Vector3 &torque)
	{
//		_rigidBody->applyTorqueImpulse(btVector3(torque.x, torque.y, torque.z));
	}
	void ODEBody::ApplyImpulse(const Vector3 &impulse)
	{
//		_rigidBody->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
	}
	void ODEBody::ApplyImpulse(const Vector3 &impulse, const Vector3 &origin)
	{
//		_rigidBody->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(origin.x, origin.y, origin.z));
	}
		
	void ODEBody::Update(float delta)
	{
		ODECollisionObject::Update(delta);

		if(_body)
		{
			const dReal *odeRotation = dBodyGetQuaternion(_body);
			Quaternion rotation(odeRotation[1], odeRotation[2], odeRotation[3], odeRotation[0]);
			SetWorldRotation(rotation);

			const dReal *odePosition = dBodyGetPosition(_body);
			Vector3 position(odePosition[0], odePosition[1], odePosition[2]);
			SetWorldPosition(position + rotation.GetRotatedVector(offset));
		}
	}
		
	void ODEBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		ODECollisionObject::DidUpdate(changeSet);
			
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			if(_body)
			{
				Quaternion rotation = GetWorldRotation();
				Vector3 position = GetWorldPosition() - rotation.GetRotatedVector(offset);
				dBodySetPosition(_body, position.x, position.y, position.z);

				dQuaternion odeRotatio = { rotation.w, rotation.x, rotation.y, rotation.z };
				dBodySetQuaternion(_body, odeRotatio);
			}
		}
	}
/*	void ODEBody::UpdateFromMaterial(BulletMaterial *material)
	{
		_rigidBody->setFriction(material->GetFriction());
		_rigidBody->setRollingFriction(material->GetRollingFriction());
		_rigidBody->setSpinningFriction(material->GetSpinningFriction());
		_rigidBody->setRestitution(material->GetRestitution());
		_rigidBody->setDamping(material->GetLinearDamping(), material->GetAngularDamping());
	}
		*/
		
	void ODEBody::InsertIntoWorld(ODEWorld *world)
	{
		ODECollisionObject::InsertIntoWorld(world);

		if(_body)
		{
			Quaternion rotation = GetWorldRotation();
			Vector3 position = GetWorldPosition() - rotation.GetRotatedVector(offset);
			dBodySetPosition(_body, position.x, position.y, position.z);

			dQuaternion odeRotatio = { rotation.w, rotation.x, rotation.y, rotation.z };
			dBodySetQuaternion(_body, odeRotatio);
		}

/*		_motionState->SetSceneNode(this);
			
		{
			btTransform transform;
				
			_motionState->getWorldTransform(transform);
			_rigidBody->setCenterOfMassTransform(transform);
		}
			
		auto bulletWorld = world->GetBulletDynamicsWorld();
		bulletWorld->addRigidBody(_rigidBody, GetCollisionFilter(), GetCollisionFilterMask());*/
	}
		
	void ODEBody::RemoveFromWorld(ODEWorld *world)
	{
		ODECollisionObject::RemoveFromWorld(world);
			
/*		auto bulletWorld = world->GetBulletDynamicsWorld();
		bulletWorld->removeRigidBody(_rigidBody);*/
	}

	void ODEBody::SetPositionOffset(RN::Vector3 offset)
	{
		ODECollisionObject::SetPositionOffset(offset);
//		_motionState->SetPositionOffset(offset);
	}

	void ODEBody::AccelerateToTarget(const Vector3 &position, const Quaternion &rotation, float delta)
	{
		RN_ASSERT(_body, "AccelerateToTarget can only be used on dynamic bodies (mass > 0)!");

		//Linear velocity
		RN::Vector3 speed = position - GetWorldPosition();
		speed /= delta;

		//Angular velocity
		RN::Quaternion startRotation = GetWorldRotation();
		if(rotation.GetDotProduct(startRotation) > 0.0f)
			startRotation = startRotation.GetConjugated();
		RN::Quaternion rotationSpeed = rotation*startRotation;
		RN::Vector4 axisAngleSpeed = rotationSpeed.GetAxisAngle();
		if(axisAngleSpeed.w > 180.0f)
			axisAngleSpeed.w -= 360.0f;
		RN::Vector3 angularVelocity(axisAngleSpeed.x, axisAngleSpeed.y, axisAngleSpeed.z);
		angularVelocity *= axisAngleSpeed.w *M_PI;
		angularVelocity /= 180.0f;
		angularVelocity /= delta;

/*		RN::Vector3 linearForce = speed - GetLinearVelocity();
		linearForce /= delta;
		linearForce *= _mass;
		linearForce *= 10.0f;

		RN::Vector3 angularForce = angularVelocity - GetAngularVelocity();
		angularForce *= delta;
		angularForce *= _mass;
		angularForce *= 10.0f;*/

/*		if(linearForce.GetLength() > 500.0f)
			linearForce.Normalize(500.0f);*/

/*		if(angularForce.GetLength() > 15000.0f)
			angularForce.Normalize(15000.0f);*/

		dBodySetLinearVel(_body, speed.x, speed.y, speed.z);
		dBodySetAngularVel(_body, angularVelocity.x, angularVelocity.y, angularVelocity.z);

/*		dBodyAddForce(_body, linearForce.x, linearForce.y, linearForce.z);
		dBodyAddTorque(_body, angularForce.x, angularForce.y, angularForce.z);*/
	}
}
