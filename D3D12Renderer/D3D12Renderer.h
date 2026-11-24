#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <wrl/client.h>
#include <dxgi1_4.h>

using Microsoft::WRL::ComPtr;

class D3D12Renderer
{
public:
	bool Initialize(HWND hwnd);
	bool Render();
private:
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

	bool WaitForGPU();
};