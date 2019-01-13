#include <stdio.h>
#include <time.h>

#include "Common.h"
#include "Engine.h"

#define DEFAULT_WINDOWED_WIDTH 1280
#define DEFAULT_WINDOWED_HEIGHT 720

int showCmd;
bool fullscreen = false;
bool rightMouseButtonHeldDown = false;
POINT lastMousePos;
bool ctrl = false;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
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

		float time = (float)clock() / (float)CLOCKS_PER_SEC;
		gEngine->Update(time);
		gEngine->RenderFrame();
	}

quit:
	gEngine->Release();
	delete gEngine;
	return msg.wParam;
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	if (message == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}

	if (gEngine == nullptr)
		return DefWindowProc(hWnd, message, wParam, lParam);

	switch (message)
	{
	case WM_RBUTTONUP:
		rightMouseButtonHeldDown = false;
		SetCapture(nullptr);
		ShowCursor(true);
		break;
	case WM_RBUTTONDOWN:
	{
		rightMouseButtonHeldDown = true;
		SetCapture(hWnd);
		ShowCursor(false);
		GetCursorPos(&lastMousePos);
		break;
	}
	case WM_MOUSEMOVE:
		if (rightMouseButtonHeldDown)
		{
			POINT mousePos;
			GetCursorPos(&mousePos);
			gEngine->cameraInputState.deltaYaw = mousePos.x - lastMousePos.x;
			gEngine->cameraInputState.deltaPitch = mousePos.y - lastMousePos.y;
			SetCursorPos(lastMousePos.x, lastMousePos.y);
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case 0x57: // W
			gEngine->cameraInputState.isMovingForward = false;
			break;
		case 0x53: // S
			gEngine->cameraInputState.isMovingBackward = false;
			break;
		case 0x44: // D
			gEngine->cameraInputState.isMovingRight = false;
			break;
		case 0x41: // A
			gEngine->cameraInputState.isMovingLeft = false;
			break;
		case 0x45: // E
			gEngine->cameraInputState.isMovingUp = false;
			break;
		case 0x51: // Q
			gEngine->cameraInputState.isMovingDown = false;
			break;
		case VK_SHIFT:
			gEngine->cameraInputState.isSpeeding = false;
			break;
		case VK_CONTROL:
			ctrl = false;
			break;
		}
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 0x57: // W
			if (ctrl)
				gEngine->ToggleGUI();
			else
				gEngine->cameraInputState.isMovingForward = true;
			break;
		case 0x53: // S
			gEngine->cameraInputState.isMovingBackward = true;
			break;
		case 0x44: // D
			gEngine->cameraInputState.isMovingRight = true;
			break;
		case 0x41: // A
			gEngine->cameraInputState.isMovingLeft = true;
			break;
		case 0x45: // E
			gEngine->cameraInputState.isMovingUp = true;
			break;
		case 0x51: // Q
			gEngine->cameraInputState.isMovingDown= true;
			break;
		case VK_SHIFT:
			gEngine->cameraInputState.isSpeeding = true;
			break;
		case VK_CONTROL:
			ctrl = true;
			break;
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
			gEngine->Init(hWnd, float(clientRect.right - clientRect.left), float(clientRect.bottom - clientRect.top));

			break;
		}
		}
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}