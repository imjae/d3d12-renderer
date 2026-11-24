#include "D3D12Renderer.h"

/*
해야할 일 정리
1. device 생성
2. commandQueue 생성
3. swapChain 생성
*/

bool D3D12Renderer::Initialize(HWND hwnd)
{
	if (IsWindow(hwnd) == FALSE)
	{
		return false;
	}

	const HRESULT deviceHr = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_device));

	if (FAILED(deviceHr)) return false;

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc {};
	// 화면을 지우거나 삼각형을 그리는 그래픽 명령을 제출할 수 있는 큐
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	commandQueueDesc.Priority = D3D12_COMMAND_LIST_FLAG_NONE;
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// 단일 GPU 노드 구성에서 쓰는 값. 노드가 여러개일 경우 비트로 이 큐가 사용할 노드를 지정할 수 있다.
	commandQueueDesc.NodeMask = 0;

	const HRESULT queueHr = m_device->CreateCommandQueue(
		&commandQueueDesc,
		IID_PPV_ARGS(&m_commandQueue)
	);

	if (FAILED(queueHr)) return false;

	ComPtr<IDXGIFactory4> factory;
	const HRESULT factoryHr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));

	if (FAILED(factoryHr)) return false;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Width = 0;
	swapChainDesc.Height = 0;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	ComPtr<IDXGISwapChain1> swapChain;
	const HRESULT swapChainHr = factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	);

	if (FAILED(swapChainHr)) return false;

	const HRESULT swapChainASHr = swapChain.As(&m_swapChain);
	if (FAILED(swapChainASHr)) return false;

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc {};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = BackBufferCount;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT rtvHeapHr = m_device->CreateDescriptorHeap(
		&rtvHeapDesc,
		IID_PPV_ARGS(&m_rtvHeap)
	);

	if (FAILED(rtvHeapHr)) return false;

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (int i = 0; i < BackBufferCount; i++)
	{
		HRESULT getBufferHr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i]));
		if (FAILED(getBufferHr)) return false;

		m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += m_rtvDescriptorSize;
	}

	HRESULT commandAllocatorHr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
	if (FAILED(commandAllocatorHr)) return false;

	HRESULT commandListHr = m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&m_commandList)
	);
	if (FAILED(commandListHr)) return false;

	HRESULT commandListCloseHr = m_commandList->Close();
	if (FAILED(commandListCloseHr)) return false;

	HRESULT createFenceHr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
	if (FAILED(createFenceHr)) return false;

	m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		return false;
	}

	return true;
}

bool D3D12Renderer::WaitForGPU()
{
	const UINT64 fenceValue = m_fenceValue + 1;
	if (FAILED(m_commandQueue->Signal(m_fence.Get(), fenceValue)))
	{
		return false;
	}

	m_fenceValue = fenceValue;

	if (m_fence->GetCompletedValue() >= fenceValue)
	{
		return true;
	}

	if (FAILED(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent)))
	{
		return false;
	}

	return WaitForSingleObject(m_fenceEvent, INFINITE) == WAIT_OBJECT_0;
}