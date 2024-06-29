//
//  RNEOS.h
//  Rayne-EOS
//
//  Copyright 2021 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_EOS_H_
#define __RAYNE_EOS_H_

#include <Rayne.h>

#if defined(RN_BUILD_EOS)
	#define EOSAPI RN_EXPORT
#else
	#define EOSAPI RN_IMPORT
#endif

namespace RN
{
	RN_OPTIONS(EOSResult, uint32,
		   Success,
		   NoConnection,
		   NotLoggedIn,
		   Other);
}

#endif /* __RAYNE_EOS_H_ */
