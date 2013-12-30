//
//  RNIndexSet.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_INDEXSET_H__
#define __RAYNE_INDEXSET_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class IndexSet : public Object
	{
	public:
		RNAPI IndexSet();
		RNAPI IndexSet(size_t index);
		RNAPI IndexSet(IndexSet *other);
		
		RNAPI void AddIndex(size_t index);
		RNAPI void RemoveIndex(size_t index);
		
		RNAPI bool ContainsIndex(size_t index);
		
		RNAPI size_t GetFirstIndex() const;
		RNAPI size_t GetLastIndex() const;
		RNAPI size_t GetCount() const;
		RNAPI size_t GetIndex(size_t index) const;
		
		const std::vector<size_t>& GetIndices() const { return _sortedIndices; }
		
	private:
		std::vector<size_t> _sortedIndices;
		std::unordered_set<size_t> _indices;
		
		RNDefineMetaWithTraits(IndexSet, Object, MetaClassTraitCronstructable, MetaClassTraitCopyable)
	};
}

#endif /* __RAYNE_INDEXSET_H__ */
