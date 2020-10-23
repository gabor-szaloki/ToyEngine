#include <3rdParty/ImGui/imgui.h>
#include <Common.h>
#include <Driver/ITexture.h>

#include "WorldRenderer.h"
#include "Renderer/Light.h"

static void renderer_settings()
{
	ImGui::Checkbox("Show wireframe", &wr->showWireframe);
}
REGISTER_IMGUI_WINDOW("Renderer settings", renderer_settings);

static void lighting_settings()
{
	ImGui::Text("Ambient light");
	ImGui::SliderFloat("Intensity##ambient", &wr->ambientLightIntensity, 0.0f, 1.0f);
	ImGui::ColorEdit3("Color##ambient", reinterpret_cast<float*>(&wr->ambientLightColor));

	ImGui::Text("Main light");
	Light& mainLight = *wr->mainLight.get();
	float mainLightYaw = mainLight.GetYaw();
	float mainLightPitch = mainLight.GetPitch();
	float mainLightIntensity = mainLight.GetIntensity();
	XMFLOAT4 mainLightColor = mainLight.GetColor();
	if (ImGui::SliderAngle("Yaw", &mainLightYaw, -180.0f, 180.0f))
		mainLight.SetYaw(mainLightYaw);
	if (ImGui::SliderAngle("Pitch", &mainLightPitch, 0.0f, 90.0f))
		mainLight.SetPitch(mainLightPitch);
	if (ImGui::SliderFloat("Intensity##main", &mainLightIntensity, 0.0f, 2.0f))
		mainLight.SetIntensity(mainLightIntensity);
	if (ImGui::ColorEdit3("Color##main", reinterpret_cast<float*>(&mainLightColor)))
		mainLight.SetColor(mainLightColor);

	ImGui::Text("Main light shadows");
	ImGui::SliderFloat("Shadow distance", &wr->shadowDistance, 1.0f, 100.0f);
	ImGui::SliderFloat("Directional distance", &wr->directionalShadowDistance, 1.0f, 100.0f);
	int shadowResolution = wr->getShadowResolution();
	if (ImGui::InputInt("Resolution", &shadowResolution, 512, 1024))
		wr->setShadowResolution((int)fmaxf((float)shadowResolution, 128));
	bool setShadowBias = false;
	setShadowBias |= ImGui::DragInt("Depth bias", &wr->shadowDepthBias, 100.0f);
	setShadowBias |= ImGui::DragFloat("Slope scaled depth bias", &wr->shadowSlopeScaledDepthBias, 0.01f);
	if (setShadowBias)
		wr->setShadowBias(wr->shadowDepthBias, wr->shadowSlopeScaledDepthBias);

	if (ImGui::Button("Show shadowmap"))
		autoimgui::set_window_opened("Shadowmap debug", !autoimgui::is_window_opened("Shadowmap debug"));
}
REGISTER_IMGUI_WINDOW("Lighting settings", lighting_settings);

static void shadowmap_debug_window()
{
	const unsigned int shadowResolution = wr->getShadowResolution();
	static float zoom = 512.0f / shadowResolution;
	ImGui::SliderFloat("Zoom", &zoom, 0.1f, 5.0f);
	ImGui::Image(wr->getShadowMap()->getViewHandle(), ImVec2(zoom * shadowResolution, zoom * shadowResolution));
}
REGISTER_IMGUI_WINDOW_EX("Shadowmap debug", nullptr, 100, ImGuiWindowFlags_HorizontalScrollbar, shadowmap_debug_window);