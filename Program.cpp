#include <stdio.h>
#include <chrono>

#include "Common.h"
#include "Engine.h"

#define DEFAULT_WINDOWED_POS_X 100
#define DEFAULT_WINDOWED_POS_Y 100
#define DEFAULT_WINDOWED_WIDTH 1280
#define DEFAULT_WINDOWED_HEIGHT 720

int showCmd;
bool fullscreen = false;
RECT lastWindowedRect = { DEFAULT_WINDOWED_POS_X, DEFAULT_WINDOWED_POS_Y, DEFAULT_WINDOWED_POS_X + DEFAULT_WINDOWED_WIDTH, DEFAULT_WINDOWED_POS_Y + DEFAULT_WINDOWED_HEIGHT };
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
		"ToyEngine", // title
		WS_OVERLAPPEDWINDOW, // window style
		//WS_POPUP, // window style
		DEFAULT_WINDOWED_POS_X, DEFAULT_WINDOWED_POS_Y, wr.right - wr.left, wr.bottom - wr.top, // x, y, w, h
		nullptr, nullptr, hInstance, nullptr);

	gEngine = new Engine();
	gEngine->Init(hWnd, DEFAULT_WINDOWED_WIDTH, DEFAULT_WINDOWED_HEIGHT);

	ShowWindow(hWnd, nShowCmd);

	auto lastTime = std::chrono::high_resolution_clock::now();

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

		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTimeInSeconds = (float)(((double)std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime).count()) * 0.001 * 0.001);
		lastTime = currentTime;

		gEngine->Update(deltaTimeInSeconds);
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
			gEngine->cameraInputState.deltaYaw += mousePos.x - lastMousePos.x;
			gEngine->cameraInputState.deltaPitch += mousePos.y - lastMousePos.y;
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
			if (!fullscreen)
				GetWindowRect(hWnd, &lastWindowedRect);

			fullscreen = !fullscreen;
			OutputDebugString(fullscreen ? "VK_F11, Going fullscreen\n" : "Going windowed\n");

			HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO monitorInfo;
			monitorInfo.cbSize = sizeof(MONITORINFO);
			GetMonitorInfo(hMon, &monitorInfo);

			RECT monitorRect = monitorInfo.rcMonitor;
			RECT windowRect = fullscreen ? monitorRect : lastWindowedRect;

			int x = windowRect.left;
			int y = windowRect.top;
			int width = windowRect.right - windowRect.left;
			int height = windowRect.bottom - windowRect.top;

			SetWindowLong(hWnd, GWL_STYLE, fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW);
			SetWindowPos(hWnd, hWnd, x, y, width, height, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			MoveWindow(hWnd, x, y, width, height, true);
			ShowWindow(hWnd, showCmd);

			break;
		}
		}
		break;
	case WM_SIZE:
	{
		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		int w = clientRect.right - clientRect.left;
		int h = clientRect.bottom - clientRect.top;

		char str[MAX_PATH];
		sprintf_s(str, "WM_SIZE, clientRect: x:%d, y:%d, w:%d, h:%d\n", clientRect.left, clientRect.top, w, h);
		OutputDebugString(str);

		gEngine->Resize(hWnd, float(w), float(h));

		break;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}