//
//  ${NAME}.h
//  Rayne
//
//  Copyright ${YEAR} by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//
#set ($NormalizedName = $NAME.substring(2))
#set ($NormalizedNameCaps = $NAME.substring(2).toUpperCase())

#[[#ifndef]]# __RAYNE_${NormalizedNameCaps}_H_
#[[#define]]# __RAYNE_${NormalizedNameCaps}_H_

#[[#include]]# "../Base/RNBase.h"

namespace RN
{
	class ${NormalizedName}
	{
	public:
	};
}


#[[#endif]]# /* __RAYNE_${NormalizedNameCaps}_H_ */
