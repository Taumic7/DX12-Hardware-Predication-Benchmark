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
	Renderer(HINSTANCE hInstance, int width, int height);
	~Renderer();

	void Run();

private:

	bool decrease = true;

	float pointSize[3][2] = {};
	int pointWidth[3], pointHeight[3];
	unsigned int numberOfObjects[3] = { 0 };

	float clearColor[4] = { 0.2f,0.2f,0.2f,1.0f };
	UINT currentRenderTarget = 0;

	struct Vertex
	{
		float x, y; // Position
	};

	struct TestState
	{
		ID3D12Resource1*			vertexBufferResource = nullptr;
		D3D12_VERTEX_BUFFER_VIEW	vertexBufferView = {};

		float pointSize[2] = {};
		int pointWidth, pointHeight;
		unsigned int numberOfObjects = 0;
	};


	TestState* states[3] = {};

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

	//ID3D12Resource1*			vertexBufferResource[3] = {};
	//D3D12_VERTEX_BUFFER_VIEW	vertexBufferView[3] = {};

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
	void CreateFence();
	void CreateRenderTargets();
	void CreateViewportAndScissorRect(LONG width, LONG height);
	void CreateShaders();
	void CreatePSO();
	void CreateRootSignature();
	void CreateVertexBufferAndVertexData(TestState* state);
	void Present();

	TestState* CreateTestState(int width, int height);
	void renderTest(TestState* state);



	void waitForGPU();

	void SetResourceTransitionBarrier(ID3D12GraphicsCommandList * commandList, ID3D12Resource * resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter);

};
