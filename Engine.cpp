#include "Engine.h"

#include "Privitives.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"

#include "LodePNG/lodepng.h"

Engine *gEngine;

Engine::Engine()
{
	time = deltaTime = 0.0f;
	
	camera = new Camera();
	camera->SetEye(XMVectorSet(2.0f, 3.5f, -3.0f, 0.0f));
	camera->SetRotation(XM_PI / 6, -XM_PIDIV4);

	ZeroMemory(&cameraInputState, sizeof(CameraInputState));
	cameraMoveSpeed = 5.0f;
	cameraTurnSpeed = 0.002f;

	ambientLightIntensity = 0.5f;
	ambientLightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	
	shadowResolution = 2048;
	shadowDistance = 20.0f;
	directionalShadowDistance = 20.0f;
	shadowDepthBias = 50000;
	shadowSlopeScaledDepthBias = 1.0f;

	mainLight = new Light();
	mainLight->SetRotation(-XM_PI / 3, XM_PI / 3);

	ZeroMemory(&guiState, sizeof(GuiState));
	guiState.enabled = true;
	guiState.showEngineSettingsWindow = true;
	guiState.showLightSettingsWindow = true;
	guiState.showStatsWindow = true;
	guiState.showShadowmapDebugWindow = false;

	showWireframe = false;
	anisotropicFilteringEnabled = true;
	vsync = 1;
}

Engine::~Engine()
{
	delete mainLight;
	delete camera;
}

void Engine::Init(HWND hWnd, float viewportWidth, float viewportHeight)
{
	this->hWnd = hWnd;

	InitDevice();
	InitRenderTargets(viewportWidth, viewportHeight);
	InitImGui();
	InitPipeline();
	InitScene();
}

void Engine::Release()
{
	ReleaseScene();
	ReleasePipeline();
	ReleaseImGui();
	ReleaseRenderTargets();
	ReleaseDevice();
}

void Engine::InitDevice()
{
	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = 1;
	scd.Windowed = true;

	HRESULT hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
		&scd, &swapchain, &device, nullptr, &context);
}

void Engine::ReleaseDevice()
{
	SAFE_RELEASE(swapchain);
	SAFE_RELEASE(device);
	SAFE_RELEASE(context);
}

