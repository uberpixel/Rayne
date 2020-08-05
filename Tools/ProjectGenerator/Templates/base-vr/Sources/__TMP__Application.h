//
//  __TMP__Application.h
//  __TMP_APPLICATION_NAME__
//
//  Copyright __TMP_YEAR__ by __TMP_COMPANY__. All rights reserved.
//

#ifndef ____TMP___APPLICATION_H_
#define ____TMP___APPLICATION_H_

#include <Rayne.h>
#include "RNVRApplication.h"

namespace __TMP__
{
	class Application : public RN::VRApplication
	{
	public:
		Application();
		~Application();
		
		void WillFinishLaunching(RN::Kernel *kernel) override;
		void DidFinishLaunching(RN::Kernel *kernel) override;
	};
}


#endif /* ____TMP___APPLICATION_H_ */
