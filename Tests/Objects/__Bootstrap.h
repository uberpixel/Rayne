//
// Created by Sidney Just on 14/10/15.
//

#ifndef RAYNEALL_BOOTSTRAP_H
#define RAYNEALL_BOOTSTRAP_H

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


#endif //RAYNEALL_BOOTSTRAP_H
