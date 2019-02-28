#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <Windows.h>
#include <iostream>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "DXGI.lib")
#pragma comment (lib, "d3dcompiler.lib")

template<class Interface>
inline void SafeRelease(
	Interface **ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();

		(*ppInterfaceToRelease) = NULL;
	}
}

const unsigned int NUM_SWAP_BUFFERS = 2; //Number of buffers

class Renderer
{
public:
	Renderer(HINSTANCE hInstance, LONG width, LONG height);
	~Renderer();

	void Run();

private:

	float clearColor[4] = { 0,0,0,0 };
	UINT currentRenderTarget = 0;

	struct Vertex
	{
		float x, y; // Position
	};


	HWND			window;
	D3D12_VIEWPORT	viewport = {};
	D3D12_RECT		scissorRect = {};

	ID3D12Device4*			device = nullptr;
	IDXGISwapChain4*		swapChain = nullptr;
	ID3D12RootSignature*	rootSignature = nullptr;
	ID3D12PipelineState*	PSO = nullptr;

	ID3D12DescriptorHeap*		renderTargetsHeap = nullptr;
	ID3D12Resource1*			renderTargets[NUM_SWAP_BUFFERS] = {};
	UINT						renderTargetDescriptorSize = 0;

	ID3D12Resource1*			vertexBufferResource = nullptr;
	D3D12_VERTEX_BUFFER_VIEW	vertexBufferView = {};

	ID3D12CommandQueue*			directQueue	= nullptr;
	ID3D12CommandAllocator*		directQueueAlloc = nullptr;
	ID3D12GraphicsCommandList3*			directList = nullptr;


	ID3DBlob*					vertexShader = nullptr;
	ID3DBlob*					geometryShader = nullptr;
	ID3DBlob*					pixelShader	 = nullptr;

	ID3D12DescriptorHeap*		resourceHeap = nullptr;
	unsigned int				resourceHeapSize = 0;

	ID3D12Fence1* fence = nullptr;
	HANDLE eventHandle = nullptr;
	UINT64 fenceValue = 0;

	void CreateHWND(HINSTANCE hInstance, LONG width, LONG height);
	void CreateDevice();
	void CreateCMDInterface();
	void CreateSwapChain();
	void CreateRenderTargets();
	void CreateViewportAndScissorRect(LONG width, LONG height);
	void CreateShaders();
	void CreatePSO();
	void CreateRootSignature();
	void CreateVertexBufferAndVertexData(int width, int height);
	void Present();


	void waitForGPU();

};
