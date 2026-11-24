#include <Windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    constexpr wchar_t className[] = L"D3D12RendererWindow";

    // 1. 창 클래스와 메시지 처리 함수를 등록합니다.
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = GetSysColorBrush(COLOR_WINDOW);

    if (RegisterClassW(&windowClass) == 0)
    {
        return 1;
    }

    // 2. 창을 만들고 화면에 표시합니다.
    HWND hwnd = CreateWindowExW(
        0,
        className,
        L"D3D12 Renderer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr, nullptr, hInstance, nullptr);

    if (hwnd == nullptr)
    {
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);

    // 3. 종료 메시지를 받을 때까지 창 메시지를 처리합니다.
    MSG message{};
    for (;;)
    {
        const BOOL result = GetMessageW(&message, nullptr, 0, 0);
        if (result == -1)
        {
            return 1;
        }
        if (result == 0)
        {
            return static_cast<int>(message.wParam);
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}