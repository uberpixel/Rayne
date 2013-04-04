//
//  RBPhysicsMaterial.cpp
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

#include "RBPhysicsMaterial.h"

namespace RN
{
	namespace bullet
	{
		RNDeclareMeta(PhysicsMaterial)
		
		PhysicsMaterial::PhysicsMaterial()
		{
			_linearDamping  = 0.0f;
			_angularDamping = 0.0f;
			
			_friction    = 0.0f;
			_restitution = 0.0f;
		}
		
		void PhysicsMaterial::AddListener(void *handle, const std::function<void (PhysicsMaterial *)>& callback)
		{
			_listener.insert(std::map<void *, std::function<void (PhysicsMaterial *)>>::value_type(handle, callback));
		}
		
		void PhysicsMaterial::RemoveListener(void *handle)
		{
			auto iterator = _listener.find(handle);
			if(iterator != _listener.end())
			{
				_listener.erase(handle);
			}
		}
		
		void PhysicsMaterial::NotifyListener()
		{
			for(auto i=_listener.begin(); i!=_listener.end(); i++)
			{
				i->second(this);
			}
		}
		
		void PhysicsMaterial::SetLinearDamping(float damping)
		{
			_linearDamping = damping;
			NotifyListener();
		}
		
		void PhysicsMaterial::SetAngularDamping(float damping)
		{
			_angularDamping = damping;
			NotifyListener();
		}
		
		void PhysicsMaterial::SetFriction(float friction)
		{
			_friction = friction;
			NotifyListener();
		}
		
		void PhysicsMaterial::SetRestitution(float restitution)
		{
			_restitution = restitution;
			NotifyListener();
		}
	}
}
