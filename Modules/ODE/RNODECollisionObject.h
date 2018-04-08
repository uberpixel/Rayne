//
//  RNODECollisionObject.h
//  Rayne-ODE
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ODECOLLISIONOBJECT_H_
#define __RAYNE_ODECOLLISIONOBJECT_H_

#include "RNODE.h"
#include "RNODEWorld.h"

namespace RN
{
	class ODECollisionObject : public SceneNodeAttachment
	{
	public:
		friend class ODEWorld;
			
		ODEAPI ODECollisionObject();
		ODEAPI ~ODECollisionObject() override;
			
		ODEAPI void SetCollisionFilter(short int filter);
		ODEAPI void SetCollisionFilterMask(short int mask);
//		ODEAPI void SetMaterial(BulletMaterial *material);
		ODEAPI void SetContactCallback(std::function<void(ODECollisionObject *, const ODEContactInfo&)> &&callback);
		ODEAPI void SetSimulationCallback(std::function<void()> &&callback);
		ODEAPI virtual void SetPositionOffset(RN::Vector3 offset);
			
		short int GetCollisionFilter() const { return _collisionFilter; }
		short int GetCollisionFilterMask() const { return _collisionFilterMask; }
//		BulletMaterial *GetMaterial() const { return _material; }
			
//		ODEAPI virtual btCollisionObject *GetODECollisionObject() const = 0;
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		//void DidAddToParent() override;
		//void WillRemoveFromParent() override;
		
		ODEWorld *GetOwner() const { return _owner; }
		void ReInsertIntoWorld();
//		virtual void UpdateFromMaterial(BulletMaterial *material) = 0;
		virtual void InsertIntoWorld(ODEWorld *world);
		virtual void RemoveFromWorld(ODEWorld *world);
		Vector3 offset;
			
	private:
		Connection *_connection;
		ODEWorld *_owner;
//		BulletMaterial *_material;
			
		std::function<void(ODECollisionObject *, const ODEContactInfo&)> _contactCallback;
		std::function<void()> _simulationStepCallback;
			
		short int _collisionFilter;
		short int _collisionFilterMask;
			
		RNDeclareMetaAPI(ODECollisionObject, ODEAPI)
	};
}

#endif /* defined(__RAYNE_ODECOLLISIONOBJECT_H_) */
