#include <RNKernel.h>
#include <RNFile.h>
#include <RNMutex.h>

using namespace std;

int main(int argc, char* argv[])
{
	RN::Kernel *kernel;
	
	for(int i=1; i<argc; i++)
	{
		if(strcmp(argv[i], "-r") == 0 && i < argc - 1)
		{
			char *path = argv[++ i];
			RN::File::AddSearchPath(path);
		}
	}

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
