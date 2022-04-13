#define NOMINMAX
#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include <direct.h>
#include <chrono>
#include <3rdParty/imgui/imgui.h>
#include <3rdParty/imgui/implot.h>
#include <3rdParty/imgui/imgui_impl_win32.h>
#include <3rdParty/plog/Log.h>
#include <3rdParty/plog/Appenders/RollingFileAppender.h>
#include <3rdParty/plog/Appenders/DebugOutputAppender.h>
#include <3rdParty/cxxopts/cxxopts.hpp>
#include <Driver/ITexture.h>
#include <Engine/AssetManager.h>
#include <Renderer/WorldRenderer.h>
#include <Renderer/RenderUtil.h>
#include <Renderer/Experiments/IFullscreenExperiment.h>
#include <Util/AutoImGui.h>
#include <Util/ImGuiLogWindow.h>
#include <Util/FpsLimiter.h>
#include <Util/ThreadPool.h>

#include "Common.h"

static constexpr int DEFAULT_WINDOWED_POS_X = 100;
static constexpr int DEFAULT_WINDOWED_POS_Y = 100;
static constexpr int DEFAULT_WINDOWED_WIDTH = 1600;
static constexpr int DEFAULT_WINDOWED_HEIGHT = 900;

static int showCmd;
static HWND hWnd;
static bool fullscreen = false;
static RECT lastWindowedRect = { DEFAULT_WINDOWED_POS_X, DEFAULT_WINDOWED_POS_Y, DEFAULT_WINDOWED_POS_X + DEFAULT_WINDOWED_WIDTH, DEFAULT_WINDOWED_POS_Y + DEFAULT_WINDOWED_HEIGHT };
static bool rightMouseButtonHeldDown = false;
static POINT lastMousePos;
static bool ctrl = false;
static std::string log_file_path;

void* get_hwnd() { return hWnd; }
const char* get_log_file_path() { return log_file_path.c_str(); }

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

static void init_logging()
{
	const std::time_t t = std::time(0);
	std::tm timeNow;
	localtime_s(&timeNow, &t);
	char timeStampBuf[MAX_PATH];
	strftime(timeStampBuf, sizeof(timeStampBuf), "%Y_%m_%d_%H_%M_%S", &timeNow);
	std::string timeStamp(timeStampBuf);
#if defined(TOY_DEBUG)
	const plog::Severity logSeverity = plog::debug;
	log_file_path = ".log/" + timeStamp + "_dbg.log";
#elif defined(TOY_DEV)
	const plog::Severity logSeverity = plog::info;
	log_file_path = ".log/" + timeStamp + "_dev.log";
#else
	const plog::Severity logSeverity = plog::error;
	log_file_path = ".log/" + timeStamp + "_rel.log";
#endif
	_mkdir(".log");
	static plog::RollingFileAppender<plog::ToyTxtFormatter> fileAppender(log_file_path.c_str(), 100 * 1024 * 1024, 1);
	static plog::DebugOutputAppender<plog::ToyTxtFormatter> debugOutputAppender;
	plog::init(logSeverity).addAppender(&fileAppender).addAppender(&debugOutputAppender).addAppender(&plog::imguiLogWindow);
	PLOG_INFO << "Log system initialized. Severity: " << plog::severityToString(logSeverity) <<". Log file : " << log_file_path;
}

static bool init()
{
	init_logging();
	init_cmdline_opts();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGui_ImplWin32_Init(hWnd);
	autoimgui::init();

	const std::string& driverOption = get_cmdline_opts()["driver"].as<std::string>();
	if (driverOption == "d3d11" || driverOption == "D3D11")
		create_driver_d3d11();
	else if (driverOption == "d3d12" || driverOption == "D3D12")
		create_driver_d3d12();
	else
	{
		PLOG_ERROR << "Unknown driver passed as command line param. Defaulting to D3D11.";
		create_driver_d3d11();
	}
	bool success = drv->init(hWnd, DEFAULT_WINDOWED_WIDTH, DEFAULT_WINDOWED_HEIGHT);
	if (!success)
	{
		PLOG_FATAL << "Driver initialization failed.";
		return false;
	}

	const int numCores = std::thread::hardware_concurrency();
	const int numWorkerThreads = std::clamp(numCores - 1, 3, 12);
	PLOG_INFO << "Detected number of processor cores: " << numCores;
	PLOG_INFO << "Initializing thread pool with " << numWorkerThreads << " threads";
	tp = new ThreadPool(numWorkerThreads);

#ifndef D3D12_DEV
	render_util::init();
#endif

	am = new AssetManager();

#ifndef D3D12_DEV
	wr = new WorldRenderer();
	wr->init();
#endif

	const std::string& sceneOption = get_cmdline_opts()["scene"].as<std::string>();
	std::string scenePath;
	if (!sceneOption.empty())
	{
		scenePath = "Assets/Scenes/";
		scenePath += sceneOption;
		scenePath += ".ini";
	}
	else
	{
		scenePath = autoimgui::load_custom_param("lastLoadedScenePath", "Assets/Scenes/default.ini");
	}
	am->loadScene(scenePath);

	return true;
}

