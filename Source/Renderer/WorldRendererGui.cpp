#include <algorithm>
#include <filesystem>
#include <3rdParty/imgui/imgui.h>
#include <Util/ImGuiExtensions.h>
#include <Util/AutoImGui.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>
#include <Engine/AssetManager.h>

#include "WorldRenderer.h"
#include "Light.h"
#include "Hbao.h"

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

void WorldRenderer::lightingGui()
{
	if (ImGui::BeginTabBar("LightingGuiTabBar"))
	{
		if (ImGui::BeginTabItem("Ambient"))
		{
			if (ImGui::CollapsingHeader("Color", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::SliderFloat("Intensity##ambient", &ambientLightIntensity, 0.0f, 2.0f);
				ImGui::ColorEdit3("Bottom color", reinterpret_cast<float*>(&ambientLightBottomColor));
				ImGui::ColorEdit3Srgb("Bottom color (SRGB)", reinterpret_cast<float*>(&ambientLightBottomColor));
				ImGui::ColorEdit3("Top color", reinterpret_cast<float*>(&ambientLightTopColor));
				ImGui::ColorEdit3Srgb("Top color (SRGB)", reinterpret_cast<float*>(&ambientLightTopColor));
			}
			if (ImGui::CollapsingHeader("SSAO", ImGuiTreeNodeFlags_DefaultOpen))
			{
				float oldSsaoResolutionScale = ssaoResolutionScale;
				ssao->gui(ssaoResolutionScale);
				if (ssaoResolutionScale != oldSsaoResolutionScale)
				{
					int w, h;
					drv->getDisplaySize(w, h);
					ssao.reset(new Hbao(XMINT2(w * ssaoResolutionScale, h * ssaoResolutionScale)));
				}
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Sun"))
		{
			if (ImGui::CollapsingHeader("Direction and color", ImGuiTreeNodeFlags_DefaultOpen))
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
				if (ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&mainLightColor)))
					mainLight->SetColor(mainLightColor);
				if (ImGui::ColorEdit3Srgb("Color (SRGB)", reinterpret_cast<float*>(&mainLightColor)))
					mainLight->SetColor(mainLightColor);
			}

			if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Checkbox("Enabled", &shadowEnabled);

				constexpr int numSoftShadowModes = 3;
				enum { SOFT_SHADOWS_OFF, SOFT_SHADOWS_TENT, SOFT_SHADOWS_POISSON };
				static const char* softShadowModeKeywords[numSoftShadowModes] = { "SOFT_SHADOWS_OFF", "SOFT_SHADOWS_TENT", "SOFT_SHADOWS_POISSON" };
				std::vector<std::string>& globalKeywords = am->getGlobalShaderKeywords();
				int selectedSoftShadowMode = 0;
				for (int i = 0; i < numSoftShadowModes; i++)
				{
					auto it = std::find(globalKeywords.begin(), globalKeywords.end(), softShadowModeKeywords[i]);
					if (it != globalKeywords.end())
					{
						selectedSoftShadowMode = i;
						break;
					}
				}
				bool softShadowModeChanged = false;
				for (int i = 0; i < numSoftShadowModes; i++)
				{
					static const char* softShadowModes[numSoftShadowModes] = { "Off", "Tent", "Poisson" };
					softShadowModeChanged |= ImGui::RadioButton(softShadowModes[i], &selectedSoftShadowMode, i);
					ImGui::SameLine();
				}
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.65f, 12); // Hack to continue where labels are
				ImGui::TextUnformatted("Soft shadow mode");
				if (softShadowModeChanged)
				{
					for (int i = 0; i < numSoftShadowModes; i++)
						am->setGlobalShaderKeyword(softShadowModeKeywords[i], i == selectedSoftShadowMode);
				}
				if (selectedSoftShadowMode == SOFT_SHADOWS_POISSON)
				{
					ImGui::SliderFloat("Poisson softness", &poissonShadowSoftness, 0, 0.01f, "%.5f");
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("Radius for poisson samples in shadowmap uv");
				}

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
			ImGui::EndTabItem();
		}
	}
	ImGui::EndTabBar();
}

void WorldRenderer::shadowMapGui()
{
	const unsigned int shadowResolution = getShadowResolution();
	static float zoom = 512.0f / shadowResolution;
	ImGui::SliderFloat("Zoom", &zoom, 0.1f, 5.0f);
	ImGui::Image(shadowMap->getViewHandle(), ImVec2(zoom * shadowResolution, zoom * shadowResolution));
}

void WorldRenderer::ssaoTexGui()
{
	ITexture* ssaoTex = ssao->getResultTex();
	XMINT2 ssaoTexRes(ssaoTex->getDesc().width, ssaoTex->getDesc().height);
	static float zoom = 0.5f;
	ImGui::SliderFloat("Zoom", &zoom, 0.1f, 5.0f);
	ImGui::Image(ssaoTex->getViewHandle(), ImVec2(zoom * ssaoTexRes.x, zoom * ssaoTexRes.y));
}

REGISTER_IMGUI_WINDOW("Lighting settings", []() { wr->lightingGui(); });
REGISTER_IMGUI_WINDOW_EX("Shadowmap debug", nullptr, 200, ImGuiWindowFlags_HorizontalScrollbar, []() { wr->shadowMapGui(); });
REGISTER_IMGUI_WINDOW_EX("SSAO tex debug", nullptr, 201, ImGuiWindowFlags_HorizontalScrollbar, []() { wr->ssaoTexGui(); });