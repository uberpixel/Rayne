//
//  RNWeakStorage.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//


#ifndef __RAYNE_WEAKSTORAGE_H_
#define __RAYNE_WEAKSTORAGE_H_

#include "../Base/RNBase.h"
#include "RNArray.h"

namespace RN
{
	class WeakStorage : public Object
	{
	public:
		RNAPI WeakStorage();
		RNAPI ~WeakStorage() override;

		RNAPI void AddObject(Object *object);
		RNAPI void RemoveObject(Object *object);
		RNAPI void RemoveObjectIdenticalTo(Object *object);
		RNAPI void RemoveAllObjects();

		RNAPI Array *GetAllObjects() const;

		void Enumerate(const std::function<void (Object *, bool &)>& callback) const
		{
			bool stop = false;

			for(Entry &entry : _storage)
			{
				Object *object = entry.object.Load();
				if(!object)
					continue;

				callback(object, stop);

				if(stop)
					break;
			}
		}

		template<class T>
		void Enumerate(const std::function<void (T *, bool &)>& callback) const
		{
			bool stop = false;

			for(Entry &entry : _storage)
			{
				Object *object = entry.object.Load();
				if(!object)
					continue;

				callback(static_cast<T *>(object), stop);

				if(stop)
					break;
			}
		}

	private:
		struct Entry
		{
			Entry(Object *tobject) :
				object(tobject)
			{}

			WeakRef<Object> object;
		};

		void CleanZombies() const;

		mutable std::vector<Entry> _storage; // Mutable because some const functions need to remove zombies... Only a tad ugly

		RNDeclareMeta(WeakStorage)
	};
}


#endif /* __RAYNE_WEAKSTORAGE_H_ */