void Engine::InitRenderTargets(float width, float height)
{
	// Setup viewport

	ZeroMemory(&forwardPassViewport, sizeof(D3D11_VIEWPORT));
	forwardPassViewport.MinDepth = 0;
	forwardPassViewport.MaxDepth = 1;
	forwardPassViewport.TopLeftX = 0;
	forwardPassViewport.TopLeftY = 0;
	forwardPassViewport.Width = width;
	forwardPassViewport.Height = height;

	context->RSSetViewports(1, &forwardPassViewport);
	camera->SetProjectionParams(width, height, camera->GetFOV(), camera->GetNearPlane(), camera->GetFarPlane());

	// Create backbuffer render target view

	ID3D11Texture2D *backBufferTexture;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferTexture);
	device->CreateRenderTargetView(backBufferTexture, nullptr, &backbuffer);
	backBufferTexture->Release();

	// Create depth stencil view
	
	ID3D11Texture2D *depthStencilTexture;
	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(D3D11_TEXTURE2D_DESC));
	td.Width = (UINT)width;
	td.Height = (UINT)height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	td.CPUAccessFlags = 0;
	td.MiscFlags = 0;
	device->CreateTexture2D(&td, nullptr, &depthStencilTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ZeroMemory(&dsvd, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	dsvd.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvd.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(depthStencilTexture, &dsvd, &depthStencilView);
	depthStencilTexture->Release();

	// Create depth stencil state

	D3D11_DEPTH_STENCIL_DESC dsd;
	ZeroMemory(&dsd, sizeof(D3D11_DEPTH_STENCIL_DESC));
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;
	dsd.StencilEnable = false;

	device->CreateDepthStencilState(&dsd, &depthStencilState);
	context->OMSetDepthStencilState(depthStencilState, 1);

	context->OMSetRenderTargets(1, &backbuffer, depthStencilView);

	D3D11_RASTERIZER_DESC wireframeRD;
	ZeroMemory(&wireframeRD, sizeof(D3D11_RASTERIZER_DESC));
	wireframeRD.FillMode = D3D11_FILL_WIREFRAME;
	wireframeRD.CullMode = D3D11_CULL_NONE;
	device->CreateRasterizerState(&wireframeRD, &wireframeRasterizerState);

	InitShadowmap();
}

void Engine::ReleaseRenderTargets()
{
	ReleaseShadowmap();

	SAFE_RELEASE(backbuffer);
	SAFE_RELEASE(depthStencilView);
	SAFE_RELEASE(depthStencilState);
}

void Engine::InitShadowmap()
{
	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Width = shadowResolution;
	td.Height = shadowResolution;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R32_TYPELESS;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	td.CPUAccessFlags = 0;
	td.MiscFlags = 0;

	ID3D11Texture2D *shadowmapTexture;
	HRESULT result = device->CreateTexture2D(&td, nullptr, &shadowmapTexture);
	if (FAILED(result))
	{
		OutputDebugString("issue creating shadowmap texture\n");
		throw;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ZeroMemory(&dsvd, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
	dsvd.Format = DXGI_FORMAT_D32_FLOAT;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvd.Texture2D.MipSlice = 0;
	result = device->CreateDepthStencilView(shadowmapTexture, &dsvd, &shadowmapDSV);
	if (FAILED(result))
	{
		OutputDebugString("issue creating shadowmap DSV\n");
		throw;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	result = device->CreateShaderResourceView(shadowmapTexture, &srvDesc, &shadowmapSRV);
	if (FAILED(result))
	{
		OutputDebugString("issue creating shadowmap SRV\n");
		throw;
	}

	ZeroMemory(&shadowPassViewport, sizeof(D3D11_VIEWPORT));
	shadowPassViewport.MinDepth = 0;
	shadowPassViewport.MaxDepth = 1;
	shadowPassViewport.TopLeftX = 0;
	shadowPassViewport.TopLeftY = 0;
	shadowPassViewport.Width = (float)shadowResolution;
	shadowPassViewport.Height = (float)shadowResolution;

	D3D11_RASTERIZER_DESC shadowPassRD;
	ZeroMemory(&shadowPassRD, sizeof(D3D11_RASTERIZER_DESC));
	shadowPassRD.FillMode = D3D11_FILL_SOLID;
	shadowPassRD.CullMode = D3D11_CULL_BACK;
	shadowPassRD.DepthBias = 1024 * shadowDepthBias / shadowResolution;
	shadowPassRD.SlopeScaledDepthBias = shadowSlopeScaledDepthBias;
	result = device->CreateRasterizerState(&shadowPassRD, &shadowPassRasterizerState);
	if (FAILED(result))
	{
		OutputDebugString("issue creating shadow pass rasterizer state\n");
		throw;
	}

	shadowmapTexture->Release();
}

void Engine::ReleaseShadowmap()
{
	SAFE_RELEASE(shadowPassRasterizerState);

	SAFE_RELEASE(shadowmapSRV);
	SAFE_RELEASE(shadowmapDSV);
}

void Engine::InitImGui()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device, context);
}

void Engine::ReleaseImGui()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Engine::InitPipeline()
{
	InitShaders();

	HRESULT hr;

	// Constant buffers
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;

		bd.ByteWidth = sizeof(PerFrameConstantBufferData);
		hr = device->CreateBuffer(&bd, nullptr, &perFrameCB);

		bd.ByteWidth = sizeof(PerCameraConstantBufferData);
		hr = device->CreateBuffer(&bd, nullptr, &perCameraCB);

		bd.ByteWidth = sizeof(PerObjectConstantBufferData);
		hr = device->CreateBuffer(&bd, nullptr, &perObjectCB);
	}

	// Samplers
	{
		D3D11_SAMPLER_DESC sd;
		ZeroMemory(&sd, sizeof(D3D11_SAMPLER_DESC));
		sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.MipLODBias = 0;
		sd.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sd.MinLOD = 0;
		sd.MaxLOD = D3D11_FLOAT32_MAX;

		// linear
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		if (FAILED(device->CreateSamplerState(&sd, &samplerLinearWrap)))
			throw;

		// anisotropic
		sd.Filter = D3D11_FILTER_ANISOTROPIC;
		sd.MaxAnisotropy = 16;
		if (FAILED(device->CreateSamplerState(&sd, &samplerAnisotropicWrap)))
			throw;

		// shadow cmp
		ZeroMemory(&sd, sizeof(D3D11_SAMPLER_DESC));
		sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		sd.BorderColor[0] = 1.0f;
		sd.BorderColor[1] = 1.0f;
		sd.BorderColor[2] = 1.0f;
		sd.BorderColor[3] = 1.0f;
		sd.MinLOD = 0.f;
		sd.MaxLOD = D3D11_FLOAT32_MAX;
		sd.MipLODBias = 0.f;
		sd.MaxAnisotropy = 0;
		sd.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
		sd.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		if (FAILED(device->CreateSamplerState(&sd, &samplerShadowCmp)))
			throw;
	}
}

void Engine::ReleasePipeline()
{
	SAFE_RELEASE(samplerLinearWrap);
	SAFE_RELEASE(samplerAnisotropicWrap);
	SAFE_RELEASE(perFrameCB);
	SAFE_RELEASE(perCameraCB);
	SAFE_RELEASE(perObjectCB);

	ReleaseShaders();
}

void Engine::InitShaders()
{
	ID3DBlob *vsBlob;

	// Shaders
	{
		// Forward Pass
		standardForwardVS = CompileVertexShader(L"Shaders\\Standard.shader", "StandardForwardVS", &vsBlob);
		standardOpaqueForwardPS = CompilePixelShader(L"Shaders\\Standard.shader", "StandardOpaqueForwardPS");

		// Shadow Pass
		standardShadowVS = CompileVertexShader(L"Shaders\\Standard.shader", "StandardShadowVS");
		standardOpaqueShadowPS = CompilePixelShader(L"Shaders\\Standard.shader", "StandardOpaqueShadowPS");
	}

	// Input layout
	{
		D3D11_INPUT_ELEMENT_DESC ied[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		HRESULT hr = device->CreateInputLayout(ied, ARRAYSIZE(ied), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &standardInputLayout);
	}

	SAFE_RELEASE(vsBlob);
}

void Engine::ReleaseShaders()
{
	SAFE_RELEASE(standardInputLayout);
	
	SAFE_RELEASE(standardForwardVS);
	SAFE_RELEASE(standardShadowVS);
	SAFE_RELEASE(standardOpaqueForwardPS);
	SAFE_RELEASE(standardOpaqueShadowPS);
}

void Engine::InitScene()
{
	auto* testBaseTexture = LoadTextureFromPNG("Textures\\test_base.png");
	textureRVs.push_back(testBaseTexture);
	auto* testNormalTexture = LoadTextureFromPNG("Textures\\test_nrm.png");
	textureRVs.push_back(testNormalTexture);

	auto* blueTilesBaseTexture = LoadTextureFromPNG("Textures\\Tiles20_base.png");
	textureRVs.push_back(blueTilesBaseTexture);
	auto* blueTilesNormalTexture = LoadTextureFromPNG("Textures\\Tiles20_nrm.png");
	textureRVs.push_back(blueTilesNormalTexture);

	Material *test = new Material(
		standardForwardVS, standardOpaqueForwardPS, 
		standardShadowVS, standardOpaqueShadowPS,
		testBaseTexture, testNormalTexture);
	materials.push_back(test);

	Material *blueTiles = new Material(
		standardForwardVS, standardOpaqueForwardPS,
		standardShadowVS, standardOpaqueShadowPS,
		blueTilesBaseTexture, blueTilesNormalTexture);
	materials.push_back(blueTiles);

	floor = new Primitives::Plane();
	floor->Init(device, context);
	floor->worldTransform = XMMatrixScaling(10.0f, 10.0f, 10.0f);
	floor->material = blueTiles;
	drawables.push_back(floor);

	box = new Primitives::Box();
	box->Init(device, context);
	box->worldTransform = XMMatrixTranslation(-1.5f, 1.5f, 0.0f);
	box->material = blueTiles;
	drawables.push_back(box);

	sphere = new Primitives::Sphere();
	sphere->Init(device, context);
	sphere->worldTransform = XMMatrixTranslation(1.5f, 1.5f, 0.0f);
	sphere->material = blueTiles;
	drawables.push_back(sphere);
}

void Engine::ReleaseScene()
{
	for (size_t i = 0; i < drawables.size(); i++)
		delete drawables[i];
	drawables.clear();

	for (size_t i = 0; i < materials.size(); i++)
		delete materials[i];
	materials.clear();

	for (size_t i = 0; i < textureRVs.size(); i++)
		SAFE_RELEASE(textureRVs[i]);
	textureRVs.clear();
}

void Engine::Resize(HWND hWnd, float width, float height)
{
	context->OMSetRenderTargets(0, 0, 0);
	ReleaseRenderTargets();
	swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	InitRenderTargets(width, height);
}

void Engine::SetShadowResolution(int shadowResolution)
{
	this->shadowResolution = shadowResolution;
	ReleaseShadowmap();
	InitShadowmap();
}

void Engine::SetShadowBias(int depthBias, float slopeScaledDepthBias)
{
	shadowDepthBias = depthBias;
	shadowSlopeScaledDepthBias = slopeScaledDepthBias;
	ReleaseShadowmap();
	InitShadowmap();
}

void Engine::RecompileShaders()
{
	ReleaseShaders();
	InitShaders();
}

void Engine::Update(float deltaTime)
{
	// Update time
	this->deltaTime = deltaTime;
	time += deltaTime;;

	// Update camera movement
	XMVECTOR cameraMoveDir =
		camera->GetRight() * ((cameraInputState.isMovingRight ? 1.0f : 0.0f) + (cameraInputState.isMovingLeft ? -1.0f : 0.0f)) +
		camera->GetUp() * ((cameraInputState.isMovingUp ? 1.0f : 0.0f) + (cameraInputState.isMovingDown ? -1.0f : 0.0f)) +
		camera->GetForward() * ((cameraInputState.isMovingForward ? 1.0f : 0.0f) + (cameraInputState.isMovingBackward ? -1.0f : 0.0f));
	XMVECTOR cameraMoveDelta = cameraMoveDir * cameraMoveSpeed * deltaTime;
	if (cameraInputState.isSpeeding)
		cameraMoveDelta *= 2.0f;
	camera->MoveEye(cameraMoveDelta);

	// Update camera rotation
	camera->Rotate(cameraInputState.deltaPitch * cameraTurnSpeed, cameraInputState.deltaYaw * cameraTurnSpeed);
	cameraInputState.deltaPitch = cameraInputState.deltaYaw = 0;

	// Update scene
	box->worldTransform = XMMatrixRotationY(time * 0.05f) * XMMatrixTranslation(-1.5f, 1.5f, 0.0f);
	sphere->worldTransform = XMMatrixRotationY(time * 0.05f) * XMMatrixTranslation(1.5f, 1.5f, 0.0f);

	// Update ImGui
	UpdateGUI();
}

void Engine::UpdateGUI()
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
		ImGui::Checkbox("Show wireframe", &showWireframe);
		ImGui::Checkbox("Anisotropic filtering", &anisotropicFilteringEnabled);
		bool vsyncEnabled = vsync > 0;
		ImGui::Checkbox("V-Sync", &vsyncEnabled);
		vsync = vsyncEnabled ? 1 : 0;
		//if (ImGui::Button("Recompile shaders"))
		//	RecompileShaders();
		ImGui::End();
	}

	if (guiState.showLightSettingsWindow)
	{
		ImGui::Begin("Light settings", &guiState.showLightSettingsWindow, ImGuiWindowFlags_None);

		ImGui::Text("Ambient light");
		ImGui::DragFloat("Intensity##ambient", &ambientLightIntensity, 0.001f, 0.0f, 1.0f);
		ImGui::ColorEdit3("Color##ambient", reinterpret_cast<float*>(&ambientLightColor));

		ImGui::Text("Main light");
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
		ImGui::DragFloat("Shadow distance", &shadowDistance, 0.1f, 1.0f, 100.0f);
		ImGui::DragFloat("Directional distance", &directionalShadowDistance, 0.1f, 1.0f, 100.0f);
		if (ImGui::InputInt("Resolution", &shadowResolution, 512, 1024))
			SetShadowResolution((int)fmaxf(shadowResolution, 128));
		bool setShadowBias = false;
		setShadowBias |= ImGui::InputInt("Depth bias", &shadowDepthBias, 1000, 10000);
		setShadowBias |= ImGui::InputFloat("Slope scaled depth bias", &shadowSlopeScaledDepthBias, 0.1f, 0.5f, 3);
		if (setShadowBias)
			SetShadowBias(shadowDepthBias, shadowSlopeScaledDepthBias);

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
		static float zoom = 512.0f / shadowResolution;
		ImGui::SliderFloat("Zoom", &zoom, 0.1f, 5.0f);
		ImGui::SameLine();
		if (ImGui::Button("Default"))
			zoom = 512.0f / shadowResolution;
		ImGui::SameLine();
		if (ImGui::Button("1.000x"))
			zoom = 1.0f;
		ImGui::Image((void *)shadowmapSRV, ImVec2(zoom * shadowResolution, zoom * shadowResolution));
		ImGui::End();
	}
	
	// Rendering
	ImGui::Render();
}

