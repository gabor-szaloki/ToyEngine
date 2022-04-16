#include "DriverD3D12.h"
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

#include <assert.h>
#include <algorithm>
#include <3rdParty/cxxopts/cxxopts.hpp>
#include <3rdParty/imgui/imgui_impl_dx12.h>
#include <3rdParty/smhasher/MurmurHash3.h>

void create_driver_d3d12()
{
	drv = new drv_d3d12::DriverD3D12();
}

namespace drv_d3d12
{

//
// Stuff for the cube demo
// 

// Vertex data for a colored cube.
struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static WORD g_Indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

DriverD3D12& DriverD3D12::get()
{
	return *(DriverD3D12*)drv;
}

static ComPtr<IDXGIAdapter4> get_adapter(bool debug_layer_enabled)
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	uint createFactoryFlags = 0;
	if (debug_layer_enabled)
		createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

	verify(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	size_t maxDedicatedVideoMemory = 0;
	for (uint i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
		dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

		// Check to see if the adapter can create a D3D12 device without actually 
		// creating it. The adapter with the largest dedicated video memory
		// is favored.
		if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
			SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
			dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
			verify(dxgiAdapter1.As(&dxgiAdapter4));
		}
	}

	return dxgiAdapter4;
}

static ComPtr<ID3D12Device2> create_device(ComPtr<IDXGIAdapter4> adapter, bool debug_layer_enabled)
{
	ComPtr<ID3D12Device2> d3d12Device2;
	verify(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

	// Enable debug messages in debug mode.
	if (debug_layer_enabled)
	{
		ComPtr<ID3D12InfoQueue> pInfoQueue;
		if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
		{
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

			// Suppress whole categories of messages
			//D3D12_MESSAGE_CATEGORY Categories[] = {};

			// Suppress messages based on their severity level
			D3D12_MESSAGE_SEVERITY Severities[] =
			{
				D3D12_MESSAGE_SEVERITY_INFO,
				D3D12_MESSAGE_SEVERITY_MESSAGE
			};

			// Suppress individual messages by their ID
			//D3D12_MESSAGE_ID DenyIds[] = {
			//	D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			//	D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			//	D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
			//};

			D3D12_INFO_QUEUE_FILTER NewFilter = {};
			//NewFilter.DenyList.NumCategories = _countof(Categories);
			//NewFilter.DenyList.pCategoryList = Categories;
			NewFilter.DenyList.NumSeverities = _countof(Severities);
			NewFilter.DenyList.pSeverityList = Severities;
			//NewFilter.DenyList.NumIDs = _countof(DenyIds);
			//NewFilter.DenyList.pIDList = DenyIds;

			if (FAILED(pInfoQueue->PushStorageFilter(&NewFilter)))
				PLOG_ERROR << "Failed to push D3D debug layer info queue filter";
		}
	}

	return d3d12Device2;
}

static ComPtr<IDXGISwapChain4> create_swap_chain(HWND hwnd, ComPtr<ID3D12CommandQueue> command_queue, uint width, uint height, DXGI_FORMAT format, uint buffer_count, bool debug_layer_enabled)
{
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
	if (debug_layer_enabled)
		createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

	verify(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = format;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = buffer_count;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ComPtr<IDXGISwapChain1> swapChain1;
	verify(dxgiFactory4->CreateSwapChainForHwnd(
		command_queue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	verify(dxgiFactory4->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

	verify(swapChain1.As(&dxgiSwapChain4));

	return dxgiSwapChain4;
}

static ComPtr<ID3D12DescriptorHeap> create_descriptor_heap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint num_descriptors, uint* out_descriptor_size = nullptr)
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = num_descriptors;
	desc.Type = type;
	if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
		desc.Flags |= D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	verify(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

	if (out_descriptor_size != nullptr)
		*out_descriptor_size = device->GetDescriptorHandleIncrementSize(type);

	return descriptorHeap;
}

bool DriverD3D12::init(void* hwnd, int display_width, int display_height)
{
	hWnd = (HWND)hwnd;
	displayWidth = display_width;
	displayHeight = display_height;

	const bool debug_layer_enabled = get_cmdline_opts()["debug-device"].as<bool>();

	if (debug_layer_enabled)
		enableDebugLayer();

	ComPtr<IDXGIAdapter4> adapter = get_adapter(debug_layer_enabled);
	device = create_device(adapter, debug_layer_enabled);

	directCommandQueue = std::make_unique<CommandQueue>(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	computeCommandQueue = std::make_unique<CommandQueue>(device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	copyCommandQueue = std::make_unique<CommandQueue>(device, D3D12_COMMAND_LIST_TYPE_COPY);

	swapchain = create_swap_chain(hWnd, directCommandQueue->GetD3D12CommandQueue(), displayWidth, displayHeight, SWAPCHAIN_FORMAT, NUM_SWACHAIN_BUFFERS, debug_layer_enabled);
	currentBackBufferIndex = swapchain->GetCurrentBackBufferIndex();

	rtvDescriptorHeap = create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUM_RTV_DESCRIPTORS, &rtvDescriptorSize);
	dsvDescriptorHeap = create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, NUM_DSV_DESCRIPTORS, &dsvDescriptorSize);
	cbvSrvUavDescriptorHeap = create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, NUM_CBV_SRV_UAV_DESCRIPTORS, &cbvSrvUavDescriptorSize);

	initDefaultPipelineStates();

	initResolutionDependentResources();

	ImGui_ImplDX12_Init(device.Get(), NUM_SWACHAIN_BUFFERS, SWAPCHAIN_FORMAT, cbvSrvUavDescriptorHeap.Get(),
		cbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), cbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	loadDemoContent();

	return true;
}

void DriverD3D12::shutdown()
{
	// Make sure the command queues have finished all commands before closing.
	flush();

	destroyDemoContent();

	for (auto& pair : psoHashMap)
		pair.second->Release();
	psoHashMap.clear();

	ImGui_ImplDX12_Shutdown();

	for (int i = 0; i < NUM_SWACHAIN_BUFFERS; ++i)
		frameFenceValues[i] = 0;

	closeResolutionDependentResources();

	cbvSrvUavDescriptorHeap.Reset();
	dsvDescriptorHeap.Reset();
	rtvDescriptorHeap.Reset();

	swapchain.Reset();

	commandList.Reset();

	copyCommandQueue.reset();
	computeCommandQueue.reset();
	directCommandQueue.reset();

	device.Reset();

	if (dxgiDebug != nullptr)
		dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
}

void DriverD3D12::resize(int display_width, int display_height)
{
	if (display_width == displayWidth && display_height == displayHeight)
		return;

	// Flush the command queues to make sure no resource we want to resize is in-flight
	flush();

	closeResolutionDependentResources();

	displayWidth = display_width;
	displayHeight = display_height;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	verify(swapchain->GetDesc(&swapChainDesc));
	verify(swapchain->ResizeBuffers(NUM_SWACHAIN_BUFFERS, displayWidth, displayHeight,
		swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
	currentBackBufferIndex = swapchain->GetCurrentBackBufferIndex();

	initResolutionDependentResources();
}

void DriverD3D12::getDisplaySize(int& display_width, int& display_height)
{
	display_width = displayWidth;
	display_height = displayHeight;
}

ITexture* DriverD3D12::createTexture(const TextureDesc& desc)
{
	ASSERT_NOT_IMPLEMENTED;
	return nullptr;
}

ITexture* DriverD3D12::createTextureStub()
{
	ASSERT_NOT_IMPLEMENTED;
	return nullptr;
}

IBuffer* DriverD3D12::createBuffer(const BufferDesc& desc)
{
	ASSERT_NOT_IMPLEMENTED;
	return nullptr;
}

ResId DriverD3D12::createSampler(const SamplerDesc& desc)
{
	ASSERT_NOT_IMPLEMENTED;
	return BAD_RESID;
}

ResId DriverD3D12::createRenderState(const RenderStateDesc& desc)
{
	ASSERT_NOT_IMPLEMENTED;
	return BAD_RESID;
}

ResId DriverD3D12::createShaderSet(const ShaderSetDesc& desc)
{
	ASSERT_NOT_IMPLEMENTED;
	return BAD_RESID;
}

ResId DriverD3D12::createComputeShader(const ComputeShaderDesc& desc)
{
	ASSERT_NOT_IMPLEMENTED;
	return BAD_RESID;
}

ResId DriverD3D12::createInputLayout(const InputLayoutElementDesc* descs, unsigned int num_descs, ResId shader_set)
{
	ASSERT_NOT_IMPLEMENTED;
	return BAD_RESID;
}

void DriverD3D12::destroyResource(ResId res_id)
{
	// TODO
}

void DriverD3D12::setInputLayout(ResId res_id)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setIndexBuffer(ResId res_id)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setVertexBuffer(ResId res_id)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setConstantBuffer(ShaderStage stage, unsigned int slot, ResId res_id)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setBuffer(ShaderStage stage, unsigned int slot, ResId res_id)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setRwBuffer(unsigned int slot, ResId res_id)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setTexture(ShaderStage stage, unsigned int slot, ResId res_id)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setRwTexture(unsigned int slot, ResId res_id)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setSampler(ShaderStage stage, unsigned int slot, ResId res_id)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setRenderTarget(ResId target_id, ResId depth_id,
	unsigned int target_slice, unsigned int depth_slice, unsigned int target_mip, unsigned int depth_mip)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setRenderTargets(unsigned int num_targets, ResId* target_ids, ResId depth_id,
	unsigned int* target_slices, unsigned int depth_slice, unsigned int* target_mips, unsigned int depth_mip)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setRenderState(ResId res_id)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setShader(ResId res_id, unsigned int variant_index)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setView(float x, float y, float w, float h, float z_min, float z_max)
{
	CD3DX12_VIEWPORT viewport(x, y, w, h, z_min, z_max);
	CD3DX12_RECT scissorRect(0, 0, LONG_MAX, LONG_MAX); // TODO: either support setting scissor, or just set it once per cmdlist

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
}

void DriverD3D12::setView(const ViewportParams& vp)
{
	setView(vp.x, vp.y, vp.w, vp.h, vp.z_min, vp.z_max);
}

void DriverD3D12::getView(ViewportParams& vp)
{
	vp = currentViewport;
}

void DriverD3D12::draw(unsigned int vertex_count, unsigned int start_vertex)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::drawIndexed(unsigned int index_count, unsigned int start_index, int base_vertex)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::dispatch(unsigned int num_threadgroups_x, unsigned int num_threadgroups_y, unsigned int num_threadgroups_z)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::clearRenderTargets(const RenderTargetClearParams clear_params)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::beginFrame()
{
	ImGui_ImplDX12_NewFrame();
}

void DriverD3D12::update(float delta_time)
{
	//
	// update demo cube
	//

	static float totalTime = 0;
	totalTime += delta_time;

	// Update the model matrix.
	float angle = static_cast<float>(totalTime * 90.0);
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
	m_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

	// Update the view matrix.
	const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	// Update the projection matrix.
	float aspectRatio = displayWidth / static_cast<float>(displayHeight);
	m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), aspectRatio, 0.1f, 100.0f);
}

void DriverD3D12::beginRender()
{
	// Begin graphics work
	commandList = directCommandQueue->GetCommandList();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIndex, rtvDescriptorSize);
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// Clear the render target.
	{
		transitionResource(backbuffers[currentBackBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		float clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
	}

	// TODO: remove render target setting from here once we support it via the interface
	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = { rtv };
	commandList->OMSetRenderTargets(1, rtvs, false, &dsv);

	// Render demo cube
	{
		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

		currentGraphicsPipelineState.rootSignature = m_RootSignature.Get();
		currentGraphicsPipelineState.inputLayout = { inputLayout, _countof(inputLayout) };
		currentGraphicsPipelineState.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		currentGraphicsPipelineState.vs = CD3DX12_SHADER_BYTECODE(m_VertexShaderBlob.Get());
		currentGraphicsPipelineState.ps = CD3DX12_SHADER_BYTECODE(m_PixelShaderBlob.Get());
		currentGraphicsPipelineState.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
		currentGraphicsPipelineState.renderTargetFormats = rtvFormats;

		commandList->SetPipelineState(getOrCreateCurrentGraphicsPipeline());
		commandList->SetGraphicsRootSignature(m_RootSignature.Get());

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
		commandList->IASetIndexBuffer(&m_IndexBufferView);

		CD3DX12_VIEWPORT viewport(0.0f, 0.0f, (float)displayWidth, (float)displayHeight, 0.0f, 1.0f);
		commandList->RSSetViewports(1, &viewport);
		D3D12_RECT scissor = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
		commandList->RSSetScissorRects(1, &scissor);

		// Update the MVP matrix
		XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
		mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
		commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

		commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);
	}
}

void DriverD3D12::endFrame()
{
	// Render Dear ImGui graphics
	commandList->SetDescriptorHeaps(1, cbvSrvUavDescriptorHeap.GetAddressOf());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}

void DriverD3D12::present()
{
	transitionResource(backbuffers[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	// Submit graphics work
	frameFenceValues[currentBackBufferIndex] = directCommandQueue->ExecuteCommandList(commandList);

	uint syncInterval = settings.vsync ? 1 : 0;
	uint presentFlags = settings.vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING;
	verify(swapchain->Present(syncInterval, presentFlags));

	currentBackBufferIndex = swapchain->GetCurrentBackBufferIndex();
	directCommandQueue->WaitForFenceValue(frameFenceValues[currentBackBufferIndex]);
}

void DriverD3D12::beginEvent(const char* label)
{
#if PROFILE_MARKERS_ENABLED
	// TODO
#endif
}

void DriverD3D12::endEvent()
{
#if PROFILE_MARKERS_ENABLED
	// TODO
#endif
}

unsigned int DriverD3D12::getShaderVariantIndexForKeywords(ResId shader_res_id, const char** keywords, unsigned int num_keywords)
{
	ASSERT_NOT_IMPLEMENTED;
	return 0;
}

void DriverD3D12::setSettings(const DriverSettings& new_settings)
{
	DriverSettings oldSettings = settings;
	settings = new_settings;

	if (settings.textureFilteringAnisotropy != oldSettings.textureFilteringAnisotropy)
	{
		// TODO
	}
}

void DriverD3D12::recompileShaders()
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::setErrorShaderDesc(const ShaderSetDesc& desc)
{
	ASSERT_NOT_IMPLEMENTED;
}

ResId DriverD3D12::getErrorShader()
{
	ASSERT_NOT_IMPLEMENTED;
	return BAD_RESID;
}

void DriverD3D12::flush()
{
	directCommandQueue->Flush();
	computeCommandQueue->Flush();
	copyCommandQueue->Flush();
}

void DriverD3D12::initDefaultPipelineStates()
{
	currentGraphicsPipelineState.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	currentGraphicsPipelineState.blendState = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
	currentGraphicsPipelineState.rasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());
	currentGraphicsPipelineState.depthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(CD3DX12_DEFAULT());
}

bool DriverD3D12::initResolutionDependentResources()
{
	updateBackbufferRTVs();

	// init depth buffer for demo
	{
		// Create a depth buffer.
		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		optimizedClearValue.DepthStencil = { 1.0f, 0 };

		{
			CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
			CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, displayWidth, displayHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
			verify(device->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&optimizedClearValue,
				IID_PPV_ARGS(&m_DepthBuffer)
			));
		}

		// Update the depth-stencil view.
		D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
		dsv.Format = DXGI_FORMAT_D32_FLOAT;
		dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv.Texture2D.MipSlice = 0;
		dsv.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv,
			dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	}

	return true;
}

void DriverD3D12::closeResolutionDependentResources()
{
	m_DepthBuffer.Reset();

	for (int i = 0; i < NUM_SWACHAIN_BUFFERS; i++)
	{
		backbuffers[i].Reset();
		frameFenceValues[i] = frameFenceValues[currentBackBufferIndex];
	}
}

void DriverD3D12::enableDebugLayer()
{
	ComPtr<ID3D12Debug> debugInterface;
	HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
	if (SUCCEEDED(hr))
	{
		PLOG_INFO << "Enabling debug layer";
		debugInterface->EnableDebugLayer();
	}
	else
		PLOG_ERROR << "Failed to enable debug layer";

	if (FAILED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
	{
		PLOG_ERROR << "Failed to get DXGI debug interface.";
			dxgiDebug = nullptr;
	}
}

void DriverD3D12::updateBackbufferRTVs()
{
	uint rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < NUM_SWACHAIN_BUFFERS; ++i)
	{
		ComPtr<ID3D12Resource> backbuffer;
		verify(swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));

		device->CreateRenderTargetView(backbuffer.Get(), nullptr, rtvHandle);

		backbuffers[i] = backbuffer;

		rtvHandle.Offset(rtvDescriptorSize);
	}
}

void DriverD3D12::transitionResource(ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
	commandList->ResourceBarrier(1, &barrier);
}

void DriverD3D12::updateBufferResource(ID3D12GraphicsCommandList2* cmdList, ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
	const size_t bufferSize = numElements * elementSize;

	// Create a committed resource for the GPU resource in a default heap.
	{
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
		verify(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(pDestinationResource)));
	}

	// Create an committed resource for the upload.
	if (bufferData)
	{
		const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		verify(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pIntermediateResource)));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(cmdList, *pDestinationResource, *pIntermediateResource, 0, 0, 1, &subresourceData);
	}
}

