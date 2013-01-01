//
//  RNKernel.cpp
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNKernel.h"

namespace RN
{
	static Kernel *sharedKernel = 0;
	
	Kernel::Kernel()
	{
	}
	
	Kernel::~Kernel()
	{
		if(sharedKernel == this)
			sharedKernel = 0;
	}
	
	Kernel *Kernel::SharedInstance()
	{
		if(!sharedKernel)
			sharedKernel = new Kernel();
		
		return sharedKernel;
	}
}
