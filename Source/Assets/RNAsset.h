//
//  RNAsset.h
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ASSET_H_
#define __RAYNE_ASSET_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"

namespace RN
{
	class AssetManager;
	class Asset : public Object
	{
	public:
		friend class AssetManager;

		RNAPI Asset();

		RNAPI const String *GetDescription() const override;
		RNAPI const String *GetName() const;

	protected:
		RNAPI void Dealloc() override;

	private:
		void __AwakeWithCoordinator(AssetManager *coordinator, String *name, MetaClass *meta);

		AssetManager *_coordinator;
		String *_name;
		MetaClass *_meta;

		__RNDeclareMetaInternal(Asset)
	};
}


#endif /* __RAYNE_ASSET_H_ */
