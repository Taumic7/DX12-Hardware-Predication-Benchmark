#include "Renderer.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	AllocConsole();
	_wfreopen(L"CONOUT$", L"w", stdout);

	Renderer r(hInstance,1200,800);
	r.Run();

	return 0;
}