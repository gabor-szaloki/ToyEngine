#include "Gui.h"

#include "Engine.h"

#include "3rdParty/ImGui/imgui.h"
#include "3rdParty/ImGui/imgui_impl_win32.h"
#include "3rdParty/ImGui/imgui_impl_dx11.h"

#include <3rdParty/glm/glm.hpp>

Gui::Gui()
{
	hWnd = 0;

	ZeroMemory(&guiState, sizeof(GuiState));
	guiState.enabled = true;
	guiState.showEngineSettingsWindow = true;
	guiState.showLightSettingsWindow = true;
	guiState.showStatsWindow = true;
	guiState.showShadowmapDebugWindow = false;
}

Gui::~Gui()
{
}

void Gui::Init(HWND hWnd, ID3D11Device *device, ID3D11DeviceContext *context)
{
	this->hWnd = hWnd;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device, context);
}

void Gui::Release()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Gui::Update()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (!guiState.enabled)
	{
		ImGui::Render();
		return;
	}

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("App"))
		{
			if (ImGui::MenuItem("Toggle fullscreen", "F11")) { PostMessage(hWnd, WM_KEYDOWN, VK_F11, 0); }
			if (ImGui::MenuItem("Exit", "ALT+F4")) { PostMessage(hWnd, WM_CLOSE, 0, 0); }
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("Engine settings", nullptr)) { guiState.showEngineSettingsWindow = true; }
			if (ImGui::MenuItem("Light settings", nullptr)) { guiState.showLightSettingsWindow = true; }
			if (ImGui::MenuItem("Stats", nullptr)) { guiState.showStatsWindow = true; }
			if (ImGui::MenuItem("Shadowmap debug", nullptr)) { guiState.showShadowmapDebugWindow = true; }
			ImGui::Separator();
			if (ImGui::MenuItem("ImGui demo window", nullptr)) { guiState.showDemoWindow = true; }
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	if (guiState.showDemoWindow)
		ImGui::ShowDemoWindow(&guiState.showDemoWindow);

	if (guiState.showEngineSettingsWindow)
	{
		ImGui::Begin("Engine settings", &guiState.showEngineSettingsWindow, ImGuiWindowFlags_None);
		ImGui::Checkbox("Show wireframe", &gEngine->showWireframe);
		ImGui::Checkbox("Anisotropic filtering", &gEngine->anisotropicFilteringEnabled);
		bool vsyncEnabled = gEngine->vsync > 0;
		ImGui::Checkbox("V-Sync", &vsyncEnabled);
		gEngine->vsync = vsyncEnabled ? 1 : 0;
		//if (ImGui::Button("Recompile shaders"))
		//	RecompileShaders();
		ImGui::End();
	}

	if (guiState.showLightSettingsWindow)
	{
		ImGui::Begin("Light settings", &guiState.showLightSettingsWindow, ImGuiWindowFlags_None);

		ImGui::Text("Ambient light");
		ImGui::DragFloat("Intensity##ambient", &gEngine->ambientLightIntensity, 0.001f, 0.0f, 1.0f);
		ImGui::ColorEdit3("Color##ambient", reinterpret_cast<float *>(&gEngine->ambientLightColor));

		ImGui::Text("Main light");
		Light *mainLight = gEngine->mainLight;
		float mainLightYaw = mainLight->GetYaw();
		float mainLightPitch = mainLight->GetPitch();
		float mainLightIntensity = mainLight->GetIntensity();
		XMFLOAT4 mainLightColor = mainLight->GetColor();
		if (ImGui::SliderAngle("Yaw", &mainLightYaw, -180.0f, 180.0f))
			mainLight->SetYaw(mainLightYaw);
		if (ImGui::SliderAngle("Pitch", &mainLightPitch, 0.0f, 90.0f))
			mainLight->SetPitch(mainLightPitch);
		if (ImGui::DragFloat("Intensity##main", &mainLightIntensity, 0.001f, 0.0f, 2.0f))
			mainLight->SetIntensity(mainLightIntensity);
		if (ImGui::ColorEdit3("Color##main", reinterpret_cast<float *>(&mainLightColor)))
			mainLight->SetColor(mainLightColor);

		ImGui::Text("Main light shadows");
		ImGui::DragFloat("Shadow distance", &gEngine->shadowDistance, 0.1f, 1.0f, 100.0f);
		ImGui::DragFloat("Directional distance", &gEngine->directionalShadowDistance, 0.1f, 1.0f, 100.0f);
		if (ImGui::InputInt("Resolution", &gEngine->shadowResolution, 512, 1024))
			gEngine->SetShadowResolution((int)fmaxf((float)gEngine->shadowResolution, 128));
		bool setShadowBias = false;
		setShadowBias |= ImGui::InputInt("Depth bias", &gEngine->shadowDepthBias, 1000, 10000);
		setShadowBias |= ImGui::InputFloat("Slope scaled depth bias", &gEngine->shadowSlopeScaledDepthBias, 0.1f, 0.5f, 3);
		if (setShadowBias)
			gEngine->SetShadowBias(gEngine->shadowDepthBias, gEngine->shadowSlopeScaledDepthBias);

		if (ImGui::Button("Show shadowmap"))
			guiState.showShadowmapDebugWindow = true;

		ImGui::End();
	}

	if (guiState.showStatsWindow)
	{
		ImGui::Begin("Stats", &guiState.showEngineSettingsWindow, ImGuiWindowFlags_None);
		float fps = ImGui::GetIO().Framerate;
		ImGui::Text("FPS:           %.0f", fps);
		ImGui::Text("Frame time:    %.1f ms", 1000.0f / fps);
		ImGui::End();
	}

	if (guiState.showShadowmapDebugWindow)
	{
		ImGui::Begin("Shadowmap debug", &guiState.showShadowmapDebugWindow, ImGuiWindowFlags_HorizontalScrollbar);
		static float zoom = 512.0f / gEngine->shadowResolution;
		ImGui::SliderFloat("Zoom", &zoom, 0.1f, 5.0f);
		ImGui::SameLine();
		if (ImGui::Button("Default"))
			zoom = 512.0f / gEngine->shadowResolution;
		ImGui::SameLine();
		if (ImGui::Button("1.000x"))
			zoom = 1.0f;
		ImGui::Image((void *)gEngine->shadowmapSRV, ImVec2(zoom * gEngine->shadowResolution, zoom * gEngine->shadowResolution));
		ImGui::End();
	}
}

void Gui::Render()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
