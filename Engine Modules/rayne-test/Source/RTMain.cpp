//
//  RTMain.cpp
//  rayne-test
//
//  Created by Sidney Just on 28.02.13.
//  Copyright (c) 2013 Sidney Just. All rights reserved.
//

#include "RTMain.h"

namespace RT
{
	
}

extern "C" bool RNModuleConstructor(RN::ModuleExports *exports)
{
	exports->version = 1;
	printf("Hello world\n");
}
