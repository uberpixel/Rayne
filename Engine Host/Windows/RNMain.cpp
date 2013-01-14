#include <Windows.h>
#include <RNKernel.h>
#include <RNWorld.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	RN::Kernel *kernel = new RN::Kernel();
	RN::World *world =  new RN::World(kernel);

	/*DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD dwStyle = WS_OVERLAPPEDWINDOW;

	RECT windowRect;

	windowRect.left = 0;
	windowRect.right = 1024;
	windowRect.top = 0;
	windowRect.bottom = 768;

	AdjustWindowRectEx(&windowRect, dwStyle, false, dwExStyle);
	MessageBoxA(nullptr, "Hello fucking World", "Test", MB_OK | MB_ICONINFORMATION);

	HWND hWnd = CreateWindowEx(0, (LPCWSTR)"RNWindowClass", (LPCWSTR)"Rayne", dwStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, 0, 0, hInstance, 0);

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);*/

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
