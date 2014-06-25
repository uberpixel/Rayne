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
		
		RNAPI ~Asset() override;
		
		RNAPI void Serialize(Serializer *serializer) override;
		RNAPI const std::string &GetName() const;
		
		RNAPI void MarkChanged();
		RNAPI void MarkUnchanged();
		RNAPI bool IsMarkedChanged() const { return _dirty; }
		
		RNAPI static Asset *Deserialize(Deserializer *deserializer);
		
	protected:
		RNAPI Asset();
		RNAPI virtual void Unfault(Deserializer *deserializer);
		
	private:
		void WakeUpFromResourceCoordinator(const std::string &name, Dictionary *settings);
		
		std::string _name;
		Dictionary *_settings;
		bool _dirty;
		
		Signal<void(Asset *)> _signal;
		
		RNDeclareMeta(Asset)
	};
	
	RNObjectClass(Asset)
}

#endif /* __RAYNE_ASSET_H__ */
