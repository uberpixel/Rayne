//
//  ${NAME}.h
//  Test Game
//
//  Copyright ${YEAR} by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//
#set ($NormalizedName = $NAME.substring(2))
#set ($NormalizedNameCaps = $NAME.substring(2).toUpperCase())

#[[#ifndef]]# __TEST_GAME_${NormalizedNameCaps}_H_
#[[#define]]# __TEST_GAME_${NormalizedNameCaps}_H_

#[[#include]]# <Rayne.h>

namespace TG
{
	class ${NormalizedName}
	{
	public:
	};
}


#[[#endif]]# /* __TEST_GAME_${NormalizedNameCaps}_H_ */
