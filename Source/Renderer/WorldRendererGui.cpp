#include <algorithm>
#include <filesystem>
#include <3rdParty/imgui/imgui.h>
#include <Util/AutoImGui.h>
#include <Driver/ITexture.h>
#include <Engine/MeshRenderer.h>

#include "WorldRenderer.h"
#include "Light.h"

static XMVECTOR euler_to_quaternion(XMVECTOR e)
{
	float roll = e.m128_f32[0], pitch = e.m128_f32[1], yaw = e.m128_f32[2];

	float qx = sinf(roll * 0.5f) * cosf(pitch * 0.5f) * cosf(yaw * 0.5f) - cosf(roll * 0.5f) * sinf(pitch * 0.5f) * sinf(yaw * 0.5f);
	float qy = cosf(roll * 0.5f) * sinf(pitch * 0.5f) * cosf(yaw * 0.5f) + sinf(roll * 0.5f) * cosf(pitch * 0.5f) * sinf(yaw * 0.5f);
	float qz = cosf(roll * 0.5f) * cosf(pitch * 0.5f) * sinf(yaw * 0.5f) - sinf(roll * 0.5f) * sinf(pitch * 0.5f) * cosf(yaw * 0.5f);
	float qw = cosf(roll * 0.5f) * cosf(pitch * 0.5f) * cosf(yaw * 0.5f) + sinf(roll * 0.5f) * sinf(pitch * 0.5f) * sinf(yaw * 0.5f);

	XMFLOAT4 q(qx, qy, qz, qw);
	return XMLoadFloat4(&q);
}

void WorldRenderer::rendererSettingsGui()
{
	static std::vector<std::string> scenePaths;
	static std::string scenes;
	static int currentScene = -1;

	if (scenePaths.size() == 0)
	{
		for (const std::filesystem::directory_entry& file : std::filesystem::directory_iterator("Assets/Scenes"))
		{
			std::string scenePath = file.path().string();
			std::replace(scenePath.begin(), scenePath.end(), '\\', '/');
			scenePaths.push_back(scenePath);
			std::string sceneName = file.path().filename().string();
			sceneName = sceneName.substr(0, sceneName.length() - 4); // remove ".ini"
			scenes += sceneName;
			scenes += '\0';
		}
	}
	if (currentScene < 0)
	{
		std::string lastLoadedScenePath = autoimgui::load_custom_param("lastLoadedScenePath");
		auto it = std::find(scenePaths.begin(), scenePaths.end(), lastLoadedScenePath);
		currentScene = it != scenePaths.end() ? int(it - scenePaths.begin()) : 0;
	}
	ImGui::Combo("Scene", &currentScene, scenes.c_str());
	ImGui::SameLine();
	if (ImGui::Button("Load"))
	{
		unloadCurrentScene();
		loadScene(scenePaths[currentScene]);
		autoimgui::save_custom_param("lastLoadedScenePath", scenePaths[currentScene]);
	}

	ImGui::Checkbox("Show wireframe", &showWireframe);
}

void WorldRenderer::lightingGui()
{
	if (ImGui::CollapsingHeader("Ambient light", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::SliderFloat("Intensity##ambient", &ambientLightIntensity, 0.0f, 1.0f);
		ImGui::ColorEdit3("Color##ambient", reinterpret_cast<float*>(&ambientLightColor));
	}

	if (ImGui::CollapsingHeader("Main light", ImGuiTreeNodeFlags_DefaultOpen))
	{
		float mainLightYaw = mainLight->GetYaw();
		float mainLightPitch = mainLight->GetPitch();
		float mainLightIntensity = mainLight->GetIntensity();
		XMFLOAT4 mainLightColor = mainLight->GetColor();
		if (ImGui::SliderAngle("Yaw", &mainLightYaw, -180.0f, 180.0f))
			mainLight->SetYaw(mainLightYaw);
		if (ImGui::SliderAngle("Pitch", &mainLightPitch, 0.0f, 90.0f))
			mainLight->SetPitch(mainLightPitch);
		if (ImGui::SliderFloat("Intensity##main", &mainLightIntensity, 0.0f, 2.0f))
			mainLight->SetIntensity(mainLightIntensity);
		if (ImGui::ColorEdit3("Color##main", reinterpret_cast<float*>(&mainLightColor)))
			mainLight->SetColor(mainLightColor);
	}

	if (ImGui::CollapsingHeader("Main light shadows", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Enabled", &shadowEnabled);
		ImGui::SliderFloat("Shadow distance", &shadowDistance, 1.0f, 100.0f);
		ImGui::SliderFloat("Directional distance", &directionalShadowDistance, 1.0f, 100.0f);
		int shadowResolution = getShadowResolution();
		if (ImGui::InputInt("Resolution", &shadowResolution, 512, 1024))
			setShadowResolution((int)fmaxf((float)shadowResolution, 128));
		bool shadowBiasChanged = false;
		shadowBiasChanged |= ImGui::DragInt("Depth bias", &shadowDepthBias, 100.0f);
		shadowBiasChanged |= ImGui::DragFloat("Slope scaled depth bias", &shadowSlopeScaledDepthBias, 0.01f);
		if (shadowBiasChanged)
			setShadowBias(shadowDepthBias, shadowSlopeScaledDepthBias);
		if (ImGui::Button("Show shadowmap"))
			autoimgui::set_window_opened("Shadowmap debug", !autoimgui::is_window_opened("Shadowmap debug"));
	}
}

void WorldRenderer::shadowMapGui()
{
	const unsigned int shadowResolution = getShadowResolution();
	static float zoom = 512.0f / shadowResolution;
	ImGui::SliderFloat("Zoom", &zoom, 0.1f, 5.0f);
	ImGui::Image(shadowMap->getViewHandle(), ImVec2(zoom * shadowResolution, zoom * shadowResolution));
}

void WorldRenderer::meshRendererGui()
{
	for (MeshRenderer* mr : sceneMeshRenderers)
		if (ImGui::CollapsingHeader(mr->name.c_str()))
			mr->gui();
}

REGISTER_IMGUI_WINDOW("Renderer settings", []() { wr->rendererSettingsGui(); });
REGISTER_IMGUI_WINDOW("Lighting settings", []() { wr->lightingGui(); });
REGISTER_IMGUI_WINDOW_EX("Shadowmap debug", nullptr, 100, ImGuiWindowFlags_HorizontalScrollbar, []() { wr->shadowMapGui(); });
REGISTER_IMGUI_WINDOW("Mesh renderers", []() { wr->meshRendererGui(); });