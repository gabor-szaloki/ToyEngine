#include "DriverD3D12.h"

#include "TextureD3D12.h"
#include "BufferD3D12.h"
#include "RenderStateD3D12.h"
#include "InputLayoutD3D12.h"
#include "GraphicsShaderSet.h"
#include "DescriptorHeap.h"

#include <d3dcompiler.h>
#pragma comment(lib,"d3dcompiler.lib")

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

static ResId nextAvailableResId = 0;

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
			D3D12_MESSAGE_ID DenyIds[] = {
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE, // This warning occurs when a render target is cleared using a clear color that is 
																			  // not the optimized clear color specified during resource creation.If you want to
																			  // clear a render target using an arbitrary clear color, you should disable this warning.
				D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE, // TODO: we should init depth textures with 1.0, or expose optimized clearvalue to TextureDesc
				//D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
				//D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
			};

			D3D12_INFO_QUEUE_FILTER NewFilter = {};
			//NewFilter.DenyList.NumCategories = _countof(Categories);
			//NewFilter.DenyList.pCategoryList = Categories;
			NewFilter.DenyList.NumSeverities = _countof(Severities);
			NewFilter.DenyList.pSeverityList = Severities;
			NewFilter.DenyList.NumIDs = _countof(DenyIds);
			NewFilter.DenyList.pIDList = DenyIds;

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

	rtvHeap = std::make_unique<DescriptorHeap>(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1000);
	dsvHeap = std::make_unique<DescriptorHeap>(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1000);
	cbvSrvUavHeap = std::make_unique<DescriptorHeap>(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000);

	initCapabilities();
	initDefaultPipelineStates();
	initResolutionDependentResources();

	DescriptorHandlePair imguiSrv = cbvSrvUavHeap->allocateDescriptor();
	ImGui_ImplDX12_Init(device.Get(), NUM_SWACHAIN_BUFFERS, SWAPCHAIN_FORMAT, cbvSrvUavHeap->getHeap(), imguiSrv.cpuHandle, imguiSrv.gpuHandle);

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

	errorShader.reset();

	for (int i = 0; i < NUM_SWACHAIN_BUFFERS; ++i)
		frameFenceValues[i] = 0;

	closeResolutionDependentResources();
	releaseAllResources();

	cbvSrvUavHeap.reset();
	dsvHeap.reset();
	rtvHeap.reset();

	swapchain.Reset();

	frameCmdList.Reset();

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
	return new Texture(&desc);
}

ITexture* DriverD3D12::createTextureStub()
{
	return new Texture();
}

IBuffer* DriverD3D12::createBuffer(const BufferDesc& desc)
{
	return new Buffer(desc);
}

ResId DriverD3D12::createSampler(const SamplerDesc& desc)
{
	ASSERT_NOT_IMPLEMENTED;
	return BAD_RESID;
}

ResId DriverD3D12::createRenderState(const RenderStateDesc& desc)
{
	RenderState* rs = new RenderState(desc);
	return rs->getId();
}

ResId DriverD3D12::createShaderSet(const ShaderSetDesc& desc)
{
	GraphicsShaderSet* shaderSet = new GraphicsShaderSet(desc);
	return shaderSet->getId();
}

ResId DriverD3D12::createComputeShader(const ComputeShaderDesc& desc)
{
	ASSERT_NOT_IMPLEMENTED;
	return BAD_RESID;
}

ResId DriverD3D12::createInputLayout(const InputLayoutElementDesc* descs, unsigned int num_descs, ResId /*shader_set*/)
{
	InputLayout* inputLayout = new InputLayout(descs, num_descs);
	return inputLayout->getId();
}

template<typename T>
bool erase_if_found(ResId id, std::unordered_map<ResId, T*>& from)
{
	T* res;
	{
		RESOURCE_LOCK_GUARD;
		auto it = from.find(id);
		if (it == from.end())
			return false;
		res = it->second;
	}
	delete res;
	// resource will remove itself from the pool from dtor
	return true;
}

void DriverD3D12::destroyResource(ResId res_id)
{
	if (erase_if_found(res_id, buffers))
		return;
	if (erase_if_found(res_id, renderStates))
		return;
	if (erase_if_found(res_id, graphicsShaders))
		return;
	if (erase_if_found(res_id, inputLayouts))
		return;
	assert(false);
}

