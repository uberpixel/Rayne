//
//  RNKernel.h
//  Rayne
//
//  Copyright 2012 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_KERNEL_H__
#define __RAYNE_KERNEL_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class Kernel : public Object, public Singleton<Kernel>
	{
	public:
		Kernel();
		virtual ~Kernel();
		
		static Kernel *SharedInstance();
	};
}

#endif /* __RAYNE_KERNEL_H__ */
