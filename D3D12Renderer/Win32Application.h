#pragma once
#include <Windows.h>

class Win32Application
{
public:
	void AttachWindowState(HWND hwnd);
	bool ConsumeResizePending();
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:

	struct WindowState
	{
		bool resizePending = false;
	};

	WindowState m_windowState{};
};