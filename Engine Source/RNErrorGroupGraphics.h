//
//  RNErrorGroupGraphics.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ERRORGROUPGRAPHICS_H__
#define __RAYNE_ERRORGROUPGRAPHICS_H__

#include "RNPlatform.h"
#include "RNDefines.h"

namespace RN
{
	enum
	{
		kGraphicsGroupGeneric = 0x0,
		kGraphicsGroupOpenGL  = 0x1
	};
	
	enum
	{
		kGraphicsNoHardware = 0x1,
		kGraphicsContextFailed = 0x2,
		kGraphicsShaderTypeNotSupported = 0x3,
		kGraphicsShaderCompilingFailed = 0x4,
		kGraphicsShaderLinkingFailed = 0x5,
		kGraphicsShaderAlreadyLinked = 0x6
	};
	
	enum
	{
		kGraphicsFramebufferGenericError,
		kGraphicsFramebufferUndefined,
		kGraphicsFramebufferIncompleteAttachment,
		kGraphicsFramebufferIncompleteMissingAttachment,
		kGraphicsFramebufferIncompleteDrawBuffer,
		kGraphicsFramebufferUnsupported,
		kGraphicsFramebufferIncompleteMultisample,
		kGraphicsFramebufferIncompleteLayerTargets,
		kGraphicsFramebufferIncompleteDimensions
	};
}

#endif /* __RAYNE_ERRORGROUPGRAPHICS_H__ */
