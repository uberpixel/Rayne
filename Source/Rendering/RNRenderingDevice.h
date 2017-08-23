//
//  RNRenderingDevice.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERINGDEVICE_H_
#define __RAYNE_RENDERINGDEVICE_H_

#include "../Base/RNBase.h"
#include "../Objects/RNObject.h"

namespace RN
{
	class RenderingDevice : public Object
	{
	public:
		enum class Type
		{
			Discrete,
			Integrated,
			CPU,
			Virtual,
			External,
			Other
		};

		struct Descriptor
		{
			uint32 apiVersion;
			uint32 driverVersion;
			uint32 vendorID;
			Type type;
		};


		RNAPI ~RenderingDevice() override;
		RNAPI bool IsEqual(const Object *other) const override;

		RNAPI const String *GetDescription() const override;

		const String *GetName() const { return _name; }
		uint32 GetAPIVersion() const { return _apiVersion; }
		uint32 GetDriverVersion() const { return _driverVersion; }
		uint32 GetVendorID() const { return _vendorID; }
		Type GetType() const { return _type; }


	protected:
		RNAPI RenderingDevice(const String *name, const Descriptor &descriptor);

	private:

		String *_name;

		uint32 _apiVersion;
		uint32 _driverVersion;
		uint32 _vendorID;

		Type _type;

		__RNDeclareMetaInternal(RenderingDevice)
	};
}


#endif /* __RAYNE_RENDERINGDEVICE_H_ */