template<typename T>
T* get_resource(ResId res_id, std::unordered_map<ResId, T*>& pool, bool optional = false)
{
	assert(optional || res_id != BAD_RESID);

	if (res_id == BAD_RESID)
		return nullptr;

	RESOURCE_LOCK_GUARD;
	assert(pool.find(res_id) != pool.end());
	return pool[res_id];
}

void DriverD3D12::setInputLayout(ResId res_id)
{
	const InputLayout* inputLayout = get_resource(res_id, inputLayouts);
	const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputElements = inputLayout->getInputElements();
	currentGraphicsPipelineState.inputLayout = { inputElements.data(), (uint)inputElements.size() };
}

void DriverD3D12::setIndexBuffer(ResId res_id)
{
	const Buffer* buf = get_resource(res_id, buffers);

	D3D12_INDEX_BUFFER_VIEW ibv;
	ibv.BufferLocation = buf->getResource()->GetGPUVirtualAddress();
	ibv.SizeInBytes = buf->getDesc().numElements * buf->getDesc().elementByteSize;
	ibv.Format = static_cast<DXGI_FORMAT>(getIndexFormat());

	frameCmdList->IASetIndexBuffer(&ibv);
}

void DriverD3D12::setVertexBuffer(ResId res_id)
{
	const Buffer* buf = get_resource(res_id, buffers);

	D3D12_VERTEX_BUFFER_VIEW vbv;
	vbv.BufferLocation = buf->getResource()->GetGPUVirtualAddress();
	vbv.SizeInBytes = buf->getDesc().numElements * buf->getDesc().elementByteSize;
	vbv.StrideInBytes = buf->getDesc().elementByteSize;

	frameCmdList->IASetVertexBuffers(0, 1, &vbv);
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
	currentRTVs.fill(CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
	currentDSV = CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT());

	Texture* targetTex = get_resource(target_id, textures, true);

	CD3DX12_CPU_DESCRIPTOR_HANDLE* rtv = nullptr;
	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	if (targetTex != nullptr)
	{
		assert(targetTex->getDesc().bindFlags & BIND_RENDER_TARGET);

		targetTex->transition(D3D12_RESOURCE_STATE_RENDER_TARGET);
			
		currentRTVs[0] = targetTex->getRtv(target_slice, target_mip);
		rtv = &currentRTVs[0];

		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = (DXGI_FORMAT)targetTex->getDesc().getRtvFormat();
	}
	currentGraphicsPipelineState.renderTargetFormats = rtvFormats;

	Texture* depthTex = get_resource(depth_id, textures, true);

	CD3DX12_CPU_DESCRIPTOR_HANDLE* dsv = nullptr;
	currentGraphicsPipelineState.depthStencilFormat = DXGI_FORMAT_UNKNOWN;
	if (depthTex != nullptr)
	{
		assert(depthTex->getDesc().bindFlags & BIND_DEPTH_STENCIL);

		depthTex->transition(D3D12_RESOURCE_STATE_DEPTH_WRITE);

		currentDSV = depthTex->getDsv(depth_slice, depth_mip);
		dsv = &currentDSV;

		currentGraphicsPipelineState.depthStencilFormat = (DXGI_FORMAT)depthTex->getDesc().getDsvFormat();
	}

	frameCmdList->OMSetRenderTargets(rtvFormats.NumRenderTargets, rtv, false, dsv);
}

