//
//  RNODE.h
//  Rayne-ODE
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ODE_H_
#define __RAYNE_ODE_H_

#include <Rayne.h>

#if defined(RN_BUILD_ODE)
	#define ODEAPI RN_EXPORT
#else
	#define ODEAPI RN_IMPORT
#endif

#endif /* __RAYNE_ODE_H_ */
