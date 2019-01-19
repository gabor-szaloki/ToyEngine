#include "Engine.h"

#include "Box.h"
#include "Plane.h"

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
	cameraTurnSpeed = 2.0f;

	ambientLightIntensity = 0.5f;
	ambientLightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mainLightIntensity = 1.0f;
	mainLightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mainLightYaw = -XM_PI / 3;
	mainLightPitch = XM_PI / 3;

	ZeroMemory(&guiState, sizeof(GuiState));
	guiState.enabled = true;
	guiState.showEngineSettingsWindow = true;
	guiState.showLightSettingsWindow = true;

	anisotropicFilteringEnabled = true;
}

Engine::~Engine()
{
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

	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width;
	viewport.Height = height;

	context->RSSetViewports(1, &viewport);
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
}

void Engine::ReleaseRenderTargets()
{
	SAFE_RELEASE(backbuffer);
	SAFE_RELEASE(depthStencilView);
	SAFE_RELEASE(depthStencilState);
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
	HRESULT hr;
	ID3DBlob *vsBlob, *psBlob, *errorBlob;

	// Shaders
	{
		hr = D3DCompileFromFile(L"Shaders\\Standard.shader", nullptr, nullptr, "StandardVS", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
		if (errorBlob)
		{
			OutputDebugString(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
			errorBlob = nullptr;
		}
		if (FAILED(hr))
			throw;

		hr = D3DCompileFromFile(L"Shaders\\Standard.shader", nullptr, nullptr, "StandardOpaquePS", "ps_4_0", 0, 0, &psBlob, &errorBlob);
		if (errorBlob)
		{
			OutputDebugString(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
			errorBlob = nullptr;
		}
		if (FAILED(hr))
			throw;

		hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &standardVS);
		hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &standardOpaquePS);
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

		hr = device->CreateInputLayout(ied, ARRAYSIZE(ied), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &standardInputLayout);
	}

	// Constant buffers
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(PerFrameConstantBufferData);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;

		hr = device->CreateBuffer(&bd, nullptr, &perFrameCB);

		bd.ByteWidth = sizeof(PerObjectConstantBufferData);

		hr = device->CreateBuffer(&bd, nullptr, &perObjectCB);
	}

	// Samplers
	{
		D3D11_SAMPLER_DESC sd;
		ZeroMemory(&sd, sizeof(D3D11_SAMPLER_DESC));
		sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sd.MipLODBias = 0;
		sd.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sd.MinLOD = 0;
		sd.MaxLOD = D3D11_FLOAT32_MAX;

		if (FAILED(device->CreateSamplerState(&sd, &samplerLinearWrap)))
			throw;

		sd.Filter = D3D11_FILTER_ANISOTROPIC;
		sd.MaxAnisotropy = 16;

		if (FAILED(device->CreateSamplerState(&sd, &samplerAnisotropicWrap)))
			throw;
	}
}

void Engine::ReleasePipeline()
{
	SAFE_RELEASE(samplerLinearWrap);
	SAFE_RELEASE(samplerAnisotropicWrap);
	SAFE_RELEASE(perFrameCB);
	SAFE_RELEASE(perObjectCB);
	SAFE_RELEASE(standardInputLayout);
	SAFE_RELEASE(standardVS);
	SAFE_RELEASE(standardOpaquePS);
}