void DriverD3D12::setRenderTargets(unsigned int num_targets, ResId* target_ids, ResId depth_id,
	unsigned int* target_slices, unsigned int depth_slice, unsigned int* target_mips, unsigned int depth_mip)
{
	assert(num_targets <= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

	currentRTVs.fill(CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
	currentDSV = CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT());

	D3D12_RT_FORMAT_ARRAY rtvFormats;
	rtvFormats.NumRenderTargets = num_targets;
	for (uint i = 0; i < num_targets; i++)
	{
		Texture* targetTex = get_resource(target_ids[i], textures);
		assert(targetTex->getDesc().bindFlags & BIND_RENDER_TARGET);

		targetTex->transition(D3D12_RESOURCE_STATE_RENDER_TARGET);

		uint targetSlice = target_slices != nullptr ? target_slices[i] : 0;
		uint targetMip = target_mips != nullptr ? target_mips[i] : 0;

		currentRTVs[i] = targetTex->getRtv(targetSlice, targetMip);
		rtvFormats.RTFormats[i] = (DXGI_FORMAT)targetTex->getDesc().getRtvFormat();
	}

	Texture* depthTex = get_resource(depth_id, textures, true);

	CD3DX12_CPU_DESCRIPTOR_HANDLE* dsv = nullptr;
	currentGraphicsPipelineState.depthStencilFormat = DXGI_FORMAT_UNKNOWN;
	if (depthTex != nullptr)
	{
		assert(depthTex->getDesc().bindFlags & BIND_DEPTH_STENCIL);

		depthTex->transition(D3D12_RESOURCE_STATE_DEPTH_WRITE);

		currentDSV = depthTex->getDsv(depth_slice, depth_mip);
		dsv = &currentDSV;

		currentGraphicsPipelineState.depthStencilFormat = (DXGI_FORMAT)depthTex->getDesc().getDsvFormat();
	}

	frameCmdList->OMSetRenderTargets(num_targets, num_targets > 0 ? currentRTVs.data() : nullptr, false, dsv);
}

void DriverD3D12::setRenderState(ResId res_id)
{
	const RenderState* rs = get_resource(res_id, renderStates);
	currentGraphicsPipelineState.rasterizerState = rs->getRasterizerState();
	currentGraphicsPipelineState.depthStencilState = rs->getDepthStencilState();
}

void DriverD3D12::setShader(ResId res_id, unsigned int variant_index)
{
	const GraphicsShaderSet* shader = get_resource(res_id, graphicsShaders);
	shader->setToPipelineState(currentGraphicsPipelineState, variant_index);
}

void DriverD3D12::setView(float x, float y, float w, float h, float z_min, float z_max)
{
	CD3DX12_VIEWPORT viewport(x, y, w, h, z_min, z_max);
	frameCmdList->RSSetViewports(1, &viewport);
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
	flushGraphicsPipeline();
	frameCmdList->DrawInstanced(vertex_count, 1, start_vertex, 0);
}

void DriverD3D12::drawIndexed(unsigned int index_count, unsigned int start_index, int base_vertex)
{
	flushGraphicsPipeline();
	frameCmdList->DrawIndexedInstanced(index_count, 1, start_index, base_vertex, 0);
}

void DriverD3D12::dispatch(unsigned int num_threadgroups_x, unsigned int num_threadgroups_y, unsigned int num_threadgroups_z)
{
	ASSERT_NOT_IMPLEMENTED;
}

void DriverD3D12::clearRenderTargets(const RenderTargetClearParams clear_params)
{
	if (clear_params.clearFlags & CLEAR_FLAG_COLOR)
	{
		for (uint i = 0, mask = clear_params.colorTargetMask;
			mask && i < currentRTVs.size();
			i++, mask >>= mask)
		{
			if ((mask & 1) && currentRTVs[i].ptr != 0)
				frameCmdList->ClearRenderTargetView(currentRTVs[i], clear_params.color, 0, nullptr);
		}
	}
	if ((clear_params.clearFlags & (CLEAR_FLAG_DEPTH | CLEAR_FLAG_STENCIL)) && currentDSV.ptr != 0)
	{
		D3D12_CLEAR_FLAGS dsvClearFlags = (D3D12_CLEAR_FLAGS)0;
		if (clear_params.clearFlags & CLEAR_FLAG_DEPTH)
			dsvClearFlags |= D3D12_CLEAR_FLAG_DEPTH;
		if (clear_params.clearFlags & CLEAR_FLAG_STENCIL)
			dsvClearFlags |= D3D12_CLEAR_FLAG_STENCIL;
		assert(dsvClearFlags != 0);

		frameCmdList->ClearDepthStencilView(currentDSV, dsvClearFlags, clear_params.depth, clear_params.stencil, 0, nullptr);
	}
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
	// Init frame cmdlist
	{
		frameCmdList = directCommandQueue->GetCommandList();

		// Expose the following stuff to IDriver interface if we want to customize them
		{
			frameCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			CD3DX12_RECT scissorRect(0, 0, LONG_MAX, LONG_MAX);
			frameCmdList->RSSetScissorRects(1, &scissorRect);
		}
	}

	currentRTVs.fill(CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT()));
	currentDSV = CD3DX12_CPU_DESCRIPTOR_HANDLE(CD3DX12_DEFAULT());
}

