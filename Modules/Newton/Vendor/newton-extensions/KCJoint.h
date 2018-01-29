#include "Newton.h"
#include "dVector.h"
#include "dQuaternion.h"
#include "dMatrix.h"

class KCJoint
{
public:
   NewtonBody *m_body0;
   NewtonJoint *m_joint;

   int m_pickMode; // constrain orientation if 1
   dFloat m_maxLinearFriction;
   dFloat m_maxAngularFriction;
   dVector m_localHandle;
   dVector m_targetPosit;
   dQuaternion m_targetRot;


   KCJoint ()
   {
      m_body0 = 0;
      m_joint = 0;
      m_pickMode = 0;
      m_maxLinearFriction = 3500;
      m_maxAngularFriction = 1600;
      m_localHandle = dVector(0,0,0);
      m_targetPosit = dVector(0,20,0);
      m_targetRot = dQuaternion(0,1.0,0,0);
   }

   void SubmitConstraints (dFloat timestep, int threadIndex)
   {
      dFloat invTimestep = (timestep > 0.0f) ? 1.0f / timestep: 1.0f;

 /*     if (!(propsPhysics::flags & propsPhysics::HAND_JOINT)) return;
      m_maxLinearFriction = (propsPhysics::f[propsPhysics::handLinMaxF]);*/
      
      dVector v;
      dVector w;
      dVector cg;
      dMatrix matrix0;
      // calculate the position of the pivot point and the Jacobian direction vectors, in global space. 

      NewtonBodyGetOmega (m_body0, &w[0]);
      NewtonBodyGetVelocity (m_body0, &v[0]);
      NewtonBodyGetCentreOfMass (m_body0, &cg[0]);
      NewtonBodyGetMatrix (m_body0, &matrix0[0][0]);
   
      dVector p0 (matrix0.TransformVector (m_localHandle));

      dVector pointVeloc = v + w.CrossProduct(matrix0.RotateVector (m_localHandle - cg));
      dVector relPosit (m_targetPosit - p0);
      dVector relVeloc (relPosit.Scale (invTimestep) - pointVeloc);
      dVector relAccel (relVeloc.Scale (invTimestep * 0.3f)); 

      //////////////////
      //
      //  Modification: Use Gramm Schmidt Projection along error vector instead of body space
      //
      //////////////////

      dMatrix matrixCS;
      dVector alignTo = relPosit;
      if ((alignTo.DotProduct3(alignTo)) < 0.000001f) matrixCS = dGetIdentityMatrix();
      else matrixCS = dGrammSchmidt(alignTo);

      // Restrict the movement on the pivot point along all tree orthonormal direction
      NewtonUserJointAddLinearRow (m_joint, &p0[0], &m_targetPosit[0], &matrixCS.m_front[0]);
      NewtonUserJointSetRowAcceleration (m_joint, relAccel.DotProduct3(matrixCS.m_front));
      NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxLinearFriction);
      NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxLinearFriction);

      NewtonUserJointAddLinearRow (m_joint, &p0[0], &m_targetPosit[0], &matrixCS.m_up[0]);
      NewtonUserJointSetRowAcceleration (m_joint, relAccel.DotProduct3(matrixCS.m_up));
      NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxLinearFriction);
      NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxLinearFriction);

      NewtonUserJointAddLinearRow (m_joint, &p0[0], &m_targetPosit[0], &matrixCS.m_right[0]);
      NewtonUserJointSetRowAcceleration (m_joint, relAccel.DotProduct3(matrixCS.m_right));
      NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxLinearFriction);
      NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxLinearFriction);

      //////////////////
      //
      //////////////////

      if (m_pickMode) 
      {
         dFloat mag;
         dQuaternion rotation;

         NewtonBodyGetRotation (m_body0, &rotation.m_q0);

         if (m_targetRot.DotProduct (rotation) < 0.0f) {
            rotation.m_q0 *= -1.0f; 
            rotation.m_q1 *= -1.0f; 
            rotation.m_q2 *= -1.0f; 
            rotation.m_q3 *= -1.0f; 
         }

         dVector relOmega (rotation.CalcAverageOmega (m_targetRot, invTimestep).Scale(0.3) - w);
         mag = relOmega.DotProduct3(relOmega);
         if (mag > 1.0e-6f) 
         {
            dFloat relAlpha;
            dFloat relSpeed;
            dVector pin (relOmega.Scale (1.0f / mag));
            dMatrix basis (dGrammSchmidt (pin));    
            relSpeed = dSqrt (relOmega.DotProduct3(relOmega));
            relAlpha = relSpeed * invTimestep;

            NewtonUserJointAddAngularRow (m_joint, 0.0f, &basis.m_front[0]);
            NewtonUserJointSetRowAcceleration (m_joint, relAlpha);
            NewtonUserJointSetRowStiffness (m_joint, 0.4f);
            NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
            NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

            NewtonUserJointAddAngularRow (m_joint, 0.0f, &basis.m_up[0]);
            NewtonUserJointSetRowAcceleration (m_joint, 0.0f);
            NewtonUserJointSetRowStiffness (m_joint, 0.4f);
            NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
            NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

            NewtonUserJointAddAngularRow (m_joint, 0.0f, &basis.m_right[0]);
            NewtonUserJointSetRowAcceleration (m_joint, 0.0f);
            NewtonUserJointSetRowStiffness (m_joint, 0.4f);
            NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
            NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);
         } 
         else 
         {
            dVector relAlpha = w.Scale (-invTimestep);
            NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_front[0]);
            NewtonUserJointSetRowAcceleration (m_joint, relAlpha.DotProduct3(matrix0.m_front));
            NewtonUserJointSetRowStiffness (m_joint, 0.4f);
            NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
            NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

            NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_up[0]);
            NewtonUserJointSetRowAcceleration (m_joint, relAlpha.DotProduct3(matrix0.m_up));
            NewtonUserJointSetRowStiffness (m_joint, 0.4f);
            NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
            NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);

            NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_right[0]);
            NewtonUserJointSetRowAcceleration (m_joint, relAlpha.DotProduct3(matrix0.m_right));
            NewtonUserJointSetRowStiffness (m_joint, 0.4f);
            NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction);
            NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction);
         }

      } 
      else 
      {
         // this is the single handle pick mode, add some angular friction

         dFloat const frictionFactor = 0.005f;

         dVector relAlpha = w.Scale (-invTimestep);

         NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_front[0]);
         NewtonUserJointSetRowAcceleration (m_joint, relAlpha.DotProduct3(matrix0.m_front));
         NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction * frictionFactor);
         NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction * frictionFactor);

         NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_up[0]);
         NewtonUserJointSetRowAcceleration (m_joint, relAlpha.DotProduct3(matrix0.m_up));
         NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction * frictionFactor);
         NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction * frictionFactor);

         NewtonUserJointAddAngularRow (m_joint, 0.0f, &matrix0.m_right[0]);
         NewtonUserJointSetRowAcceleration (m_joint, relAlpha.DotProduct3(matrix0.m_right));
         NewtonUserJointSetRowMinimumFriction (m_joint, -m_maxAngularFriction * frictionFactor);
         NewtonUserJointSetRowMaximumFriction (m_joint,  m_maxAngularFriction * frictionFactor);
      }
   }

   static void KCJointCallback(const NewtonJoint* joint, float timestep, int threadIndex)
   {
	   KCJoint* custom = (KCJoint*)NewtonJointGetUserData(joint);
	   custom->SubmitConstraints(timestep, threadIndex);
   }
};
