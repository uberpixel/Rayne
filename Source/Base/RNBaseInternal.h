//
// Created by Sidney Just on 02/05/15.
//

#ifndef __RAYNE_BASEINTERNAL_H__
#define __RAYNE_BASEINTERNAL_H__

#include "RNBase.h"
#include "../Debug/RNLogger.h"

#if RN_PLATFORM_MAC_OS
	#import <Cocoa/Cocoa.h>
	#import <Foundation/Foundation.h>
	#import <IOKit/graphics/IOGraphicsLib.h>
#endif


#if RN_ENABLE_VTUNE

#include <ittnotify.h>

namespace RN
{
	extern __itt_domain *VTuneDomain;
}

#endif

#endif /* __RAYNE_BASEINTERNAL_H__ */