void DriverD3D12::endFrame()
{
	// Render Dear ImGui graphics
	frameCmdList->SetDescriptorHeaps(1, cbvSrvUavHeap->getHeapAddress());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), frameCmdList.Get());
}

void DriverD3D12::present()
{
	backbuffers[currentBackBufferIndex]->transition(D3D12_RESOURCE_STATE_PRESENT);

	// Submit graphics work
	frameFenceValues[currentBackBufferIndex] = directCommandQueue->ExecuteCommandList(frameCmdList);

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
	RESOURCE_LOCK_GUARD;
	assert(graphicsShaders.find(shader_res_id) != graphicsShaders.end());
	return graphicsShaders[shader_res_id]->getVariantIndexForKeywords(keywords, num_keywords);
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
	int numShaders = 0, numShadersFailedToCompile = 0;
	for (auto& [k, v] : graphicsShaders)
	{
		if (strlen(settings.shaderRecompileFilter) > 0 && strstr(v->getName().c_str(), settings.shaderRecompileFilter) == nullptr)
			continue;
		if (!v->recompile())
			numShadersFailedToCompile++;
		numShaders++;
	}
	if (numShadersFailedToCompile <= 0)
		PLOG_INFO << "Shaders recompiled successfully. Number of shaders: " << numShaders;
	else
		PLOG_ERROR << "There were errors during shader recompilation. Number of shaders: " << numShaders << " Number of shaders failed to compile: " << numShadersFailedToCompile;
}

void DriverD3D12::setErrorShaderDesc(const ShaderSetDesc& desc)
{
	ResId errorShaderResId = createShaderSet(desc);
	errorShader.reset(graphicsShaders[errorShaderResId]);
}

ResId DriverD3D12::getErrorShader()
{
	return errorShader->getId();
}

void DriverD3D12::transitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before_state, D3D12_RESOURCE_STATES after_state, ID3D12GraphicsCommandList2* cmd_list)
{
	// TODO: implement batch barriers
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before_state, after_state);
	cmd_list->ResourceBarrier(1, &barrier);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DriverD3D12::createShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* desc)
{
	DescriptorHandlePair srv = cbvSrvUavHeap->allocateDescriptor();
	device->CreateShaderResourceView(resource, desc, srv.cpuHandle);
	return srv.cpuHandle;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DriverD3D12::createUnorderedAccessView(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* desc)
{
	DescriptorHandlePair uav = cbvSrvUavHeap->allocateDescriptor();
	ID3D12Resource* counterResource = nullptr;
	device->CreateUnorderedAccessView(resource, counterResource, desc, uav.cpuHandle);
	return uav.cpuHandle;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DriverD3D12::createConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc)
{
	DescriptorHandlePair cbv = cbvSrvUavHeap->allocateDescriptor();
	ID3D12Resource* counterResource = nullptr;
	device->CreateConstantBufferView(&desc, cbv.cpuHandle);
	return cbv.cpuHandle;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DriverD3D12::createRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc)
{
	DescriptorHandlePair rtv = rtvHeap->allocateDescriptor();
	device->CreateRenderTargetView(resource, desc, rtv.cpuHandle);
	return rtv.cpuHandle;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DriverD3D12::createDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC* desc)
{
	DescriptorHandlePair dsv = dsvHeap->allocateDescriptor();
	device->CreateDepthStencilView(resource, desc, dsv.cpuHandle);
	return dsv.cpuHandle;
}

template<typename T>
static ResId emplace_resource(T* res, std::unordered_map<ResId, T*>& dest)
{
	RESOURCE_LOCK_GUARD;
	ResId resId = nextAvailableResId++;
	auto&& emplaceResult = dest.emplace(resId, res);
	assert(emplaceResult.second);
	return resId;
}

template<typename T>
static void erase_resource(ResId id, std::unordered_map<ResId, T*>& from)
{
	RESOURCE_LOCK_GUARD;
	size_t numElementsErased = from.erase(id);
	assert(numElementsErased == 1);
}

ResId DriverD3D12::registerTexture(Texture* tex) { return emplace_resource(tex, textures); }
void DriverD3D12::unregisterTexture(ResId id) { erase_resource(id, textures); }
ResId DriverD3D12::registerBuffer(Buffer* buf) { return emplace_resource(buf, buffers); }
void DriverD3D12::unregisterBuffer(ResId id) { erase_resource(id, buffers); }
ResId DriverD3D12::registerRenderState(RenderState* rs) { return emplace_resource(rs, renderStates); }
void DriverD3D12::unregisterRenderState(ResId id) { erase_resource(id, renderStates); }
ResId DriverD3D12::registerShaderSet(GraphicsShaderSet* shader_set) { return emplace_resource(shader_set, graphicsShaders); }
void DriverD3D12::unregisterShaderSet(ResId id) { erase_resource(id, graphicsShaders); }
ResId DriverD3D12::registerInputLayout(InputLayout* input_layout) { return emplace_resource(input_layout, inputLayouts); }
void DriverD3D12::unregisterInputLayout(ResId id) { erase_resource(id, inputLayouts); }

void DriverD3D12::flush()
{
	directCommandQueue->Flush();
	computeCommandQueue->Flush();
	copyCommandQueue->Flush();
}

void DriverD3D12::initCapabilities()
{
	// Root signature version 1.1 should be preferred because it allows for driver level optimizations to be made.
	// Read more at:
	// - https://www.3dgep.com/learning-directx-12-2/#root-signature
	// - https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_descriptor_range_flags

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	highestRootSignatureVersion = featureData.HighestVersion;
}

void DriverD3D12::initDefaultPipelineStates()
{
	currentGraphicsPipelineState.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	currentGraphicsPipelineState.blendState = CD3DX12_BLEND_DESC(CD3DX12_DEFAULT());
	currentGraphicsPipelineState.rasterizerState = CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT());
	currentGraphicsPipelineState.depthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(CD3DX12_DEFAULT());
}

