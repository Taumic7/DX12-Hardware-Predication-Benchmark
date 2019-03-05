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

		ID3D12Resource*				predicateUploadResource = nullptr;
		ID3D12Resource*				predicateResource = nullptr;


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

	ID3D12RootSignature*		computeRootSignature = nullptr;
	ID3D12PipelineState*		computePSO			 = nullptr;

	ID3D12DescriptorHeap*		renderTargetsHeap = nullptr;
	ID3D12Resource1*			renderTargets[NUM_SWAP_BUFFERS] = {};
	UINT						renderTargetDescriptorSize = 0;

	ID3D12CommandQueue*			directQueue	= nullptr;
	ID3D12CommandAllocator*		directQueueAlloc = nullptr;
	ID3D12GraphicsCommandList3*	directList = nullptr;

	ID3D12CommandQueue*			computeQueue = nullptr;
	ID3D12CommandAllocator*		computeQueueAlloc = nullptr;
	ID3D12GraphicsCommandList3* computeList = nullptr;

	ID3D12CommandQueue*			copyQueue = nullptr;
	ID3D12CommandAllocator*		copyQueueAlloc = nullptr;
	ID3D12GraphicsCommandList3* copyList = nullptr;

	ID3D12Resource1*			logicBufferResource = nullptr;
	ID3D12Resource1*			logicUploadResource = nullptr;

	ID3DBlob*					vertexShader = nullptr;
	ID3DBlob*					geometryShader = nullptr;
	ID3DBlob*					pixelShader	 = nullptr;
	ID3DBlob*					computeShader = nullptr;

	ID3D12DescriptorHeap*		resourceHeap = nullptr;
	unsigned int				resourceHeapSize = 0;

	ID3D12Fence1* directFence = nullptr;
	HANDLE directEventHandle = nullptr;
	UINT64 directFenceValue = 0;

	ID3D12Fence1* computeFence = nullptr;
	HANDLE computeEventHandle = nullptr;
	UINT64 computeFenceValue = 0;

	ID3D12Fence1* copyFence = nullptr;
	HANDLE copyEventHandle = nullptr;
	UINT64 copyFenceValue = 0;

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
	void CreatePredicateBuffer(TestState* state);
	void CreateLogicBuffer();
	void Present();

	TestState* CreateTestState(int width, int height);
	void renderTest(TestState* state);



	void waitForDirectQueue();

	void waitForComputeQueue();

	void waitForCopyQueue();

	void SetResourceTransitionBarrier(ID3D12GraphicsCommandList * commandList, ID3D12Resource * resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter);
};