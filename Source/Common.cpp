#include "Common.h"

#define NOMINMAX
#include <Windows.h>
#include <3rdParty/imgui/imgui.h>
#include <Driver/IDriver.h>

IDriver* drv;
WorldRenderer* wr;

static void driver_settings_window()
{
	DriverSettings drvSettings = drv->getSettings();
	bool changed = false;

	changed |= ImGui::SliderInt("Texture filtering anisotropy", (int*)&drvSettings.textureFilteringAnisotropy, 0, 16);
	changed |= ImGui::Checkbox("V-sync", &drvSettings.vsync);
	//if (ImGui::Button("Recompile shaders"))
	//	RecompileShaders();

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