static void shutdown()
{
	delete tp; // delete ThreadPool first, so jobs in flight don't end up working on deleted data
	delete wr;
	delete am;
	SAFE_DELETE(fe);
#ifndef D3D12_DEV
	render_util::shutdown();
#endif
	drv->shutdown();
	delete drv;
	autoimgui::shutdown();
	ImGui_ImplWin32_Shutdown();
	ImPlot::DestroyContext();
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

	if (fe != nullptr)
	{
		fe->gui();
		fe->update(deltaTimeInSeconds);
	}
	else
		wr->update(deltaTimeInSeconds);
}

static void render()
{
	if (fe != nullptr)
		fe->render(*drv->getBackbufferTexture());
	else
	{
		wr->beforeRender();
		wr->render();
	}

#ifndef D3D12_DEV
	drv->setRenderTarget(drv->getBackbufferTexture()->getId(), BAD_RESID);
#endif

	if (autoimgui::is_active)
		ImGui::Render();
	else
		ImGui::EndFrame();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	showCmd = nShowCmd;

	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	create_hwnd(hInstance);
	if (!init())
		return 1;

	ShowWindow(hWnd, nShowCmd);

	MSG msg;
	while (true)
	{
		FpsLimiter limiter(drv->getSettings().fpsLimit);

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
		drv->beginRender();
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

#ifdef D3D12_DEV
	if (message == WM_SIZE)
	{
		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		int w = clientRect.right - clientRect.left;
		int h = clientRect.bottom - clientRect.top;
		PLOG_INFO << "WM_SIZE, clientrect: x:" << clientRect.left << ", y:" << clientRect.top << ", w:" << w << ", h:" << h;

		// Minimize sends WM_SIZE requests with 0 size, which is invalid.
		w = std::max(8, w);
		h = std::max(8, h);
		if (wr)
			wr->onResize(w, h);
		else
			drv->resize(w, h);
		if (fe != nullptr)
			fe->onResize(w, h);
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
#endif

	if (wr == nullptr)
		return DefWindowProc(hWnd, message, wParam, lParam);

	switch (message)
	{
	case WM_RBUTTONUP:
		rightMouseButtonHeldDown = false;
		ReleaseCapture();
		ShowCursor(true);
		wr->sceneCameraInputState = {};
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
			wr->sceneCameraInputState.deltaYaw += mousePos.x - lastMousePos.x;
			wr->sceneCameraInputState.deltaPitch += mousePos.y - lastMousePos.y;
			SetCursorPos(lastMousePos.x, lastMousePos.y);
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case 0x57: // W
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isMovingForward = false;
			break;
		case 0x53: // S
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isMovingBackward = false;
			break;
		case 0x44: // D
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isMovingRight = false;
			break;
		case 0x41: // A
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isMovingLeft = false;
			break;
		case 0x45: // E
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isMovingUp = false;
			break;
		case 0x51: // Q
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isMovingDown = false;
			break;
		case VK_SHIFT:
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isSpeeding = false;
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
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isMovingForward = true;
			break;
		case 0x53: // S
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isMovingBackward = true;
			break;
		case 0x44: // D
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isMovingRight = true;
			break;
		case 0x41: // A
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isMovingLeft = true;
			break;
		case 0x45: // E
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isMovingUp = true;
			break;
		case 0x51: // Q
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isMovingDown = true;
			break;
		case 0x52: // R
			if (ctrl)
				drv->recompileShaders();
			break;
		case VK_SHIFT:
			if (rightMouseButtonHeldDown) wr->sceneCameraInputState.isSpeeding = true;
			break;
		case VK_F2:
			autoimgui::is_active = !autoimgui::is_active;
			break;
		case VK_F3:
			wr->toggleWireframe();
			break;
		case VK_CONTROL:
			ctrl = true;
			break;
		case VK_F11:
		{
			if (!fullscreen)
				GetWindowRect(hWnd, &lastWindowedRect);

			fullscreen = !fullscreen;

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
		PLOG_INFO << "WM_SIZE, clientrect: x:" << clientRect.left << ", y:" << clientRect.top << ", w:" << w << ", h:" << h;

		// Minimize sends WM_SIZE requests with 0 size, which is invalid.
		w = std::max(8, w);
		h = std::max(8, h);
		wr->onResize(w, h);
		if (fe != nullptr)
			fe->onResize(w, h);
		break;
	}
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void exit_program()
{
	PostMessage(hWnd, WM_CLOSE, 0, 0);
}

REGISTER_IMGUI_FUNCTION_EX("App", "Toggle fullscreen", "F11", 100, [] { PostMessage(hWnd, WM_KEYDOWN, VK_F11, 0); });
REGISTER_IMGUI_FUNCTION_EX("App", "Toggle wireframe", "F3", 101, [] { wr->toggleWireframe(); });
REGISTER_IMGUI_FUNCTION_EX("App", "Recompile shaders", "Ctrl+R", 101, [] { drv->recompileShaders(); });
REGISTER_IMGUI_FUNCTION_EX("App", "Exit", "Alt+F4", 999, exit_program);
REGISTER_IMGUI_FUNCTION_EX("ImGui", "Hide ImGui", "F2", 100, []() { autoimgui::is_active = false; });