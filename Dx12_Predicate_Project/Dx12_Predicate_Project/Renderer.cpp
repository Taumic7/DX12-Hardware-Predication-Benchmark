#include "Renderer.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}



Renderer::Renderer(HINSTANCE hInstance, int width, int height)
{
	try
	{
		CreateHWND(hInstance, width, height);
		CreateDevice();
		CreateCMDInterface();
		CreateSwapChain();
		CreateFence();
		CreateRenderTargets();
		CreateViewportAndScissorRect(width, height);
		CreateShaders();
		CreateRootSignature();
		CreatePSO();
		CreateLogicBuffer();

		const int set = 10;
		const float ratio = (float)height / width;

		for (int i = 0; i < 3; i++)
		{
			states[i] = CreateTestState(set * (i+1), set * ratio * (i+1));
		}
	}
	catch (const char* e)
	{
		std::cout << e << std::endl;
	}
};

Renderer::~Renderer()
{
}

void Renderer::Run()
{
	MSG msg = { 0 };

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			this->waitForDirectQueue();
			this->waitForComputeQueue();

			renderTest(states[2]);
		}
	}
}

void Renderer::CreateHWND(HINSTANCE hInstance, LONG width, LONG height)
{
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	//wcex.cbSize = 0;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.lpszClassName = L"Window";
	if (!RegisterClassEx(&wcex))
	{
		throw "Could not create window.";
	}

	RECT rc = { 0, 0, width, height };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false);

	window = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		L"Window",
		L"Hello Window!",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	ShowWindow(window, SW_SHOWDEFAULT);
}

void Renderer::CreateDevice()
{
	IDXGIFactory6*	factory = nullptr;
	IDXGIAdapter1*	adapter = nullptr;
	ID3D12Debug* debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
	CreateDXGIFactory(IID_PPV_ARGS(&factory));
	for (UINT adapterIndex = 0;; ++adapterIndex)
	{
		adapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(adapterIndex, &adapter))
		{
			break; //No more adapters to enumerate.
		}
		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device4), nullptr)))
		{
			break;
		}
		SafeRelease(&adapter);
	}
	if (adapter)
	{
		//Create the actual device.
		if (FAILED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device))))
		{
			SafeRelease(&adapter);
			SafeRelease(&factory);
			throw "Could not create Device";
		}

		SafeRelease(&adapter);
	}
	SafeRelease(&factory);
}

void Renderer::CreateCMDInterface()
{
	//Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC cqd = {};
	device->CreateCommandQueue(&cqd, IID_PPV_ARGS(&directQueue));

	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&directQueueAlloc));

	// Create Command list
	device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		directQueueAlloc,
		nullptr,
		IID_PPV_ARGS(&directList));


	//Command lists are created in the recording state. Since there is nothing to
	//record right now and the main loop expects it to be closed, we close it.
	directList->Close();
	// -----------------------------------------------------------------
	D3D12_COMMAND_QUEUE_DESC cqd2 = {};
	cqd2.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	device->CreateCommandQueue(&cqd2, IID_PPV_ARGS(&computeQueue));

	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&computeQueueAlloc));

	device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		computeQueueAlloc,
		nullptr,
		IID_PPV_ARGS(&computeList));

	computeList->Close();

	//--------------------------------------------------------------------

	D3D12_COMMAND_QUEUE_DESC cqd3 = {};
	cqd3.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	device->CreateCommandQueue(&cqd3, IID_PPV_ARGS(&copyQueue));

	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&copyQueueAlloc));

	device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COPY,
		copyQueueAlloc,
		nullptr,
		IID_PPV_ARGS(&copyList));

	copyList->Close();

}

