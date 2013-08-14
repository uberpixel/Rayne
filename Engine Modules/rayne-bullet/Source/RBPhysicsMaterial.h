//
//  RBPhysicsMaterial.h
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

#ifndef __RBULLET_PHYSICSMATERIAL_H__
#define __RBULLET_PHYSICSMATERIAL_H__

#include <RNObject.h>
#include <RNArray.h>

namespace RN
{
	namespace bullet
	{
		class PhysicsMaterial : public Object
		{
		public:
			PhysicsMaterial();
			
			void AddListener(void *handle, const std::function<void (PhysicsMaterial *)>& callback);
			void RemoveListener(void *handle);
			
			void SetLinearDamping(float damping);
			void SetAngularDamping(float damping);
			void SetFriction(float friction);
			void SetRestitution(float restitution);
			
			float LinearDamping() const { return _linearDamping; }
			float AngularDamping() const { return _angularDamping; }
			float Friction() const { return _friction; }
			float Restitution() const { return _restitution; }
			
		private:
			void NotifyListener();
			std::map<void *, std::function<void (PhysicsMaterial *)>> _listener;
			
			float _linearDamping;
			float _angularDamping;
			
			float _friction;
			float _restitution;
			
			RNDefineMeta(PhysicsMaterial, Object)
		};
	}
}

#endif /* __RBULLET_PHYSICSMATERIAL_H__ */
