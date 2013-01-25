//
//  RNBullet.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BULLET_H__
#define __RAYNE_BULLET_H__

class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btSequentialImpulseConstraintSolver;
class btDynamicsWorld;
class btCollisionShape;
class btTriangleMesh;
class btRigidBody;
class btTransform;

#ifndef BT_MOTIONSTATE_H
#define BT_MOTIONSTATE_H
class	btMotionState
{
public:
	
	virtual ~btMotionState()
	{
		
	}
	
	virtual void	getWorldTransform(btTransform& worldTrans ) const =0;
	
	//Bullet only calls the update of worldtransform for active objects
	virtual void	setWorldTransform(const btTransform& worldTrans)=0;
	
	
};
#endif /* BT_MOTIONSTATE_H */

#endif /* __RAYNE_BULLET_H__ */