void Renderer::CreateSwapChain()
{
	IDXGIFactory5*	factory = nullptr;
	CreateDXGIFactory(IID_PPV_ARGS(&factory));

	//Create swap chain.
	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = 0;
	scDesc.Height = 0;
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.Stereo = FALSE;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = NUM_SWAP_BUFFERS;
	scDesc.Scaling = DXGI_SCALING_NONE;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Flags = 0;
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	IDXGISwapChain1* swapChain1 = nullptr;
	if (FAILED(factory->CreateSwapChainForHwnd(
		directQueue,
		window,
		&scDesc,
		nullptr,
		nullptr,
		&swapChain1)))
	{
		SafeRelease(&swapChain1);
		SafeRelease(&factory);
		throw "Could not create swapchain";
	}

	if (FAILED(swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain))))
	{
		SafeRelease(&swapChain1);
		SafeRelease(&factory);
		throw "Could not query swapchain";

	}

	SafeRelease(&swapChain1);
	SafeRelease(&factory);
}

void Renderer::CreateFence()
{
	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&directFence));
	directFenceValue = 1;
	//Create an event handle to use for GPU synchronization.
	directEventHandle = CreateEvent(0, false, false, 0);

	//-----------------------------------------------------------------------------


	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&computeFence));
	computeFenceValue = 1;
	//Create an event handle to use for GPU synchronization.
	computeEventHandle = CreateEvent(0, false, false, 0);

	//-----------------------------------------------------------------------------

	device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&copyFence));
	copyFenceValue = 1;
	//Create an event handle to use for GPU synchronization.
	copyEventHandle = CreateEvent(0, false, false, 0);
}

void Renderer::CreateRenderTargets()
{
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.NumDescriptors = NUM_SWAP_BUFFERS;
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	if (FAILED(device->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&renderTargetsHeap))))
	{
		throw "Could not create RenderTargetHeap";
	}

	// Create render target resources
	renderTargetDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetsHeap->GetCPUDescriptorHandleForHeapStart();

	//One RTV for each frame.
	for (UINT n = 0; n < NUM_SWAP_BUFFERS; n++)
	{
		swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n]));
		device->CreateRenderTargetView(renderTargets[n], nullptr, cdh);
		cdh.ptr += renderTargetDescriptorSize;
	}
}

void Renderer::CreateViewportAndScissorRect(LONG width, LONG height)
{
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.Width = (float)width;
	viewport.Height = (float)height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	scissorRect.left = (long)0;
	scissorRect.right = (long)width;
	scissorRect.top = (long)0;
	scissorRect.bottom = (long)height;
}

void Renderer::CreateShaders()
{
	////// Shader Compiles //////
	D3DCompileFromFile(
		L"VertexShader.hlsl", // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"VS_main",		// entry point
		"vs_5_0",		// shader model (target)
		0,				// shader compile options			// here DEBUGGING OPTIONS
		0,				// effect compile options
		&vertexShader,	// double pointer to ID3DBlob		
		nullptr			// pointer for Error Blob messages.
						// how to use the Error blob, see here
						// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
	);

	D3DCompileFromFile(
		L"GeometryShader.hlsl", // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"GS_main",		// entry point
		"gs_5_0",		// shader model (target)
		0,				// shader compile options			// here DEBUGGING OPTIONS
		0,				// effect compile options
		&geometryShader,	// double pointer to ID3DBlob		
		nullptr			// pointer for Error Blob messages.
						// how to use the Error blob, see here
						// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
	);

	D3DCompileFromFile(
		L"PixelShader.hlsl", // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"PS_main",		// entry point
		"ps_5_0",		// shader model (target)
		0,				// shader compile options			// here DEBUGGING OPTIONS
		0,				// effect compile options
		&pixelShader,		// double pointer to ID3DBlob		
		nullptr			// pointer for Error Blob messages.
						// how to use the Error blob, see here
						// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
	);

	D3DCompileFromFile(
		L"ComputeShader.hlsl", // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"CS_main",		// entry point
		"cs_5_0",		// shader model (target)
		0,				// shader compile options			// here DEBUGGING OPTIONS
		0,				// effect compile options
		&computeShader,		// double pointer to ID3DBlob		
		nullptr			// pointer for Error Blob messages.
						// how to use the Error blob, see here
						// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
	);
}

