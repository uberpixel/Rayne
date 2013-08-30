//
//  ROVRCamera.h
//  rayne-oculus
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

#ifndef __rayne_oculus__ROVRCamera__
#define __rayne_oculus__ROVRCamera__

#include <Rayne.h>
#include "ROVRSystem.h"

namespace RN
{
	namespace oculus
	{
		class Camera : public RN::SceneNode
		{
		public:
			Camera();
			~Camera();
			
			void Update(float delta);
			
		private:
			void WindowFrameChanged(RN::Message *message);
			
			float _projshift;
			float _eyeshift;
			float _riftfov;
			float _scalefac;
			
			RN::Camera *_left;
			RN::Camera *_right;
			
			RN::Camera *_left_riftstage;
			RN::Camera *_right_riftstage;
			
			RN::RenderStorage *_storage;
			
			RN::Vector3 _eyecenteroffset;
			RN::Quaternion _headrotation;
		};
	}
}

#endif /* defined(__rayne_oculus__ROVRCamera__) */
