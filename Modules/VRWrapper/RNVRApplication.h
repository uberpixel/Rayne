//
//  RNVRApplication.h
//  Rayne-VR
//
//  Copyright 2020 by SlinDev. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and corona.
//

#ifndef __RAYNE_VRAPPLICATION_H_
#define __RAYNE_VRAPPLICATION_H_

#include <Rayne.h>
#include "RNVRWindow.h"

//TODO: These should be defines somehow set by the corresponding vr module....
#if !RN_PLATFORM_MAC_OS && !RN_PLATFORM_IOS && !RN_PLATFORM_VISIONOS
    #include "RNOpenXRWindow.h"
#endif
#if RN_PLATFORM_VISIONOS
	#include "RNAppleXRWindow.h"
#endif

namespace RN
{
	class VRApplication : public Application
	{
	public:
		VRApplication();
		~VRApplication() override;
		
		virtual RendererDescriptor *GetPreferredRenderer() const override;
		virtual RenderingDevice *GetPreferredRenderingDevice(RendererDescriptor *descriptor, const Array *devices) const override;

		virtual void WillFinishLaunching(Kernel *kernel) override;
		virtual void DidFinishLaunching(Kernel *kernel) override;
		
		virtual void WillStep(float delta) override;
		
		VRWindow *GetVRWindow() const  { return _vrWindow; }
		
	protected:
		void SetupVR();

    private:
        VRWindow *_vrWindow;
		
		//RNDeclareMeta(VRApplication)
	};
}


#endif /* __RAYNE_VRAPPLICATION_H_ */
