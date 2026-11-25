#include "D3D12Renderer.h"
#include <d3dcompiler.h>
#include <cstring>


bool D3D12Renderer::Initialize(HWND hwnd)
{
	if (IsWindow(hwnd) == FALSE)
	{
		return false;
	}
	const HRESULT deviceHr = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_device)
	);

	if (FAILED(deviceHr)) return false;

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
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

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
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


	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;
	HRESULT readVSHr = D3DReadFileToBlob(L"x64\\Debug\\shaders_VSMain.cso", &vertexShader);
	if (FAILED(readVSHr)) return false;
	HRESULT readPSHr = D3DReadFileToBlob(L"x64\\Debug\\shaders_PSMain.cso", &pixelShader);
	if (FAILED(readPSHr)) return false;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> serializedRootSignature;
	ComPtr<ID3DBlob> serializationErrors;

	const HRESULT serializeHr = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSignature,
		&serializationErrors
	);

	if (FAILED(serializeHr)) return false;

	const HRESULT rootSignatureHr = m_device->CreateRootSignature(
		0,
		serializedRootSignature->GetBufferPointer(),
		serializedRootSignature->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature)
	);

	if (FAILED(rootSignatureHr)) return false;
	const D3D12_INPUT_ELEMENT_DESC inputElements[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, static_cast<UINT>(offsetof(Vertex, position)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(Vertex, color)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.InputLayout = { inputElements, static_cast<UINT>(_countof(inputElements)) };
	psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = swapChainDesc.Format;
	psoDesc.SampleDesc = swapChainDesc.SampleDesc;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;

	auto& blend = psoDesc.BlendState.RenderTarget[0];
	blend.BlendEnable = FALSE;
	blend.LogicOpEnable = FALSE;
	blend.SrcBlend = D3D12_BLEND_ONE;
	blend.DestBlend = D3D12_BLEND_ZERO;
	blend.BlendOp = D3D12_BLEND_OP_ADD;
	blend.SrcBlendAlpha = D3D12_BLEND_ONE;
	blend.DestBlendAlpha = D3D12_BLEND_ZERO;
	blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blend.LogicOp = D3D12_LOGIC_OP_NOOP;
	blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;

	const HRESULT psoHr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
	if (FAILED(psoHr)) return false;
	const Vertex TriangleVertices[] = {
		{ { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	uploadHeapProperties.CreationNodeMask = 1;
	uploadHeapProperties.VisibleNodeMask = 1;
	// const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC vertexBufferDesc{};
	vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexBufferDesc.Width = sizeof(TriangleVertices);
	vertexBufferDesc.Height = 1;
	vertexBufferDesc.DepthOrArraySize = 1;
	vertexBufferDesc.MipLevels = 1;
	vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	vertexBufferDesc.SampleDesc.Count = 1;
	vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	// const auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

	const HRESULT vertexBufferHr = m_device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&vertexBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)
	);
	if (FAILED(vertexBufferHr)) return false;

	const SIZE_T vertexDataSize = sizeof(TriangleVertices);

	const D3D12_RANGE readRange{ 0, 0 };
	void* mappedVertexData = nullptr;

	const HRESULT mapHr = m_vertexBuffer->Map(0, &readRange, &mappedVertexData);
	if (FAILED(mapHr)) return false;

	std::memcpy(mappedVertexData, TriangleVertices, vertexDataSize);

	const D3D12_RANGE writtenRange{ 0, vertexDataSize };
	m_vertexBuffer->Unmap(0, &writtenRange);

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = static_cast<UINT>(vertexDataSize);
	m_vertexBufferView.StrideInBytes = static_cast<UINT>(sizeof(Vertex));

	const HRESULT commandListHr = m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&m_commandList)
	);
	if (FAILED(commandListHr)) return false;

	const HRESULT commandListCloseHr = m_commandList->Close();
	if (FAILED(commandListCloseHr)) return false;

	const HRESULT createFenceHr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
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

bool D3D12Renderer::Render()
{
	HRESULT commandAllocatorResetHr = m_commandAllocator->Reset();
	if (FAILED(commandAllocatorResetHr)) return false;

	HRESULT commandListResetHr = m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get());
	if (FAILED(commandListResetHr)) return false;

	UINT frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_backBuffers[frameIndex].Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	m_commandList->ResourceBarrier(1, &barrier);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += frameIndex * m_rtvDescriptorSize;

	const float clearColor[4] = { 0.08f, 0.25f, 0.70f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	const D3D12_RESOURCE_DESC backBufferDesc = m_backBuffers[frameIndex]->GetDesc();

	const D3D12_VIEWPORT viewport{
		0.0f, 0.0f,
		static_cast<float>(backBufferDesc.Width),
		static_cast<float>(backBufferDesc.Height),
		0.0f, 1.0f
	};

	const D3D12_RECT scissorRect{
		0, 0,
		static_cast<LONG>(backBufferDesc.Width),
		static_cast<LONG>(backBufferDesc.Height)
	};

	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->RSSetViewports(1, &viewport);
	m_commandList->RSSetScissorRects(1, &scissorRect);

	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(3, 1, 0, 0);

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	m_commandList->ResourceBarrier(1, &barrier);

	HRESULT commandListCloseHr = m_commandList->Close();
	if (FAILED(commandListCloseHr)) return false;

	ID3D12CommandList* lists[] = { m_commandList.Get() };

	m_commandQueue->ExecuteCommandLists(1, lists);

	HRESULT presentHr = m_swapChain->Present(1, 0);

	bool gpuFinished = WaitForGPU();
	return SUCCEEDED(presentHr) && gpuFinished;
}