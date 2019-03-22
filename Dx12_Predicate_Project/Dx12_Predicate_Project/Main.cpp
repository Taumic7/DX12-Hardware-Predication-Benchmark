#include "Renderer.h"



int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// Initiate console window
	AllocConsole();
	_wfreopen(L"CONOUT$", L"w", stdout);
	Renderer r;

	r.Init(hInstance, 1200, 800, RENDER_TEST);
	//r.Init(hInstance, 1200, 800, RENDER_TEST);
	r.Run();

	return 0;
}