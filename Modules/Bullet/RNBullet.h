//
//  RNBullet.h
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BULLET_H_
#define __RAYNE_BULLET_H_

#include <Rayne.h>

#if defined(RN_BUILD_BULLET)
	#define BTAPI RN_EXPORT
#else
	#define BTAPI RN_IMPORT
#endif

#endif /* __RAYNE_BULLET_H_ */
