//
//  RNAsset.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ASSET_H__
#define __RAYNE_ASSET_H__

#include "RNBase.h"
#include "RNObject.h"

namespace RN
{
	class ResourceCoordinator;
	
	class Asset : public Object
	{
	public:
		friend class ResourceCoordinator;
		
		~Asset() override;
		
		void Serialize(Serializer *serializer) override;
		static Asset *Deserialize(Deserializer *deserializer);
		
		const std::string &GetName() const;
		
	protected:
		Asset();
		
	private:
		void SetName(const std::string &name);
		
		std::string _name;
		
		Signal<void(Asset *)> _signal;
		
		RNDeclareMeta(Asset, Object)
	};
}

#endif /* __RAYNE_ASSET_H__ */