void Renderer::CreatePSO()
{
	////// Input Layout //////
	D3D12_INPUT_ELEMENT_DESC inputElementDesc[] = {
		{ "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = inputElementDesc;
	inputLayoutDesc.NumElements = ARRAYSIZE(inputElementDesc);

	////// Pipline State //////
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};

	//Specify pipeline stages:
	gpsd.pRootSignature = rootSignature;
	gpsd.InputLayout = inputLayoutDesc;
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	gpsd.VS.pShaderBytecode = reinterpret_cast<void*>(vertexShader->GetBufferPointer());
	gpsd.VS.BytecodeLength = vertexShader->GetBufferSize();
	gpsd.GS.pShaderBytecode = reinterpret_cast<void*>(geometryShader->GetBufferPointer());
	gpsd.GS.BytecodeLength = geometryShader->GetBufferSize();
	gpsd.PS.pShaderBytecode = reinterpret_cast<void*>(pixelShader->GetBufferPointer());
	gpsd.PS.BytecodeLength = pixelShader->GetBufferSize();

	//Specify render target and depthstencil usage.
	gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsd.NumRenderTargets = 1;

	gpsd.SampleDesc.Count = 1;
	gpsd.SampleMask = UINT_MAX;

	//Specify rasterizer behaviour.
	gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	//Specify blend descriptions.
	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc = {
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsd.BlendState.RenderTarget[i] = defaultRTdesc;

	if (FAILED(device->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&PSO))))
	{
		throw "Could not create Graphics PSO";
	}

	//--------------------------------------------------------
	//// Retrieve the cached PSO blob
	//ID3DBlob* cachedBlob;
	//if (FAILED(this->computePSO->GetCachedBlob(&cachedBlob)))
	//{
	//	throw "Failed to get cached computePSO blob";
	//}

	//D3D12_CACHED_PIPELINE_STATE cachedPSO = {};
	//cachedPSO.pCachedBlob = reinterpret_cast<void*>(cachedBlob->GetBufferPointer());
	//cachedPSO.CachedBlobSizeInBytes = cachedBlob->GetBufferSize();

	D3D12_COMPUTE_PIPELINE_STATE_DESC cPsd = {};
	cPsd.pRootSignature = this->computeRootSignature;
	cPsd.CS.pShaderBytecode = reinterpret_cast<void*>(this->computeShader->GetBufferPointer());
	cPsd.CS.BytecodeLength = this->computeShader->GetBufferSize();
	cPsd.NodeMask = 0; // Multi gpu stuff, dont touch
	cPsd.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	if (FAILED(this->device->CreateComputePipelineState(&cPsd, IID_PPV_ARGS(&this->computePSO))))
	{
		throw "Failed to create compute PSO";
	}
}

