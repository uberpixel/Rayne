#include <RNKernel.h>
#include <RNWorld.h>
#include <RNMutex.h>

using namespace std;

int main()
{
	RN::Kernel *kernel;
	RN::World *world;

	try
	{
	kernel = new RN::Kernel();
	}
	catch(RN::ErrorException e)
	{
		puts("It looks like your hardware is unsupported. Please make sure that your hardware supports OpenGL 3.2+ and that your drivers are up to date!");
		return -1;
	}

	while(kernel->Tick())
	{}

	kernel->Release();

	return 0;
}