bool DriverD3D12::initResolutionDependentResources()
{
	// Create backbuffer render target views
	for (int i = 0; i < NUM_SWACHAIN_BUFFERS; i++)
	{
		ID3D12Resource* backbufferResource;
		verify(swapchain->GetBuffer(i, IID_PPV_ARGS(&backbufferResource)));

		D3D12_RESOURCE_DESC rd;
		rd = backbufferResource->GetDesc();

		assert(rd.Width == displayWidth);
		assert(rd.Height == displayHeight);

		TextureDesc desc;
		desc.name = "backbuffer";
		desc.width = rd.Width;
		desc.height = rd.Height;
		desc.format = (TexFmt)rd.Format;
		desc.mips = rd.MipLevels;
		desc.usageFlags = ResourceUsage::DEFAULT;
		desc.bindFlags = BIND_RENDER_TARGET;
		desc.cpuAccessFlags = 0;
		desc.miscFlags = 0;

		backbuffers[i] = std::make_unique<Texture>(&desc, backbufferResource);

		set_debug_name(backbufferResource, desc.name);
	}

	return true;
}

void DriverD3D12::closeResolutionDependentResources()
{
	for (int i = 0; i < NUM_SWACHAIN_BUFFERS; i++)
	{
		backbuffers[i].reset();
		frameFenceValues[i] = frameFenceValues[currentBackBufferIndex];
	}
}

template<typename T>
static void release_pool(std::unordered_map<ResId, T*>& pool)
{
	for (auto itr = pool.begin(); itr != pool.end();)
		delete (itr++)->second;
	pool.clear();
}

void DriverD3D12::releaseAllResources()
{
	assert(graphicsShaders.size() == 0);
	release_pool(graphicsShaders);
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

void DriverD3D12::flushGraphicsPipeline()
{
	// demo cube stuff
	{
		D3D12_RT_FORMAT_ARRAY rtvFormats = {};
		rtvFormats.NumRenderTargets = 1;
		rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

		currentGraphicsPipelineState.rootSignature = m_RootSignature.Get();

		frameCmdList->SetGraphicsRootSignature(m_RootSignature.Get());

		// Update the MVP matrix
		XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
		mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
		frameCmdList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);
	}

	frameCmdList->SetPipelineState(getOrCreateCurrentGraphicsPipeline());
}

bool DriverD3D12::loadDemoContent()
{
	// Create the root signature.
	{
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
		ComPtr<ID3DBlob> rootSignatureBlob, errorBlob;
		verify(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
			highestRootSignatureVersion, &rootSignatureBlob, &errorBlob));
		// Create the root signature.
		verify(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
			rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
	}

	return true;
}

void DriverD3D12::destroyDemoContent()
{
	m_RootSignature.Reset();
}

} // namespace drv_d3d12