void Renderer::CreateRootSignature()
{
	D3D12_ROOT_PARAMETER  rootParam[1];
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam[0].Constants.Num32BitValues = 2;
	rootParam[0].Constants.RegisterSpace = 0;
	rootParam[0].Constants.ShaderRegister = 0;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;

	D3D12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = ARRAYSIZE(rootParam);
	rsDesc.pParameters = rootParam;
	rsDesc.NumStaticSamplers = 0;
	rsDesc.pStaticSamplers = nullptr;

	ID3DBlob* sBlob;
	D3D12SerializeRootSignature(
		&rsDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&sBlob,
		nullptr);

	if (FAILED(device->CreateRootSignature(
		0,
		sBlob->GetBufferPointer(),
		sBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature))))
	{
		throw "Could not create direct queue root signature";
	}

	//----------------------------------------------------------------

	// Predicate buffer UAV
	D3D12_ROOT_PARAMETER  rootParam2[2];
	rootParam2[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParam2[0].Descriptor.ShaderRegister = 0;
	rootParam2[0].Descriptor.RegisterSpace = 0;
	rootParam2[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// compute shader cbv, filled by copy queue
	rootParam2[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam2[1].Descriptor.ShaderRegister = 0;
	rootParam2[1].Descriptor.RegisterSpace = 0;
	rootParam2[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rsDesc2;
	rsDesc2.NumParameters = ARRAYSIZE(rootParam2);
	rsDesc2.pParameters = rootParam2;
	rsDesc2.NumStaticSamplers = 0;
	rsDesc2.pStaticSamplers = nullptr;
	rsDesc2.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	ID3DBlob* sBlob2;
	HRESULT hr = D3D12SerializeRootSignature(
		&rsDesc2,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&sBlob2,
		nullptr);

	if (FAILED(device->CreateRootSignature(
		0,
		sBlob2->GetBufferPointer(),
		sBlob2->GetBufferSize(),
		IID_PPV_ARGS(&computeRootSignature))))
	{
		throw "Could not create compute queue root signature";
	}
}

void Renderer::CreateVertexBufferAndVertexData(TestState* state)
{
	ID3D12Resource* uploadBuffer;

	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = D3D12_HEAP_TYPE_UPLOAD;
	hp.CreationNodeMask = 1;
	hp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = state->pointWidth * state->pointHeight * sizeof(Vertex);
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	rd.Format = DXGI_FORMAT_UNKNOWN;

	if (FAILED(device->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer))))
	{
		throw "Could not create Upload Vertex Buffer";
	}

	float wFloat = state->pointWidth, hFloat = state->pointHeight;
	double widthIncrement = (float)2.0f / ((float)wFloat);
	double heightIncrement = (float)2.0f / (float)hFloat;

	Vertex* pointArr = new Vertex[state->pointWidth * state->pointHeight];
	float widthPos = -1 + widthIncrement / 2.f;
	unsigned int counter = 0;

	for (int i = 0; i < state->pointWidth; i++)
	{
		float heightPos = -1 + heightIncrement / 2.f;
		for (int j = 0; j < state->pointHeight; j++)
		{
			pointArr[counter++] = { widthPos,heightPos };
			heightPos += heightIncrement;
		}
		widthPos += widthIncrement;
	}

	void* dataBegin = nullptr;
	D3D12_RANGE range = { 0,0 };

	uploadBuffer->Map(0, &range, &dataBegin);
	memcpy(dataBegin, pointArr, state->pointWidth * state->pointHeight * sizeof(Vertex));
	uploadBuffer->Unmap(0, nullptr);


	// Create DEFAULT buffer
	hp = {};
	hp.Type = D3D12_HEAP_TYPE_DEFAULT;
	hp.CreationNodeMask = 1;
	hp.VisibleNodeMask = 1;

	rd = {};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = state->pointWidth * state->pointHeight * sizeof(Vertex);
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	if (FAILED(device->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&state->vertexBufferResource))))
	{
		throw "Could not create Default Vertex Buffer";
	}

	waitForDirectQueue();
	directList->Reset(directQueueAlloc, nullptr);

	// Copy from upload buffer to default buffer
	directList->CopyBufferRegion(state->vertexBufferResource, 0, uploadBuffer, 0, state->pointWidth * state->pointHeight * sizeof(Vertex));

	SetResourceTransitionBarrier(
		directList,
		state->vertexBufferResource,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	// Initialise vertex buffer view
	state->vertexBufferView.BufferLocation = state->vertexBufferResource->GetGPUVirtualAddress();
	state->vertexBufferView.StrideInBytes = sizeof(Vertex);
	state->vertexBufferView.SizeInBytes = state->pointWidth * state->pointHeight * sizeof(Vertex);

	directList->Close();

	ID3D12CommandList* listsToExecute[] = { directList };
	this->directQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	waitForDirectQueue();

	delete[]pointArr;
	SafeRelease(&uploadBuffer);
}

