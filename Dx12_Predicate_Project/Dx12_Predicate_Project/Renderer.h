#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>

//#include <thread>
//#include <mutex>
//#include <condition_variable>

#pragma comment (lib, "d3d12.lib")
#pragma comment (lib, "DXGI.lib")
#pragma comment (lib, "d3dcompiler.lib")

using namespace std::chrono_literals;
constexpr std::chrono::nanoseconds timestep(200ms);

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
static bool gUsePredicate;

//#define CLOCK_NOW std::chrono::high_resolution_clock::now()

//struct WindowInputParams
//{
//	HINSTANCE hInstance;
//	int width;
//	int height;
//};


struct LogicBuffer
{
	int x;
	int y;
	int height;
};

enum DIRECTION
{
	UP,
	DOWN,
	LEFT,
	RIGHT
};

//static int gCurrentState;
//static bool gStateIsChanged;
//static bool gLogicIsUpdated;
//static LogicBuffer gLogicBuffer;
//static DIRECTION gDirection;

class Renderer
{
	struct WindowThreadParams
	{
		HINSTANCE hInstance;
		int width;
		int height;
		Renderer* instanceP;
		WindowThreadParams(HINSTANCE hInstance, int w, int h, Renderer* instanceP)
		{
			this->hInstance = hInstance;
			this->width = w;
			this->height = h;
			this->instanceP = instanceP;
		}
	};

	static DWORD WINAPI StaticWindowTheadStart(void* Param)
	{
		WindowThreadParams* params = (WindowThreadParams*)Param;
		return params->instanceP->HandleInputThread(params);
	}

	static DWORD WINAPI StaticLogicThreadStart(void* Param)
	{
		Renderer* instanceP = (Renderer*)Param;
		return instanceP->CopyLogicThread();
	}

public:
	Renderer();
	~Renderer();

	void Init(HINSTANCE hInstance, int width, int height);
	void Run();

	DWORD HandleInputThread(LPVOID threadParams);
	DWORD CopyLogicThread();

private:
	// Temporary solution?
	bool windowCreated = false;


	std::chrono::high_resolution_clock clock;
	float pointSize[3][2] = {};
	int pointWidth[3], pointHeight[3];
	unsigned int numberOfObjects[3] = { 0 };
	std::chrono::time_point<std::chrono::high_resolution_clock> last_moved;
	std::chrono::time_point<std::chrono::high_resolution_clock> time_start;

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

		LogicBuffer logicBuffer;

		float pointSize[2] = {};
		int pointWidth, pointHeight;
		unsigned int numberOfObjects = 0;
	};

	bool quit = false;
	int currentState;
	//bool stateIsChanged;
	//bool logicIsUpdated;
	DIRECTION currentDirection;


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

	void Move();

	TestState* CreateTestState(int width, int height);
	void renderTest(TestState* state);

	void UpdateLogicBuffer();

	void waitForDirectQueue();
	void waitForComputeQueue();
	void waitForCopyQueue();

	void SetResourceTransitionBarrier(ID3D12GraphicsCommandList * commandList, ID3D12Resource * resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter);
};
