#include <Windows.h>
#include <RNKernel.h>
#include <RNWorld.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	RN::Kernel *kernel;
	RN::World *world;

	try
	{
		kernel = new RN::Kernel();
		world = new RN::World(kernel);
	}
	catch(RN::ErrorException e)
	{
		MessageBoxA(0, "It looks like your hardware is unsupported. Please make sure that your hardware supports OpenGL 3.2+ and that your drivers are up to date!", "Error", MB_OK | MB_ICONWARNING);
		return -1;
	}

	while(1)
	{
		MSG	message;
		while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}

		kernel->Update();
	}

	return 0;
}
