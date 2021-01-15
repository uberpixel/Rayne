//
//  RNUIConfig.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_UICONFIG_H_
#define __RAYNE_UICONFIG_H_

#include <Rayne.h>

#if defined(RN_BUILD_UI)
#define UIAPI RN_EXPORT
#else
#define UIAPI RN_IMPORT
#endif


#endif /* __RAYNE_UICONFIG_H_ */