void Engine::RenderFrame()
{
	context->IASetInputLayout(standardInputLayout);

	PerFrameConstantBufferData perFrameCBData;
	perFrameCBData.ambientLightColor = GetFinalLightColor(ambientLightColor, ambientLightIntensity);
	perFrameCBData.mainLightColor = GetFinalLightColor(mainLight->GetColor(), mainLight->GetIntensity());
	// TODO: move shadow frustum with camera frustum
	//XMMATRIX mainLightTranslateMatrix = XMMatrixTranslationFromVector(camera->GetEye() + camera->GetForward() * shadowDistance * 0.5f);
	XMMATRIX inverseLightViewMatrix = XMMatrixRotationRollPitchYaw(mainLight->GetPitch(), mainLight->GetYaw(), 0.0f);
	XMStoreFloat4(&perFrameCBData.mainLightDirection, inverseLightViewMatrix.r[2]);
	XMMATRIX lightViewMatrix = XMMatrixInverse(nullptr, inverseLightViewMatrix);
	XMMATRIX lightProjectionMatrix = XMMatrixOrthographicLH(shadowDistance, shadowDistance, -directionalShadowDistance, directionalShadowDistance);
	// TODO: precalc shadow matrix on cpu
	//perFrameCBData.mainLightShadowMatrix = GetShadowMatrix(lightViewMatrix, lightProjectionMatrix);
	perFrameCBData.mainLightView = lightViewMatrix;
	perFrameCBData.mainLightProjection = lightProjectionMatrix;

	context->UpdateSubresource(perFrameCB, 0, nullptr, &perFrameCBData, 0, 0);
	context->VSSetConstantBuffers(0, 1, &perFrameCB);
	context->PSSetConstantBuffers(0, 1, &perFrameCB);

	context->PSSetSamplers(0, 1, anisotropicFilteringEnabled ? &samplerAnisotropicWrap : &samplerLinearWrap);
	context->PSSetSamplers(1, 1, &samplerShadowCmp);

	ShadowPass(lightViewMatrix, lightProjectionMatrix);
	ForwardPass();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	swapchain->Present(vsync, 0);
}