void Renderer::CreatePredicateBuffer(TestState * state)
{
	UINT64 memSize = state->pointWidth * state->pointHeight * 8;

	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = D3D12_HEAP_TYPE_UPLOAD;
	hp.CreationNodeMask = 1;
	hp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = memSize;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	if (FAILED(device->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&state->predicateUploadResource))))
	{
		throw "Could not create Upload Buffer for Predicate";
	}


	hp = {};
	hp.Type = D3D12_HEAP_TYPE_DEFAULT;
	hp.CreationNodeMask = 1;
	hp.VisibleNodeMask = 1;

	rd = {};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = memSize;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	if (FAILED(device->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&state->predicateResource))))
	{
		throw "Could not create Predicate Buffer";
	}

	waitForDirectQueue();

	void* dataBegin = nullptr;
	D3D12_RANGE range = { 0,0 };

	state->predicateUploadResource->Map(0, &range, &dataBegin);
	memset(dataBegin, (char)1, memSize);
	//memset(dataBegin, (char)1, memSize/2.f);
	state->predicateUploadResource->Unmap(0, nullptr);

	waitForDirectQueue();
	directList->Reset(directQueueAlloc, nullptr);

	directList->CopyBufferRegion(state->predicateResource, 0, state->predicateUploadResource, 0, memSize);

	SetResourceTransitionBarrier(
		directList,
		state->predicateResource,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	directList->Close();

	ID3D12CommandList* listsToExecute[] = { directList };
	this->directQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);
}

void Renderer::CreateLogicBuffer()
{
	const UINT64 memSize = sizeof(int);

	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = D3D12_HEAP_TYPE_UPLOAD;
	hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask = 1;
	hp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = memSize;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	if (FAILED(device->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&logicUploadResource))))
	{
		throw "Could not create Upload Buffer for Predicate";
	}


	hp = {};
	hp.Type = D3D12_HEAP_TYPE_DEFAULT;
	hp.CreationNodeMask = 1;
	hp.VisibleNodeMask = 1;

	rd = {};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = memSize;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	if (FAILED(device->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&logicBufferResource))))
	{
		throw "Could not create Predicate Buffer";
	}

	waitForCopyQueue();

	void* dataBegin = nullptr;
	D3D12_RANGE range = { 0,0 };

	logicUploadResource->Map(0, &range, &dataBegin);
	int data = 12;
	memcpy(dataBegin, &data, memSize);
	logicUploadResource->Unmap(0, nullptr);

	copyList->Reset(copyQueueAlloc, nullptr);

	//copyList->CopyResource(logicBufferResource, logicUploadResource);						// TO COPY ALL
	copyList->CopyBufferRegion(logicBufferResource, 0, logicUploadResource, 0, memSize);	// TO COPT PART

	/*SetResourceTransitionBarrier(
		copyList,
		logicBufferResource,
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);*/

	copyList->Close();

	ID3D12CommandList* listsToExecute[] = { copyList };
	this->copyQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);
}

void Renderer::Present()
{
	DXGI_PRESENT_PARAMETERS pp = {};
	swapChain->Present1(0, 0, &pp);
	currentRenderTarget = ++currentRenderTarget % NUM_SWAP_BUFFERS;
}

Renderer::TestState* Renderer::CreateTestState(int width, int height)
{
	TestState* newState = new TestState;
	newState->numberOfObjects = width * height;
	newState->pointWidth = width;
	newState->pointHeight = height;

	newState->pointSize[0] = (float)1.f / width;
	newState->pointSize[1] = (float)1.f / height;

	CreateVertexBufferAndVertexData(newState);
	CreatePredicateBuffer(newState);

	return newState;
}

