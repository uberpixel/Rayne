//
//  RNIndexSet.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		IndexSet();
		IndexSet(IndexSet *other);
		
		void AddIndex(size_t index);
		void RemoveIndex(size_t index);
		
		bool ContainsIndex(size_t index);
		
		size_t GetFirstIndex() const;
		size_t GetLastIndex() const;
		size_t GetCount() const;
		size_t GetIndex(size_t index) const;
		
	private:
		std::vector<size_t> _sortedIndices;
		std::unordered_set<size_t> _indices;
		
		RNDefineMeta(IndexSet, Object)
	};
}

#endif /* __RAYNE_INDEXSET_H__ */