void Engine::ShadowPass(XMMATRIX lightViewMatrix, XMMATRIX lightProjectionMatrix)
{
	context->RSSetState(shadowPassRasterizerState);
	context->RSSetViewports(1, &shadowPassViewport);
	context->OMSetRenderTargets(0, nullptr, shadowmapDSV);

	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->ClearRenderTargetView(backbuffer, clearColor);
	context->ClearDepthStencilView(shadowmapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	PerCameraConstantBufferData perCameraCBData;
	perCameraCBData.view = lightViewMatrix;
	perCameraCBData.projection = lightProjectionMatrix;
	XMStoreFloat3(&perCameraCBData.cameraWorldPosition, camera->GetEye());

	context->UpdateSubresource(perCameraCB, 0, nullptr, &perCameraCBData, 0, 0);
	context->VSSetConstantBuffers(1, 1, &perCameraCB);
	context->PSSetConstantBuffers(1, 1, &perCameraCB);

	for (size_t i = 0; i < drawables.size(); i++)
		drawables[i]->Draw(context, perObjectCB, true);
}

void Engine::ForwardPass()
{
	context->RSSetState(showWireframe ? wireframeRasterizerState : nullptr);
	context->RSSetViewports(1, &forwardPassViewport);
	context->OMSetRenderTargets(1, &backbuffer, depthStencilView);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	context->ClearRenderTargetView(backbuffer, clearColor);
	context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	PerCameraConstantBufferData perCameraCBData;
	perCameraCBData.view = camera->GetViewMatrix();
	perCameraCBData.projection = camera->GetProjectionMatrix();
	XMStoreFloat3(&perCameraCBData.cameraWorldPosition, camera->GetEye());
	
	context->UpdateSubresource(perCameraCB, 0, nullptr, &perCameraCBData, 0, 0);
	context->VSSetConstantBuffers(1, 1, &perCameraCB);
	context->PSSetConstantBuffers(1, 1, &perCameraCB);

	context->PSSetShaderResources(2, 1, &shadowmapSRV);

	for (size_t i = 0; i < drawables.size(); i++)
		drawables[i]->Draw(context, perObjectCB, false);
}

XMMATRIX Engine::GetShadowMatrix(XMMATRIX lightView, XMMATRIX lightProjection)
{
	XMMATRIX shadowProjectionMatrix = lightProjection;
	shadowProjectionMatrix = XMMatrixTranspose(shadowProjectionMatrix);
	shadowProjectionMatrix.r[2] *= -1;
	shadowProjectionMatrix = XMMatrixTranspose(shadowProjectionMatrix);
	return shadowProjectionMatrix * lightView;
}

ID3D11ShaderResourceView *Engine::LoadTextureFromPNG(const char *filename)
{
	std::vector<unsigned char> pngData;
	UINT width, height;
	UINT error = lodepng::decode(pngData, width, height, filename);
	if (error != 0)
	{
		char errortext[MAX_PATH];
		sprintf_s(errortext, "error %d %s\n", error, lodepng_error_text(error));
		OutputDebugString(errortext);
	}

	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(D3D11_TEXTURE2D_DESC));
	td.Width = width;
	td.Height = height;
	td.MipLevels = 0;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	td.CPUAccessFlags = 0;
	td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	ID3D11Texture2D *texture2d;
	HRESULT result = device->CreateTexture2D(&td, nullptr, &texture2d);
	if (FAILED(result))
		OutputDebugString("issue creating texture\n");

	context->UpdateSubresource(texture2d, 0, nullptr, &pngData[0], td.Width * 4, 0);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = td.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	ID3D11ShaderResourceView *textureRV;
	result = device->CreateShaderResourceView(texture2d, &srvDesc, &textureRV);
	if (FAILED(result))
		OutputDebugString("issue creating shaderResourceView \n");

	context->GenerateMips(textureRV);

	texture2d->Release();

	return textureRV;
}