void Engine::InitScene()
{
	auto* testBaseTexture = LoadTextureFromPNG("Textures\\test_base.png");
	textureRVs.push_back(testBaseTexture);
	auto* testNormalTexture = LoadTextureFromPNG("Textures\\test_nrm.png");
	textureRVs.push_back(testNormalTexture);

	auto* blueTilesBaseTexture = LoadTextureFromPNG("Textures\\Tiles20_col_smooth.png");
	textureRVs.push_back(blueTilesBaseTexture);
	auto* blueTilesNormalTexture = LoadTextureFromPNG("Textures\\Tiles20_nrm.png");
	textureRVs.push_back(blueTilesNormalTexture);

	Material *test = new Material(standardVS, standardOpaquePS, testBaseTexture, testNormalTexture);
	materials.push_back(test);

	Material *blueTiles = new Material(standardVS, standardOpaquePS, blueTilesBaseTexture, blueTilesNormalTexture);
	materials.push_back(blueTiles);

	box = new Box();
	box->Init(device, context);
	box->worldTransform = XMMatrixTranslation(0.0f, 1.5f, 0.0f);
	box->material = blueTiles;
	drawables.push_back(box);

	floor = new Plane();
	floor->Init(device, context);
	floor->worldTransform = XMMatrixScaling(10.0f, 1.0f, 10.0f);
	floor->material = blueTiles;
	drawables.push_back(floor);
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

void Engine::Update(float elapsedTime)
{
	// Update time
	deltaTime = elapsedTime - time;
	time = elapsedTime;

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
	camera->Rotate(cameraInputState.deltaPitch * cameraTurnSpeed * deltaTime, cameraInputState.deltaYaw * cameraTurnSpeed * deltaTime);
	cameraInputState.deltaPitch = cameraInputState.deltaYaw = 0;

	// Update scene
	box->worldTransform = XMMatrixRotationY(elapsedTime * 0.05f) * XMMatrixTranslation(0.0f, 1.5f, 0.0f);

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
		ImGui::Checkbox("Anisotropic filtering", &anisotropicFilteringEnabled);
		ImGui::End();
	}

	if (guiState.showLightSettingsWindow)
	{
		ImGui::Begin("Light settings", &guiState.showLightSettingsWindow, ImGuiWindowFlags_None);
		ImGui::Text("Ambient light");
		ImGui::SliderFloat("Intensity##ambient", &ambientLightIntensity, 0.0f, 1.0f);
		ImGui::ColorEdit3("Color##ambient", reinterpret_cast<float*>(&ambientLightColor));
		ImGui::Text("Main light");
		ImGui::SliderAngle("Yaw", &mainLightYaw, -180.0f, 180.0f);
		ImGui::SliderAngle("Pitch", &mainLightPitch, 0.0f, 90.0f);
		ImGui::SliderFloat("Intensity##main", &mainLightIntensity, 0.0f, 2.0f);
		ImGui::ColorEdit3("Color##main", reinterpret_cast<float*>(&mainLightColor));
		ImGui::End();
	}
	
	// Rendering
	ImGui::Render();
}

void Engine::RenderFrame()
{
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	context->ClearRenderTargetView(backbuffer, clearColor);
	context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	context->IASetInputLayout(standardInputLayout);

	PerFrameConstantBufferData perFrameCBData;
	perFrameCBData.view = camera->GetViewMatrix();
	perFrameCBData.projection = camera->GetProjectionMatrix();
	perFrameCBData.ambientLightColor = GetFinalLightColor(ambientLightColor, ambientLightIntensity);
	perFrameCBData.mainLightColor = GetFinalLightColor(mainLightColor, mainLightIntensity);
	XMMATRIX lightMatrix = XMMatrixRotationRollPitchYaw(mainLightPitch, mainLightYaw, 0.0f);
	XMStoreFloat4(&perFrameCBData.mainLightDirection, lightMatrix.r[2]);
	
	context->UpdateSubresource(perFrameCB, 0, nullptr, &perFrameCBData, 0, 0);
	context->VSSetConstantBuffers(0, 1, &perFrameCB);
	context->PSSetConstantBuffers(0, 1, &perFrameCB);

	context->PSSetSamplers(0, 1, anisotropicFilteringEnabled ? &samplerAnisotropicWrap : &samplerLinearWrap);

	context->VSSetShader(standardVS, nullptr, 0);
	context->PSSetShader(standardOpaquePS, nullptr, 0);

	for (size_t i = 0; i < drawables.size(); i++)
		drawables[i]->Draw(context, perObjectCB);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	swapchain->Present(0, 0);
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
