//
//  RNObject.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef _RAYNE_OBJECT_H_
#define _RAYNE_OBJECT_H_

#include <atomic>
#include "RNBase.h"

namespace RN
{
	class Object
	{
	public:
		Object();
		virtual ~Object();

		void Retain();
		void Release();

	protected:
		virtual void WillDealloc();

	private:
		std::atomic<uint32> _refCount;
	};
}

#endif /* _ RAYNE_OBJECT_H_ */