XMFLOAT4 Engine::GetFinalLightColor(XMFLOAT4 color, float intensity)
{
	return XMFLOAT4(color.x * intensity, color.y * intensity, color.z * intensity, 1.0f);
}

ID3D11VertexShader *Engine::CompileVertexShader(LPCWSTR fileName, LPCSTR entryPoint, ID3DBlob **outBlob)
{
	ID3DBlob *blob, *errorBlob;

	HRESULT hr = D3DCompileFromFile(fileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "vs_4_0", 0, 0, &blob, &errorBlob);
	if (errorBlob != nullptr)
	{
		OutputDebugString(reinterpret_cast<const char *>(errorBlob->GetBufferPointer()));
		SAFE_RELEASE(errorBlob);
	}
	if (FAILED(hr))
		return nullptr;

	if (outBlob != nullptr)
		*outBlob = blob;

	ID3D11VertexShader *shader;
	hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);
	if (FAILED(hr))
		return nullptr;

	return shader;
}

ID3D11PixelShader *Engine::CompilePixelShader(LPCWSTR fileName, LPCSTR entryPoint, ID3DBlob **outBlob)
{
	ID3DBlob *blob, *errorBlob;

	HRESULT hr = D3DCompileFromFile(fileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "ps_4_0", 0, 0, &blob, &errorBlob);
	if (errorBlob)
	{
		OutputDebugString(reinterpret_cast<const char *>(errorBlob->GetBufferPointer()));
		SAFE_RELEASE(errorBlob);
	}
	if (FAILED(hr))
		return nullptr;

	if (outBlob != nullptr)
		*outBlob = blob;
	
	ID3D11PixelShader *shader;
	hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);
	if (FAILED(hr))
		return nullptr;

	return shader;
}