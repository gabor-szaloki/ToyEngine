#include <stdio.h>
#include <chrono>
#include <3rdParty/ImGui/imgui.h>
#include <3rdParty/ImGui/imgui_impl_win32.h>
#include <Renderer/WorldRenderer.h>

#include "Common.h"

#define DEFAULT_WINDOWED_POS_X 100
#define DEFAULT_WINDOWED_POS_Y 100
#define DEFAULT_WINDOWED_WIDTH 1280
#define DEFAULT_WINDOWED_HEIGHT 720

static int showCmd;
static HWND hWnd;
static bool fullscreen = false;
static RECT lastWindowedRect = { DEFAULT_WINDOWED_POS_X, DEFAULT_WINDOWED_POS_Y, DEFAULT_WINDOWED_POS_X + DEFAULT_WINDOWED_WIDTH, DEFAULT_WINDOWED_POS_Y + DEFAULT_WINDOWED_HEIGHT };
static bool rightMouseButtonHeldDown = false;
static POINT lastMousePos;
static bool showImGui = true;
static bool ctrl = false;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

static void create_hwnd(HINSTANCE h_instance)
{
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = h_instance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = "WindowClass1";
	RegisterClassEx(&wc);

	RECT windowRect = { 0, 0, DEFAULT_WINDOWED_WIDTH, DEFAULT_WINDOWED_HEIGHT };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

	hWnd = CreateWindowEx(0,
		"WindowClass1", // window class
		"ToyEngine", // title
		WS_OVERLAPPEDWINDOW, // window style
		//WS_POPUP, // window style
		DEFAULT_WINDOWED_POS_X, DEFAULT_WINDOWED_POS_Y, // x, y
		windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, // w, h
		nullptr, nullptr, h_instance, nullptr);
}

static bool init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hWnd);

	create_driver_d3d11();
	bool success = drv->init(hWnd, DEFAULT_WINDOWED_WIDTH, DEFAULT_WINDOWED_HEIGHT);
	if (!success)
	{
		OutputDebugString("Driver initialization failed. Exiting.");
		return false;
	}
	wr = new WorldRenderer();
	return true;
}

static void shutdown()
{
	delete wr;
	drv->shutdown();
	delete drv;
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

static void update()
{
	auto currentTime = std::chrono::high_resolution_clock::now();
	static auto lastTime = currentTime;
	float deltaTimeInSeconds = (float)(((double)std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime).count()) * 0.001 * 0.001);
	lastTime = currentTime;

	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	autoimgui::perform();
	wr->update(deltaTimeInSeconds);
}

static void render()
{
	wr->render();
	if (showImGui)
		ImGui::Render();
	else
		ImGui::EndFrame();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	showCmd = nShowCmd;

	create_hwnd(hInstance);
	if (!init())
		return 1;

	ShowWindow(hWnd, nShowCmd);

	MSG msg;
	while (true)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				shutdown();
				return (int)msg.wParam;
			}
		}

		drv->beginFrame();
		update();
		render();
		drv->endFrame();
		drv->present();
	}

	return 0;
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

	if (wr == nullptr)
		return DefWindowProc(hWnd, message, wParam, lParam);

	switch (message)
	{
	case WM_RBUTTONUP:
		rightMouseButtonHeldDown = false;
		ReleaseCapture();
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
			wr->cameraInputState.deltaYaw += mousePos.x - lastMousePos.x;
			wr->cameraInputState.deltaPitch += mousePos.y - lastMousePos.y;
			SetCursorPos(lastMousePos.x, lastMousePos.y);
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case 0x57: // W
			wr->cameraInputState.isMovingForward = false;
			break;
		case 0x53: // S
			wr->cameraInputState.isMovingBackward = false;
			break;
		case 0x44: // D
			wr->cameraInputState.isMovingRight = false;
			break;
		case 0x41: // A
			wr->cameraInputState.isMovingLeft = false;
			break;
		case 0x45: // E
			wr->cameraInputState.isMovingUp = false;
			break;
		case 0x51: // Q
			wr->cameraInputState.isMovingDown = false;
			break;
		case VK_SHIFT:
			wr->cameraInputState.isSpeeding = false;
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
			wr->cameraInputState.isMovingForward = true;
			break;
		case 0x53: // S
			wr->cameraInputState.isMovingBackward = true;
			break;
		case 0x44: // D
			wr->cameraInputState.isMovingRight = true;
			break;
		case 0x41: // A
			wr->cameraInputState.isMovingLeft = true;
			break;
		case 0x45: // E
			wr->cameraInputState.isMovingUp = true;
			break;
		case 0x51: // Q
			wr->cameraInputState.isMovingDown = true;
			break;
		case VK_SHIFT:
			wr->cameraInputState.isSpeeding = true;
			break;
		case VK_F2:
			showImGui = !showImGui;
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

		// Minimize sends WM_SIZE requests with 0 size, which is invalid.
		w = glm::max(8, w);
		h = glm::max(8, h);
		wr->onResize(w, h);
		break;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

REGISTER_IMGUI_FUNCTION_EX("App", "Toggle fullscreen", "F11", 100, [] { PostMessage(hWnd, WM_KEYDOWN, VK_F11, 0); });
REGISTER_IMGUI_FUNCTION_EX("App", "Exit", "Alt+F4", 999, [] { PostMessage(hWnd, WM_CLOSE, 0, 0); });
REGISTER_IMGUI_FUNCTION_EX("ImGui", "Hide ImGui", "F2", 100, []() { showImGui = false; });