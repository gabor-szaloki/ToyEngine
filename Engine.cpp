#include "Engine.h"

#include "Triangle.h"
#include "Box.h"

Engine *gEngine;

Engine::Engine()
{
	time = deltaTime = 0.0f;
	camera = new Camera();
	ZeroMemory(&cameraInputState, sizeof(CameraInputState));
	cameraMoveSpeed = 5.0f;
	cameraTurnSpeed = 2.0f;
}

Engine::~Engine()
{
	delete camera;
}

void Engine::Init(HWND hWnd, float viewportWidth, float viewportHeight)
{
	InitD3D(hWnd);
	InitViewport(viewportWidth, viewportHeight);
	InitPipeline();
	InitScene();
}

void Engine::Release()
{
	ReleaseScene();
	ReleasePipeline();
	ReleaseD3D();
}

void Engine::InitD3D(HWND hWnd)
{
	// Direct3D initialization

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

	// Set the render target

	ID3D11Texture2D *pBackBuffer;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	device->CreateRenderTargetView(pBackBuffer, nullptr, &backbuffer);
	pBackBuffer->Release();

	context->OMSetRenderTargets(1, &backbuffer, nullptr);
}

void Engine::ReleaseD3D()
{
	SAFE_RELEASE(swapchain);
	SAFE_RELEASE(backbuffer);
	SAFE_RELEASE(device);
	SAFE_RELEASE(context);
}

void Engine::InitViewport(float w, float h)
{
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = w;
	viewport.Height = h;

	context->RSSetViewports(1, &viewport);

	camera->SetProjectionParams(w, h, camera->GetFOV(), camera->GetNearPlane(), camera->GetFarPlane());
}

void Engine::InitPipeline()
{
	ID3DBlob *vsBlob, *psBlob, *errorBlob;

	HRESULT hr;

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

	context->VSSetShader(standardVS, nullptr, 0);
	context->PSSetShader(standardOpaquePS, nullptr, 0);

	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	hr = device->CreateInputLayout(ied, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &standardInputLayout);

	context->IASetInputLayout(standardInputLayout);

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

void Engine::ReleasePipeline()
{
	SAFE_RELEASE(perFrameCB);
	SAFE_RELEASE(perObjectCB);
	SAFE_RELEASE(standardInputLayout);
	SAFE_RELEASE(standardVS);
	SAFE_RELEASE(standardOpaquePS);
}

void Engine::InitScene()
{
	triangle = new Triangle();
	triangle->Init(device, context);

	box = new Box();
	box->Init(device, context);
}

void Engine::ReleaseScene()
{
	delete triangle;
	delete box;
}

void Engine::Update(float elapsedTime)
{
	deltaTime = elapsedTime - time;
	time = elapsedTime;

	XMVECTOR cameraMoveDir =
		camera->GetRight() * ((cameraInputState.isMovingRight ? 1.0f : 0.0f) + (cameraInputState.isMovingLeft ? -1.0f : 0.0f)) +
		camera->GetUp() * ((cameraInputState.isMovingUp ? 1.0f : 0.0f) + (cameraInputState.isMovingDown ? -1.0f : 0.0f)) +
		camera->GetForward() * ((cameraInputState.isMovingForward ? 1.0f : 0.0f) + (cameraInputState.isMovingBackward ? -1.0f : 0.0f));
	XMVECTOR cameraMoveDelta = cameraMoveDir * cameraMoveSpeed * deltaTime;
	if (cameraInputState.isSpeeding)
		cameraMoveDelta *= 2.0f;
	camera->MoveEye(cameraMoveDelta);

	camera->Rotate(cameraInputState.deltaPitch * cameraTurnSpeed * deltaTime, cameraInputState.deltaYaw * cameraTurnSpeed * deltaTime);
	//camera->Rotate(0, 1 * cameraTurnSpeed * deltaTime);
	cameraInputState.deltaPitch = cameraInputState.deltaYaw = 0;
}

void Engine::RenderFrame()
{
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	context->ClearRenderTargetView(backbuffer, clearColor);

	PerFrameConstantBufferData perFrameCBData;
	perFrameCBData.view = camera->GetViewMatrix();
	perFrameCBData.projection = camera->GetProjectionMatrix();
	context->UpdateSubresource(perFrameCB, 0, nullptr, &perFrameCBData, 0, 0);
	context->VSSetConstantBuffers(0, 1, &perFrameCB);

	//box->worldTransform = XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), time);
	//box->worldTransform = XMMatrixTranslation(1, 2, 3);
	box->Draw(context, perObjectCB);

	triangle->worldTransform = XMMatrixRotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), time);
	triangle->Draw(context, perObjectCB);

	swapchain->Present(0, 0);
}