//
//  __Bootstrap.h
//  Rayne Unit Tests
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef _RAYNE_UNITTESTS_BOOTSTRAP_H_
#define _RAYNE_UNITTESTS_BOOTSTRAP_H_

#include <gtest/gtest.h>
#include <Rayne.h>

namespace RN
{
	extern Kernel *__BootstrapKernel(Application *app);
	extern void __TearDownKernel(Kernel *kernel);
}

namespace __Bootstrap
{
	class Application : public RN::Application
	{

	};
}

class KernelFixture : public ::testing::Test
{
public:
	KernelFixture() :
		Test()
	{}

protected:
	void SetUp() override
	{
		_app = new __Bootstrap::Application();
		_kernel = RN::__BootstrapKernel(_app);
	}
	void TearDown() override
	{
		__TearDownKernel(_kernel);
		delete _app;
	}

private:
	__Bootstrap::Application *_app;
	RN::Kernel *_kernel;
};


#endif /* _RAYNE_UNITTESTS_BOOTSTRAP_H_ */
