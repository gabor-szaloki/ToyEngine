#include <algorithm>
#include <filesystem>
#include <3rdParty/imgui/imgui.h>
#include <Util/ImGuiExtensions.h>
#include <Util/AutoImGui.h>
#include <Driver/ITexture.h>
#include <Driver/IBuffer.h>

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
	if (ImGui::CollapsingHeader("Ambient light", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::SliderFloat("Intensity##ambient", &ambientLightIntensity, 0.0f, 2.0f);
		ImGui::ColorEdit3("Bottom color", reinterpret_cast<float*>(&ambientLightBottomColor));
		ImGui::ColorEdit3Srgb("Bottom color (SRGB)", reinterpret_cast<float*>(&ambientLightBottomColor));
		ImGui::ColorEdit3("Top color", reinterpret_cast<float*>(&ambientLightTopColor));
		ImGui::ColorEdit3Srgb("Top color (SRGB)", reinterpret_cast<float*>(&ambientLightTopColor));
		ImGui::Indent();
		if (ImGui::CollapsingHeader("SSAO"))
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
		ImGui::Unindent();
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
		if (ImGui::ColorEdit3("Color", reinterpret_cast<float*>(&mainLightColor)))
			mainLight->SetColor(mainLightColor);
		if (ImGui::ColorEdit3Srgb("Color (SRGB)", reinterpret_cast<float*>(&mainLightColor)))
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