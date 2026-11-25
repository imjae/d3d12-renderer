#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class D3D12Renderer
{
public:
	bool Initialize(HWND hwnd);
	bool Render();
	void Resize(UINT width, UINT height);

private:
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	struct Vertex
	{
		XMFLOAT3 position; // x, y, z
		XMFLOAT4 color;    // r, g, b, a
	};

	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<IDXGISwapChain3> m_swapChain;

	static constexpr UINT BackBufferCount = 2;
	ComPtr<ID3D12Resource> m_backBuffers[BackBufferCount];
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	UINT m_rtvDescriptorSize = 0;

	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;

	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;

	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};

	bool WaitForGPU();
};