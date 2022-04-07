#include "DriverD3D12.h"

#include <Common.h>

#include <assert.h>
#include <chrono>
#include <3rdParty/cxxopts/cxxopts.hpp>

using namespace drv_d3d12;

void create_driver_d3d12()
{
	drv = new DriverD3D12();
}

DriverD3D12& drv_d3d12::DriverD3D12::get()
{
	return *(DriverD3D12*)drv;
}

static ComPtr<IDXGIAdapter4> get_adapter(bool debug_layer_enabled)
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	uint createFactoryFlags = 0;
	if (debug_layer_enabled)
		createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

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
			ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
		}
	}

	return dxgiAdapter4;
}

static ComPtr<ID3D12Device2> create_device(ComPtr<IDXGIAdapter4> adapter, bool debug_layer_enabled)
{
	ComPtr<ID3D12Device2> d3d12Device2;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

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

static ComPtr<ID3D12CommandQueue> create_command_queue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

	return d3d12CommandQueue;
}

static ComPtr<IDXGISwapChain4> create_swap_chain(HWND hwnd, ComPtr<ID3D12CommandQueue> command_queue, uint width, uint height, uint buffer_count, bool debug_layer_enabled)
{
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
	if (debug_layer_enabled)
		createFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = buffer_count;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
		command_queue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

	return dxgiSwapChain4;
}

static ComPtr<ID3D12DescriptorHeap> create_descriptor_heap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t num_descriptors)
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = num_descriptors;
	desc.Type = type;

	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

static ComPtr<ID3D12CommandAllocator> create_command_allocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

static ComPtr<ID3D12GraphicsCommandList> create_command_list(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> command_allocator, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ThrowIfFailed(device->CreateCommandList(0, type, command_allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	ThrowIfFailed(commandList->Close());

	return commandList;
}

static ComPtr<ID3D12Fence> create_fence(ComPtr<ID3D12Device2> device)
{
	ComPtr<ID3D12Fence> fence;

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	return fence;
}

static uint64 signal(ComPtr<ID3D12CommandQueue> command_queue, ComPtr<ID3D12Fence> fence, uint64& fence_value)
{
	uint64_t fenceValueForSignal = ++fence_value;
	ThrowIfFailed(command_queue->Signal(fence.Get(), fenceValueForSignal));

	return fenceValueForSignal;
}

static void wait_for_fence_value(ComPtr<ID3D12Fence> fence, uint64_t fence_value, HANDLE fence_event, std::chrono::milliseconds duration = std::chrono::milliseconds::max())
{
	if (fence->GetCompletedValue() < fence_value)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fence_value, fence_event));
		::WaitForSingleObject(fence_event, static_cast<DWORD>(duration.count()));
	}
}

static void flush(ComPtr<ID3D12CommandQueue> command_queue, ComPtr<ID3D12Fence> fence, uint64_t& fence_value, HANDLE fence_event)
{
	uint64_t fenceValueForSignal = signal(command_queue, fence, fence_value);
	wait_for_fence_value(fence, fenceValueForSignal, fence_event);
}

static HANDLE create_event_handle()
{
	HANDLE fenceEvent;

	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent && "Failed to create fence event.");

	return fenceEvent;
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
	commandQueue = create_command_queue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	swapchain = create_swap_chain(hWnd, commandQueue, displayWidth, displayHeight, NUM_SWACHAIN_BUFFERS, debug_layer_enabled);
	currentBackBufferIndex = swapchain->GetCurrentBackBufferIndex();
	rtvDescriptorHeap = create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUM_SWACHAIN_BUFFERS);
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	initResolutionDependentResources();

	for (int i = 0; i < NUM_SWACHAIN_BUFFERS; ++i)
		commandAllocators[i] = create_command_allocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	commandList = create_command_list(device, commandAllocators[currentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);

	fence = create_fence(device);
	fenceEvent = create_event_handle();

	return true;
}

void DriverD3D12::shutdown()
{
	// Make sure the command queue has finished all commands before closing.
	flush(commandQueue, fence, fenceValue, fenceEvent);

	::CloseHandle(fenceEvent);

	// TODO: close stuff

	if (dxgiDebug != nullptr)
		dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
}

void DriverD3D12::resize(int display_width, int display_height)
{
	if (display_width == displayWidth && display_height == displayHeight)
		return;

	// Flush the GPU queue to make sure the swap chain's back buffers
	// are not being referenced by an in-flight command list.
	flush(commandQueue, fence, fenceValue, fenceEvent);

	closeResolutionDependentResources();

	displayWidth = display_width;
	displayHeight = display_height;

	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	ThrowIfFailed(swapchain->GetDesc(&swapChainDesc));
	ThrowIfFailed(swapchain->ResizeBuffers(NUM_SWACHAIN_BUFFERS, displayWidth, displayHeight,
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
	setView(ViewportParams{ x, y, w, h, z_min, z_max });
}

void DriverD3D12::setView(const ViewportParams& vp)
{
	ASSERT_NOT_IMPLEMENTED;
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
	// TODO: ImGui
}

void DriverD3D12::endFrame()
{
	// TODO: ImGui
}

void DriverD3D12::present()
{
	auto commandAllocator = commandAllocators[currentBackBufferIndex];
	auto backBuffer = backbuffers[currentBackBufferIndex];

	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);

	// Clear the render target.
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		commandList->ResourceBarrier(1, &barrier);

		float clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
			currentBackBufferIndex, rtvDescriptorSize);

		commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	}

	// Present
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &barrier);

		ThrowIfFailed(commandList->Close());

		ID3D12CommandList* const commandLists[] = {
			commandList.Get()
		};
		commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		uint syncInterval = settings.vsync ? 1 : 0;
		uint presentFlags = settings.vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING;
		ThrowIfFailed(swapchain->Present(syncInterval, presentFlags));

		frameFenceValues[currentBackBufferIndex] = signal(commandQueue, fence, fenceValue);

		currentBackBufferIndex = swapchain->GetCurrentBackBufferIndex();

		wait_for_fence_value(fence, frameFenceValues[currentBackBufferIndex], fenceEvent);
	}
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

bool DriverD3D12::initResolutionDependentResources()
{
	updateBackbufferRTVs();

	return true;
}

void DriverD3D12::closeResolutionDependentResources()
{
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
		ThrowIfFailed(swapchain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));

		device->CreateRenderTargetView(backbuffer.Get(), nullptr, rtvHandle);

		backbuffers[i] = backbuffer;

		rtvHandle.Offset(rtvDescriptorSize);
	}
}