void Renderer::renderTest(TestState* state)
{

	directQueueAlloc->Reset();
	directList->Reset(directQueueAlloc, this->PSO);

	computeQueueAlloc->Reset();
	computeList->Reset(computeQueueAlloc, this->computePSO);


	directList->SetGraphicsRootSignature(rootSignature);
	computeList->SetComputeRootSignature(this->computeRootSignature);

	//if (decrease)
	//{
	//	pointSize[1] -= 0.0001f;
	//	if (pointSize[1] <= 0.f)
	//	{
	//		decrease = false;
	//	}
	//}
	//else
	//{
	//	pointSize[1] += 0.0001f;
	//	if (pointSize[1] >= 0.3f)
	//	{
	//		decrease = true;
	//	}
	//}
	directList->SetGraphicsRoot32BitConstants(0, 2, state->pointSize, 0);
	computeList->SetComputeRootUnorderedAccessView(0, state->predicateResource->GetGPUVirtualAddress());
	computeList->SetComputeRootConstantBufferView(1, logicBufferResource->GetGPUVirtualAddress());

	//-----------------------------------------------

	computeList->Dispatch(1, 1, 1);

	//-----------------------------------------------



	directList->RSSetViewports(1, &viewport);
	directList->RSSetScissorRects(1, &scissorRect);
	directList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	directList->IASetVertexBuffers(0, 1, &state->vertexBufferView);

	SetResourceTransitionBarrier(directList,
		renderTargets[currentRenderTarget],
		D3D12_RESOURCE_STATE_PRESENT,		//state before
		D3D12_RESOURCE_STATE_RENDER_TARGET	//state after
	);

	D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	cdh.ptr += renderTargetDescriptorSize * currentRenderTarget;
	directList->OMSetRenderTargets(1, &cdh, true, nullptr);
	directList->ClearRenderTargetView(cdh, clearColor, 0, nullptr);

	for (int i = 0; i < state->numberOfObjects; i++)
	{
		directList->SetPredication(state->predicateResource, i * 8, D3D12_PREDICATION_OP_EQUAL_ZERO);
		directList->DrawInstanced(1, 1, i, 0);
	}
	SetResourceTransitionBarrier(directList,
		renderTargets[currentRenderTarget],
		D3D12_RESOURCE_STATE_RENDER_TARGET,	//state before
		D3D12_RESOURCE_STATE_PRESENT		//state after
	);
	directList->Close();
	computeList->Close();

	ID3D12CommandList* listsToExecute[] = { directList };
	this->directQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	ID3D12CommandList* listsToExecute2[] = { computeList };
	this->computeQueue->ExecuteCommandLists(ARRAYSIZE(listsToExecute2), listsToExecute2);
	
	Present();
}

void Renderer::waitForDirectQueue()
{
	//WAITING FOR EACH FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
//This is code implemented as such for simplicity. The cpu could for example be used
//for other tasks to prepare the next frame while the current one is being rendered.

//Signal and increment the fence value.
	const UINT64 fenceL = directFenceValue;
	directQueue->Signal(directFence, fenceL);
	directFenceValue++;

	//Wait until command queue is done.
	if (directFence->GetCompletedValue() < fenceL)
	{
		directFence->SetEventOnCompletion(fenceL, directEventHandle);
		WaitForSingleObject(directEventHandle, INFINITE);
	}
}

void Renderer::waitForComputeQueue()
{
	//WAITING FOR EACH FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
//This is code implemented as such for simplicity. The cpu could for example be used
//for other tasks to prepare the next frame while the current one is being rendered.

//Signal and increment the fence value.
	const UINT64 fenceL = directFenceValue;
	computeQueue->Signal(computeFence, fenceL);
	computeFenceValue++;

	//Wait until command queue is done.
	if (computeFence->GetCompletedValue() < fenceL)
	{
		computeFence->SetEventOnCompletion(fenceL, computeEventHandle);
		WaitForSingleObject(computeEventHandle, INFINITE);
	}
}

void Renderer::waitForCopyQueue()
{
	//WAITING FOR EACH FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
//This is code implemented as such for simplicity. The cpu could for example be used
//for other tasks to prepare the next frame while the current one is being rendered.

//Signal and increment the fence value.
	const UINT64 fenceL = copyFenceValue;
	copyQueue->Signal(copyFence, fenceL);
	copyFenceValue++;

	//Wait until command queue is done.
	if (copyFence->GetCompletedValue() < fenceL)
	{
		copyFence->SetEventOnCompletion(fenceL, copyEventHandle);
		WaitForSingleObject(copyEventHandle, INFINITE);
	}
}

void Renderer::SetResourceTransitionBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource,
	D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter)
{
	D3D12_RESOURCE_BARRIER barrierDesc = {};

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = resource;
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = StateBefore;
	barrierDesc.Transition.StateAfter = StateAfter;

	commandList->ResourceBarrier(1, &barrierDesc);
}
