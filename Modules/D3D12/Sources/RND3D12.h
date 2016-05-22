//
//  RND3D12.h
//  Rayne-D3D12
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_D3D12_H__
#define __RAYNE_D3D12_H__

#include "d3dx12.h"
#include <Rayne.h>

#if defined(RN_BUILD_D3D12)
	#define D3DAPI RN_EXPORT
#else
	#define D3DAPI RN_IMPORT
#endif

#endif /* __RAYNE_D3D12_H__ */
