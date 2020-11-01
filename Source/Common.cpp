#include "Common.h"

#define NOMINMAX
#include <Windows.h>
#include <3rdParty/imgui/imgui.h>
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
	if (ImGui::Button("Recompile shaders"))
		drv->recompileShaders();

	if (changed)
		drv->setSettings(drvSettings);
}
REGISTER_IMGUI_WINDOW("Driver settings", driver_settings_window);

static void stats_window()
{
	float fps = ImGui::GetIO().Framerate;
	ImGui::Text("FPS:           %.0f", fps);
	ImGui::Text("Frame time:    %.1f ms", 1000.0f / fps);
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