ID3D12PipelineState* DriverD3D12::getOrCreateCurrentGraphicsPipeline()
{
	uint64 pipelineHash = currentGraphicsPipelineState.hash();
	auto findResult = psoHashMap.find(pipelineHash);
	if (findResult != psoHashMap.end())
		return findResult->second;

	PLOG_DEBUG << "Pipeline hash ("<< std::hex << pipelineHash << ") not found. Size of PSO hashmap: " << std::dec << psoHashMap.size();

	ID3D12PipelineState* newPso;
	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(currentGraphicsPipelineState), &currentGraphicsPipelineState };
	verify(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&newPso)));

	psoHashMap[pipelineHash] = newPso;

	return newPso;
}

bool DriverD3D12::loadDemoContent()
{
	auto cmdList = copyCommandQueue->GetCommandList();

	// Upload vertex buffer data.
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	updateBufferResource(cmdList.Get(),
		&m_VertexBuffer, &intermediateVertexBuffer,
		_countof(g_Vertices), sizeof(VertexPosColor), g_Vertices);

	// Create the vertex buffer view.
	m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = sizeof(g_Vertices);
	m_VertexBufferView.StrideInBytes = sizeof(VertexPosColor);

	// Upload index buffer data.
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	updateBufferResource(cmdList.Get(),
		&m_IndexBuffer, &intermediateIndexBuffer,
		_countof(g_Indicies), sizeof(WORD), g_Indicies);

	// Create index buffer view.
	m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
	m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_IndexBufferView.SizeInBytes = sizeof(g_Indicies);

	ComPtr<ID3DBlob> errorBlob;

	// Load the vertex shader.
	HRESULT hr = D3DCompileFromFile(L"Source/Shaders/Experiments/D3D12Test.shader", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VsMain", "vs_5_1", 0, 0, &m_VertexShaderBlob, &errorBlob);
	if (FAILED(hr) && errorBlob == nullptr)
	{
		PLOG_ERROR << "HRESULT: 0x" << std::hex << hr << " " << std::system_category().message(hr);
		return false;
	}
	else if (FAILED(hr) && errorBlob != nullptr)
	{
		PLOG_ERROR << reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
		return false;
	}

	// Load the pixel shader.
	hr = D3DCompileFromFile(L"Source/Shaders/Experiments/D3D12Test.shader", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PsMain", "ps_5_1", 0, 0, &m_PixelShaderBlob, &errorBlob);
	if (FAILED(hr) && errorBlob == nullptr)
	{
		PLOG_ERROR << "HRESULT: 0x" << std::hex << hr << " " << std::system_category().message(hr);
		return false;
	}
	else if (FAILED(hr) && errorBlob != nullptr)
	{
		PLOG_ERROR << reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
		return false;
	}

	// Create the root signature.
	{
		// Before creating the root signature, the highest supported version of the root signature is queried. Root signature version 1.1 should be preferred because it allows for driver level optimizations to be made.
		// Read more at:
		// - https://www.3dgep.com/learning-directx-12-2/#root-signature
		// - https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_descriptor_range_flags

		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		// Allow input layout and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		// A single 32-bit constant root parameter that is used by the vertex shader.
		CD3DX12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

		// Serialize the root signature.
		ComPtr<ID3DBlob> rootSignatureBlob;
		verify(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
			featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
		// Create the root signature.
		verify(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
			rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
	}

	uint64 fenceValue = copyCommandQueue->ExecuteCommandList(cmdList);
	copyCommandQueue->WaitForFenceValue(fenceValue);

	return true;
}

void DriverD3D12::destroyDemoContent()
{
	m_RootSignature.Reset();
	m_IndexBuffer.Reset();
	m_VertexBuffer.Reset();
}

uint64 DriverD3D12::GraphicsPipelineStateStream::hash() const
{
	uint64 result[2];
	MurmurHash3_x64_128(this, static_cast<int>(sizeof(GraphicsPipelineStateStream)), 0, result);
	return result[0];
}


} // namespace drv_d3d12