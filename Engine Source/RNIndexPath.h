//
//  RNIndexPath.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INDEXPATH_H__
#define __RAYNE_INDEXPATH_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class IndexPath : public Object
	{
	public:
		RNAPI IndexPath();
		RNAPI IndexPath(IndexPath *other);
		
		RNAPI void AddIndex(size_t index);
		
		RNAPI size_t GetLength() const;
		RNAPI size_t GetIndexAtPosition(size_t position) const;
		
	private:
		std::vector<size_t> _indices;
		
		RNDefineMetaWithTraits(IndexPath, Object, MetaClassTraitCronstructable, MetaClassTraitCopyable)
	};
}

#endif /* __RAYNE_INDEXPATH_H__ */
