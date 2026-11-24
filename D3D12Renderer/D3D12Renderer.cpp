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
}