#include <Windows.h>
#include "D3D12Renderer.h"
#include "Win32Application.h"

/*
해야할 일 정리
1. wWinMain 함수로 진입점 정의
2. 창 메시지 받을 함수 작성하기
3. 창 클래스 등록하기
4. 창 만들고 표시하기
5. 메시지 루프 돌리기
*/


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	const wchar_t CLASS_NAME[] = L"Main Window";

	WNDCLASS wc = {};
	wc.lpfnWndProc = Win32Application::WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0,
		CLASS_NAME,
		L"SELF_D3D12_RENDERERER",
		WS_OVERLAPPEDWINDOW,

		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
	);

	if (hwnd == NULL) return 0;

	D3D12Renderer renderer;
	if (!renderer.Initialize(hwnd)) return 1;

	Win32Application app;
	app.AttachWindowState(hwnd);

	ShowWindow(hwnd, nCmdShow);

	renderer.Render();

	MSG msg = {};
	for (;;)
	{
		if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				return static_cast<int>(msg.wParam);
			}

			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			continue;
		}

		if (app.ConsumeResizePending() && !IsIconic(hwnd))
		{
			RECT clientRect{};
			if (!GetClientRect(hwnd, &clientRect)) return 1;

			const LONG width = clientRect.right - clientRect.left;
			const LONG height = clientRect.bottom - clientRect.top;

			if (width > 0 && height > 0)
			{
				renderer.Resize(
					static_cast<UINT>(width),
					static_cast<UINT>(height)
				);
			}
		}

		if (!renderer.Render())
		{
			return 1;
		}
	}

	return 0;
}

LRESULT CALLBACK Win32Application::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);


		EndPaint(hwnd, &ps);
	}
	case WM_SIZE:
	{
		auto* state = reinterpret_cast<WindowState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		if (state != nullptr)
		{
			state->resizePending = true;
		}
	}
	return 0;

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Win32Application::AttachWindowState(HWND hwnd)
{
	SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&m_windowState));
}

bool Win32Application::ConsumeResizePending()
{
	const bool pending = m_windowState.resizePending;
	m_windowState.resizePending = false;

	return pending;
}