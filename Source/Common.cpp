#include "Common.h"

#define NOMINMAX
#include <Windows.h>
#include <3rdParty/imgui/imgui.h>
#include <3rdParty/imgui/implot.h>
#include <Driver/IDriver.h>
#include <Util/AutoImGui.h>

IDriver* drv;
WorldRenderer* wr;

wchar_t* utf8_to_wcs(const char* utf8_str, wchar_t* wcs_buf, int wcs_buf_len)
{
	int cnt = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wcs_buf, wcs_buf_len);
	if (!cnt)
		return nullptr;
	wcs_buf[cnt < wcs_buf_len ? cnt : wcs_buf_len - 1] = L'\0';
	return wcs_buf;
}

char* wcs_to_utf8(const wchar_t* wcs_str, char* utf8_buf, int utf8_buf_len)
{
	int cnt = WideCharToMultiByte(CP_UTF8, 0, wcs_str, -1, utf8_buf, utf8_buf_len, NULL, NULL);
	if (!cnt)
		return NULL;
	utf8_buf[cnt < utf8_buf_len ? cnt : utf8_buf_len - 1] = L'\0';
	return utf8_buf;
}

static void driver_settings_window()
{
	DriverSettings drvSettings = drv->getSettings();
	bool changed = false;

	changed |= ImGui::SliderInt("Texture filtering anisotropy", (int*)&drvSettings.textureFilteringAnisotropy, 0, 16);
	changed |= ImGui::Checkbox("V-sync", &drvSettings.vsync);
	changed |= ImGui::SliderInt("FPS limit", &drvSettings.fpsLimit, 0, 360); // fpsLimit is handled outside of driver

	if (ImGui::Button("Recompile shaders"))
		drv->recompileShaders();

	if (changed)
		drv->setSettings(drvSettings);
}
REGISTER_IMGUI_WINDOW("Driver settings", driver_settings_window);

static void stats_window()
{
	ImGuiIO io = ImGui::GetIO();
	ImGui::Text("Average FPS:           %.1f", io.Framerate);
	ImGui::Text("Average frame time:    %.3f ms", 1000.0f / io.Framerate);

	static constexpr int NUM_FRAMES = 1001;
	static float frameTimes[NUM_FRAMES] = {};
	static bool paused = false;
	if (!paused)
	{
		memcpy(&frameTimes[1], &frameTimes[0], sizeof(frameTimes[0]) * (NUM_FRAMES - 1));
		frameTimes[0] = io.DeltaTime * 1000.0f;
	}

	ImPlot::SetNextPlotLimits(0, 1000, 0, 1000.0/60.0);
	if (ImPlot::BeginPlot("Frame times", "frames", "frame time (ms)", ImVec2(-1, 0), 0, ImPlotAxisFlags_Invert, ImPlotAxisFlags_LockMin)) {
		ImPlot::PlotLine("Frame times", frameTimes, NUM_FRAMES);
		ImPlot::EndPlot();
	}

	if (ImGui::Button("Clear"))
		memset(frameTimes, 0, sizeof(frameTimes[0]) * NUM_FRAMES);
	ImGui::SameLine();
	if (ImGui::Button(paused ? "Resume" : "Pause"))
		paused = !paused;
}
REGISTER_IMGUI_WINDOW("Stats", stats_window);

ProfileScopeHelper::ProfileScopeHelper(const char* label)
{
	drv->beginEvent(label);
}

ProfileScopeHelper::~ProfileScopeHelper()
{
	drv->endEvent();
}
