#ifndef __RAYNE_SARELAY_H_
#define __RAYNE_SARELAY_H_

#include <Rayne.h>
#include <RelayClient.h>

#if defined(RN_BUILD_SARELAY)
	#define SARELAYAPI RN_EXPORT
#else
	#define SARELAYAPI RN_IMPORT
#endif

#endif
