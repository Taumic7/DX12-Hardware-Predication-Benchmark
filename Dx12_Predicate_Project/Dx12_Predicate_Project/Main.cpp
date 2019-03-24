#include "Renderer.h"

// D3D12 Predication Project - Spring 2019 - Marcus Holmberg, Michael Lindroth, Rikard Magnom
// D3D12 Timer class provided by Stefan Petersson


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// Initiate console window
	AllocConsole();
	_wfreopen(L"CONOUT$", L"w", stdout);
	Renderer r;
	// RENDER_TEST performs the testing scenarios and writes the results to file.
	// RENDER_GAME is the basic implementation using all queues to draw "snake".
	// Use WASD to move and numpad 0-9 to increase/decrease the number of quads
	r.Init(hInstance, 1200, 800, RENDER_GAME);
	//r.Init(hInstance, 1200, 800, RENDER_TEST);
	r.Run();

	return 0;
}