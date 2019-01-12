#include "Engine.h"

#define DEFAULT_WINDOWED_WIDTH 1280
#define DEFAULT_WINDOWED_HEIGHT 720

int showCmd;
bool fullscreen = false;

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	showCmd = nShowCmd;

	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "WindowClass1";
	RegisterClassEx(&wc);

	RECT wr = { 0, 0, DEFAULT_WINDOWED_WIDTH, DEFAULT_WINDOWED_HEIGHT };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);

	HWND hWnd = CreateWindowEx(0,
		"WindowClass1", // window class
		"ThreeDee", // title
		WS_OVERLAPPEDWINDOW, // window style
		//WS_POPUP, // window style
		100, 100, wr.right - wr.left, wr.bottom - wr.top, // x, y, w, h
		nullptr, nullptr, hInstance, nullptr);

	gEngine = new Engine();
	gEngine->Init(hWnd, DEFAULT_WINDOWED_WIDTH, DEFAULT_WINDOWED_HEIGHT);

	ShowWindow(hWnd, nShowCmd);

	MSG msg;
	while (true)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
				goto quit;
		}

		gEngine->RenderFrame();
	}

quit:
	gEngine->Release();
	delete gEngine;
	return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_F11:
		{
			fullscreen = !fullscreen;
			OutputDebugString(fullscreen ? "Going fullscreen\n" : "Going windowed\n");

			HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO monitorInfo;
			monitorInfo.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(hMon, &monitorInfo);

			RECT monitorRect = monitorInfo.rcMonitor;
			RECT clientRect = fullscreen ? monitorRect : 
				RECT { monitorRect.left, monitorRect.top, monitorRect.left + DEFAULT_WINDOWED_WIDTH, monitorRect.top + DEFAULT_WINDOWED_HEIGHT };
			DWORD windowStyle = fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
			RECT windowRect = clientRect;
			AdjustWindowRect(&windowRect, windowStyle, false);

			int x = windowRect.left;
			int y = windowRect.top;
			if (!fullscreen)
			{
				x += 100;
				y += 100;
			}
			int width = windowRect.right - windowRect.left;
			int height = windowRect.bottom - windowRect.top;

			SetWindowLong(hWnd, GWL_STYLE, windowStyle);
			SetWindowPos(hWnd, hWnd, x, y, width, height, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			MoveWindow(hWnd, x, y, width, height, true);
			ShowWindow(hWnd, showCmd);

			gEngine->Release();
			gEngine->Init(hWnd, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);

			break;
		}
		}
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}