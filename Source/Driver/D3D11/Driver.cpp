#include "Driver.h"

#include <Common.h>

#include <assert.h>
#include <3rdParty/imgui/imgui.h>
#include <3rdParty/imgui/imgui_impl_dx11.h>

#include "Texture.h"
#include "Buffer.h"
#include "Sampler.h"
#include "RenderState.h"
#include "ShaderSet.h"
#include "InputLayout.h"

using namespace drv_d3d11;

static ResId nextAvailableResId = 0;

void create_driver_d3d11()
{
	drv = new Driver();
}

Driver& drv_d3d11::Driver::get()
{
	return *(Driver*)drv;
}

bool Driver::init(void* hwnd, int display_width, int display_height)
{
	HRESULT hr;

	DXGI_SWAP_CHAIN_DESC scd{};
	scd.BufferCount = 2;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Not _SRGB because then ImGui looks weird :( SRGB conversion is done in postfx
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = (HWND)hwnd;
	scd.SampleDesc.Count = 1;
	scd.Windowed = true;
	scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	unsigned int creationFlags = 0;
#if defined(_DEBUG)
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags, nullptr, 0, D3D11_SDK_VERSION,
		&scd, &swapchain, &device, nullptr, &context);
	assert(SUCCEEDED(hr));

	if (creationFlags & D3D11_CREATE_DEVICE_DEBUG)
	{
		// Device debug
		hr = device->QueryInterface(IID_PPV_ARGS(&deviceDebug));
		assert(SUCCEEDED(hr));

		// DXGI debug
		HMODULE dxgidebug = LoadLibraryEx("dxgidebug.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (dxgidebug)
		{
			typedef HRESULT(WINAPI* LPDXGIGETDEBUGINTERFACE)(REFIID, void**);
			auto dxgiGetDebugInterface = reinterpret_cast<LPDXGIGETDEBUGINTERFACE>(
				reinterpret_cast<void*>(GetProcAddress(dxgidebug, "DXGIGetDebugInterface")));
			if (FAILED(dxgiGetDebugInterface(IID_PPV_ARGS(&dxgiDebug))))
			{
				PLOG_ERROR << "Failed to get DXGI debug interface.";
				dxgiDebug = nullptr;
			}
		}
		else
			PLOG_ERROR << "Failed to load dxgidebug.dll";
	}

	initResolutionDependentResources(display_width, display_height);

	// Create default render state
	RenderStateDesc desc;
	ResId defaultRenderStateResId = createRenderState(desc);
	defaultRenderState.reset(renderStates[defaultRenderStateResId]);

	CONTEXT_LOCK_GUARD
#if PROFILE_MARKERS_ENABLED
	hr = context->QueryInterface(IID_PPV_ARGS(&perf));
	assert(SUCCEEDED(hr));
#endif

	// TODO: Maybe expose this?
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ImGui_ImplDX11_Init(device, context);

	return true;
}

void Driver::shutdown()
{
	context->ClearState();
	context->Flush();

	ImGui_ImplDX11_Shutdown();

	errorShader.reset();
	defaultRenderState.reset();

	closeResolutionDependentResources();
	releaseAllResources();

	SAFE_RELEASE(perf);
	SAFE_RELEASE(swapchain);
	SAFE_RELEASE(context);
	SAFE_RELEASE(device);
	SAFE_RELEASE(deviceDebug);

	if (dxgiDebug != nullptr)
	{
		dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
		SAFE_RELEASE(dxgiDebug);
	}
}

void Driver::resize(int display_width, int display_height)
{
	if (display_width == displayWidth && display_height == displayHeight)
		return;
	{
		CONTEXT_LOCK_GUARD
		context->OMSetRenderTargets(0, nullptr, nullptr);
	}
	closeResolutionDependentResources();

	DXGI_SWAP_CHAIN_DESC scd{};
	swapchain->GetDesc(&scd);
	HRESULT hr = swapchain->ResizeBuffers(0, display_width, display_height, DXGI_FORMAT_UNKNOWN, scd.Flags);
	assert(SUCCEEDED(hr));

	initResolutionDependentResources(display_width, display_height);
}

void Driver::getDisplaySize(int& display_width, int& display_height)
{
	display_width = displayWidth;
	display_height = displayHeight;
}

ITexture* Driver::createTexture(const TextureDesc& desc)
{
	return new Texture(&desc);
}

ITexture* Driver::createTextureStub()
{
	return new Texture();
}

IBuffer* Driver::createBuffer(const BufferDesc& desc)
{
	return new Buffer(desc);
}

ResId Driver::createSampler(const SamplerDesc& desc)
{
	Sampler* s = new Sampler(desc);
	return s->getId();
}

ResId Driver::createRenderState(const RenderStateDesc& desc)
{
	RenderState* rs = new RenderState(desc);
	return rs->getId();
}

ResId Driver::createShaderSet(const ShaderSetDesc& desc)
{
	ShaderSet* shaderSet = new ShaderSet(desc);
	return shaderSet->getId();
}

ResId Driver::createComputeShader(const ComputeShaderDesc& desc)
{
	ShaderSet* shaderSet = new ShaderSet(desc);
	return shaderSet->getId();
}

ResId Driver::createInputLayout(const InputLayoutElementDesc* descs, unsigned int num_descs, ResId shader_set)
{
	assert(descs != nullptr);
	assert(num_descs > 0);
	assert(shaders.find(shader_set) != shaders.end());
	ShaderSet* shaderSet = shaders[shader_set];
	InputLayout* inputLayout = new InputLayout(descs, num_descs, *shaderSet);
	return inputLayout->getId();
}

template<typename T>
bool erase_if_found(ResId id, std::map<ResId, T*>& from)
{
	T* res;
	{
		RESOURCE_LOCK_GUARD
		auto it = from.find(id);
		if (it == from.end())
			return false;
		res = it->second;
	}
	delete res;
	// resource will remove itself from the pool from dtor
	return true;
}

void Driver::destroyResource(ResId res_id)
{
	if (erase_if_found(res_id, inputLayouts))
		return;
	if (erase_if_found(res_id, renderStates))
		return;
	if (erase_if_found(res_id, shaders))
		return;
	if (erase_if_found(res_id, buffers))
		return;
	if (erase_if_found(res_id, textures))
		return;
	if (erase_if_found(res_id, samplers))
		return;
	assert(false);
}

void Driver::setInputLayout(ResId res_id)
{
	RESOURCE_LOCK_GUARD
	assert(res_id != BAD_RESID);
	assert(inputLayouts.find(res_id) != inputLayouts.end());
	CONTEXT_LOCK_GUARD
	context->IASetInputLayout(inputLayouts[res_id]->getResource());
}

void Driver::setIndexBuffer(ResId res_id)
{
	RESOURCE_LOCK_GUARD
	assert(res_id != BAD_RESID);
	assert(buffers.find(res_id) != buffers.end());
	assert(buffers[res_id]->getDesc().bindFlags & BIND_INDEX_BUFFER);

	CONTEXT_LOCK_GUARD
	context->IASetIndexBuffer(buffers[res_id]->getResource(), (DXGI_FORMAT)getIndexFormat(), 0);
}

void Driver::setVertexBuffer(ResId res_id)
{
	RESOURCE_LOCK_GUARD
	assert(res_id != BAD_RESID);
	assert(buffers.find(res_id) != buffers.end());
	assert(buffers[res_id]->getDesc().bindFlags & BIND_VERTEX_BUFFER);

	ID3D11Buffer* res = buffers[res_id]->getResource();
	unsigned int stride = buffers[res_id]->getDesc().elementByteSize;
	unsigned int offset = 0;
	CONTEXT_LOCK_GUARD
	context->IASetVertexBuffers(0, 1, &res, &stride, &offset);
}

void Driver::setConstantBuffer(ShaderStage stage, unsigned int slot, ResId res_id)
{
	RESOURCE_LOCK_GUARD
	assert(res_id == BAD_RESID || buffers.find(res_id) != buffers.end());
	assert(res_id == BAD_RESID || buffers[res_id]->getDesc().bindFlags & BIND_CONSTANT_BUFFER);

	ID3D11Buffer* res = res_id != BAD_RESID ? buffers[res_id]->getResource() : nullptr;
	CONTEXT_LOCK_GUARD
	switch (stage)
	{
	case ShaderStage::VS:
		context->VSSetConstantBuffers(slot, 1, &res);
		break;
	case ShaderStage::PS:
		context->PSSetConstantBuffers(slot, 1, &res);
		break;
	case ShaderStage::GS:
		context->GSSetConstantBuffers(slot, 1, &res);
		break;
	case ShaderStage::HS:
		context->HSSetConstantBuffers(slot, 1, &res);
		break;
	case ShaderStage::DS:
		context->DSSetConstantBuffers(slot, 1, &res);
		break;
	case ShaderStage::CS:
		context->CSSetConstantBuffers(slot, 1, &res);
		break;
	default:
		assert(false);
		break;
	}
}

void Driver::setBuffer(ShaderStage stage, unsigned int slot, ResId res_id)
{
	RESOURCE_LOCK_GUARD
	assert(res_id == BAD_RESID || buffers.find(res_id) != buffers.end());
	assert(res_id == BAD_RESID || buffers[res_id]->getDesc().bindFlags & BIND_SHADER_RESOURCE);

	ID3D11ShaderResourceView* srv = res_id != BAD_RESID ? buffers[res_id]->getSrv() : nullptr;
	CONTEXT_LOCK_GUARD
	switch (stage)
	{
	case ShaderStage::VS:
		context->VSSetShaderResources(slot, 1, &srv);
		break;
	case ShaderStage::PS:
		context->PSSetShaderResources(slot, 1, &srv);
		break;
	case ShaderStage::GS:
		context->GSSetShaderResources(slot, 1, &srv);
		break;
	case ShaderStage::HS:
		context->HSSetShaderResources(slot, 1, &srv);
		break;
	case ShaderStage::DS:
		context->DSSetShaderResources(slot, 1, &srv);
		break;
	case ShaderStage::CS:
		context->CSSetShaderResources(slot, 1, &srv);
		break;
	default:
		assert(false);
		break;
	}
}

void Driver::setRwBuffer(unsigned int slot, ResId res_id)
{
	RESOURCE_LOCK_GUARD
	assert(res_id == BAD_RESID || buffers.find(res_id) != buffers.end());
	assert(res_id == BAD_RESID || buffers[res_id]->getDesc().bindFlags & BIND_UNORDERED_ACCESS);

	ID3D11UnorderedAccessView* uav = res_id != BAD_RESID ? buffers[res_id]->getUav() : nullptr;
	CONTEXT_LOCK_GUARD
	context->CSSetUnorderedAccessViews(slot, 1, &uav, nullptr);
}

void Driver::setTexture(ShaderStage stage, unsigned int slot, ResId res_id)
{
	RESOURCE_LOCK_GUARD
	assert(res_id == BAD_RESID || textures.find(res_id) != textures.end());
	assert(res_id == BAD_RESID || textures[res_id]->getDesc().bindFlags & BIND_SHADER_RESOURCE);

	ID3D11ShaderResourceView* srv = res_id != BAD_RESID ? textures[res_id]->getSrv() : nullptr;
	CONTEXT_LOCK_GUARD
	switch (stage)
	{
	case ShaderStage::VS:
		context->VSSetShaderResources(slot, 1, &srv);
		break;
	case ShaderStage::PS:
		context->PSSetShaderResources(slot, 1, &srv);
		break;
	case ShaderStage::GS:
		context->GSSetShaderResources(slot, 1, &srv);
		break;
	case ShaderStage::HS:
		context->HSSetShaderResources(slot, 1, &srv);
		break;
	case ShaderStage::DS:
		context->DSSetShaderResources(slot, 1, &srv);
		break;
	case ShaderStage::CS:
		context->CSSetShaderResources(slot, 1, &srv);
		break;
	default:
		assert(false);
		break;
	}
}

void Driver::setRwTexture(unsigned int slot, ResId res_id)
{
	RESOURCE_LOCK_GUARD
	assert(res_id == BAD_RESID || textures.find(res_id) != textures.end());
	assert(res_id == BAD_RESID || textures[res_id]->getDesc().bindFlags & BIND_UNORDERED_ACCESS);

	ID3D11UnorderedAccessView* uav = res_id != BAD_RESID ? textures[res_id]->getUav() : nullptr;
	CONTEXT_LOCK_GUARD
	context->CSSetUnorderedAccessViews(slot, 1, &uav, nullptr);
}

void Driver::setSampler(ShaderStage stage, unsigned int slot, ResId res_id)
{
	RESOURCE_LOCK_GUARD
	assert(res_id == BAD_RESID || samplers.find(res_id) != samplers.end());

	ID3D11SamplerState* sampler = res_id != BAD_RESID ? samplers[res_id]->getResource() : nullptr;

	CONTEXT_LOCK_GUARD
	switch (stage)
	{
	case ShaderStage::VS:
		context->VSSetSamplers(slot, 1, &sampler);
		break;
	case ShaderStage::PS:
		context->PSSetSamplers(slot, 1, &sampler);
		break;
	case ShaderStage::GS:
		context->GSSetSamplers(slot, 1, &sampler);
		break;
	case ShaderStage::HS:
		context->HSSetSamplers(slot, 1, &sampler);
		break;
	case ShaderStage::DS:
		context->DSSetSamplers(slot, 1, &sampler);
		break;
	case ShaderStage::CS:
		context->CSSetSamplers(slot, 1, &sampler);
		break;
	default:
		assert(false);
		break;
	}
}

void Driver::setRenderTarget(ResId target_id, ResId depth_id,
	unsigned int target_slice, unsigned int depth_slice, unsigned int target_mip, unsigned int depth_mip)
{
	RESOURCE_LOCK_GUARD
	assert(target_id == BAD_RESID || textures.find(target_id) != textures.end());
	assert(target_id == BAD_RESID || textures[target_id]->getDesc().bindFlags & BIND_RENDER_TARGET);
	assert(depth_id == BAD_RESID || textures.find(depth_id) != textures.end());
	assert(depth_id == BAD_RESID || textures[depth_id]->getDesc().bindFlags & BIND_DEPTH_STENCIL);

	currentRTVs.fill(nullptr);
	Texture* targetTex = target_id != BAD_RESID ? textures[target_id] : nullptr;
	currentRTVs[0] = targetTex != nullptr ? targetTex->getRtv(target_slice, target_mip) : nullptr;

	Texture *depthTex = depth_id != BAD_RESID ? textures[depth_id] : nullptr;
	currentDSV = depthTex != nullptr ? depthTex->getDsv(depth_slice, depth_mip) : nullptr;
	
	CONTEXT_LOCK_GUARD
	context->OMSetRenderTargets(1, currentRTVs.data(), currentDSV);
}

void Driver::setRenderTargets(unsigned int num_targets, ResId* target_ids, ResId depth_id,
	unsigned int* target_slices, unsigned int depth_slice, unsigned int* target_mips, unsigned int depth_mip)
{
	RESOURCE_LOCK_GUARD
	assert(num_targets <= D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
	assert(depth_id == BAD_RESID || textures.find(depth_id) != textures.end());
	assert(depth_id == BAD_RESID || textures[depth_id]->getDesc().bindFlags & BIND_DEPTH_STENCIL);

	Texture* depthTex = depth_id != BAD_RESID ? textures[depth_id] : nullptr;
	currentDSV = depthTex != nullptr ? depthTex->getDsv(depth_slice, depth_mip) : nullptr;

	currentRTVs.fill(nullptr);
	for (unsigned int i = 0; i < num_targets; i++)
	{
		assert(target_ids[i] == BAD_RESID || textures.find(target_ids[i]) != textures.end());
		assert(target_ids[i] == BAD_RESID || textures[target_ids[i]]->getDesc().bindFlags & BIND_RENDER_TARGET);

		Texture* targetTex = target_ids[i] != BAD_RESID ? textures[target_ids[i]] : nullptr;
		unsigned int targetSlice = target_slices != nullptr ? target_slices[i] : 0;
		unsigned int targetMip = target_mips != nullptr ? target_mips[i] : 0;
		currentRTVs[i] = targetTex != nullptr ? targetTex->getRtv(targetSlice, targetMip) : nullptr;
	}

	CONTEXT_LOCK_GUARD
	context->OMSetRenderTargets(num_targets, currentRTVs.data(), currentDSV);
}

void Driver::setRenderState(ResId res_id)
{
	RESOURCE_LOCK_GUARD
	assert(res_id == BAD_RESID || renderStates.find(res_id) != renderStates.end());
	
	RenderState *rs = res_id != BAD_RESID ? renderStates[res_id] : defaultRenderState.get();
	CONTEXT_LOCK_GUARD
	context->RSSetState(rs->getRasterizerState());
	context->OMSetDepthStencilState(rs->getDepthStencilState(), 0);
}

void Driver::setShader(ResId res_id, unsigned int variant_index)
{
	RESOURCE_LOCK_GUARD
	assert(shaders.find(res_id) != shaders.end());
	shaders[res_id]->setToContext(variant_index);
}

void Driver::setView(float x, float y, float w, float h, float z_min, float z_max)
{
	setView(ViewportParams{ x, y, w, h, z_min, z_max });
}

void Driver::setView(const ViewportParams& vp)
{
	D3D11_VIEWPORT viewport{ vp.x, vp.y, vp.w, vp.h, vp.z_min, vp.z_max };
	CONTEXT_LOCK_GUARD
	currentViewport = vp;
	context->RSSetViewports(1, &viewport);
}

void Driver::getView(ViewportParams& vp)
{
	vp = currentViewport;
}

void Driver::draw(unsigned int vertex_count, unsigned int start_vertex)
{
	CONTEXT_LOCK_GUARD
	context->Draw(vertex_count, start_vertex);
}

void Driver::drawIndexed(unsigned int index_count, unsigned int start_index, int base_vertex)
{
	CONTEXT_LOCK_GUARD
	context->DrawIndexed(index_count, start_index, base_vertex);
}

void Driver::dispatch(unsigned int num_threadgroups_x, unsigned int num_threadgroups_y, unsigned int num_threadgroups_z)
{
	CONTEXT_LOCK_GUARD
	context->Dispatch(num_threadgroups_x, num_threadgroups_y, num_threadgroups_z);
}

void Driver::clearRenderTargets(const RenderTargetClearParams clear_params)
{
	if (clear_params.clearFlags & CLEAR_FLAG_COLOR)
	{
		CONTEXT_LOCK_GUARD
		for (unsigned int i = 0, mask = clear_params.colorTargetMask;
			mask && i < currentRTVs.size();
			i++, mask >>= mask)
		{
			if ((mask & 1) && currentRTVs[i] != nullptr)
				context->ClearRenderTargetView(currentRTVs[i], clear_params.color);
		}
	}
	if ((clear_params.clearFlags & (CLEAR_FLAG_DEPTH | CLEAR_FLAG_STENCIL)) && currentDSV != nullptr)
	{
		CONTEXT_LOCK_GUARD
		unsigned int dsvClearFlags = 0;
		if (clear_params.clearFlags & CLEAR_FLAG_DEPTH)
			dsvClearFlags |= D3D11_CLEAR_DEPTH;
		if (clear_params.clearFlags & CLEAR_FLAG_STENCIL)
			dsvClearFlags |= D3D11_CLEAR_STENCIL;
		context->ClearDepthStencilView(currentDSV, dsvClearFlags, clear_params.depth, clear_params.stencil);
	}
}

void Driver::beginFrame()
{
	ImGui_ImplDX11_NewFrame();
}

void Driver::endFrame()
{
	ImDrawData* drawData = ImGui::GetDrawData();
	if (drawData != nullptr)
	{
		PROFILE_SCOPE("ImGui");
		CONTEXT_LOCK_GUARD
		ImGui_ImplDX11_RenderDrawData(drawData);
	}
}

void Driver::present()
{
	swapchain->Present(settings.vsync ? 1 : 0, settings.vsync ? 0 : DXGI_PRESENT_ALLOW_TEARING);
}

void Driver::beginEvent(const char* label)
{
#if PROFILE_MARKERS_ENABLED
	wchar_t wlabel[128];
	utf8_to_wcs(label, wlabel, 128);
	CONTEXT_LOCK_GUARD
	perf->BeginEvent(wlabel);
#endif
}

void Driver::endEvent()
{
#if PROFILE_MARKERS_ENABLED
	CONTEXT_LOCK_GUARD
	perf->EndEvent();
#endif
}

unsigned int Driver::getShaderVariantIndexForKeywords(ResId shader_res_id, const char** keywords, unsigned int num_keywords)
{
	RESOURCE_LOCK_GUARD
	assert(shaders.find(shader_res_id) != shaders.end());
	return shaders[shader_res_id]->getVariantIndexForKeywords(keywords, num_keywords);
}

void Driver::setSettings(const DriverSettings& new_settings)
{
	DriverSettings oldSettings = settings;
	settings = new_settings;

	if (settings.textureFilteringAnisotropy != oldSettings.textureFilteringAnisotropy)
	{
		for (auto& [k, v] : samplers)
			v->recreate();
	}
}

void Driver::recompileShaders()
{
	bool allCompiledSuccessfully = true;
	for (auto& [k, v] : shaders)
		if (!v->recompile())
			allCompiledSuccessfully = false;
	if (allCompiledSuccessfully)
		PLOG_INFO << "Shaders recompiled successfully";
	else
		PLOG_ERROR << "There were errors during shader recompilation.";
}

void Driver::setErrorShaderDesc(const ShaderSetDesc& desc)
{
	ResId errorShaderResId = createShaderSet(desc);
	errorShader.reset(shaders[errorShaderResId]);
}

ResId Driver::getErrorShader()
{
	return errorShader->getId();
}

template<typename T>
static ResId emplace_resource(T* res, std::map<ResId, T*>& dest)
{
	ResId resId = nextAvailableResId++;
	auto&& emplaceResult = dest.emplace(resId, res);
	assert(emplaceResult.second);
	return resId;
}

template<typename T>
static void erase_resource(ResId id, std::map<ResId, T*>& from)
{
	size_t numElementsErased = from.erase(id);
	assert(numElementsErased > 0);
}

ResId Driver::registerTexture(Texture* tex)
{
	RESOURCE_LOCK_GUARD
	return emplace_resource(tex, textures);
}

void Driver::unregisterTexture(ResId id)
{
	RESOURCE_LOCK_GUARD
	erase_resource(id, textures);
}

ResId Driver::registerBuffer(Buffer* buf)
{
	RESOURCE_LOCK_GUARD
	return emplace_resource(buf, buffers);
}

void Driver::unregisterBuffer(ResId id)
{
	RESOURCE_LOCK_GUARD
	erase_resource(id, buffers);
}

ResId Driver::registerSampler(Sampler* smp)
{
	RESOURCE_LOCK_GUARD
	return emplace_resource(smp, samplers);
}

void Driver::unregisterSampler(ResId id)
{
	RESOURCE_LOCK_GUARD
	erase_resource(id, samplers);
}

ResId Driver::registerRenderState(RenderState* rs)
{
	RESOURCE_LOCK_GUARD
	return emplace_resource(rs, renderStates);
}

void Driver::unregisterRenderState(ResId id)
{
	RESOURCE_LOCK_GUARD
	erase_resource(id, renderStates);
}

ResId Driver::registerShaderSet(ShaderSet* shader_set)
{
	RESOURCE_LOCK_GUARD
	return emplace_resource(shader_set, shaders);
}

void Driver::unregisterShaderSet(ResId id)
{
	RESOURCE_LOCK_GUARD
	erase_resource(id, shaders);
}

ResId Driver::registerInputLayout(InputLayout* input_layout)
{
	RESOURCE_LOCK_GUARD
	return emplace_resource(input_layout, inputLayouts);
}

void Driver::unregisterInputLayout(ResId id)
{
	RESOURCE_LOCK_GUARD
	erase_resource(id, inputLayouts);
}

bool Driver::initResolutionDependentResources(int display_width, int display_height)
{
	HRESULT hr;

	displayWidth = display_width;
	displayHeight = display_height;

	// Create backbuffer render target view
	{
		ID3D11Texture2D* backBufferTexture;
		hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferTexture);
		assert(SUCCEEDED(hr));

		D3D11_TEXTURE2D_DESC td;
		backBufferTexture->GetDesc(&td);

		assert(td.Width == displayWidth);
		assert(td.Height == displayHeight);

		TextureDesc desc;
		desc.name = "backbuffer";
		desc.width = td.Width;
		desc.height = td.Height;
		desc.format = (TexFmt)td.Format;
		desc.mips = td.MipLevels;
		desc.usageFlags = (ResourceUsage)td.Usage;
		desc.bindFlags = td.BindFlags;
		desc.cpuAccessFlags = td.CPUAccessFlags;
		desc.miscFlags = td.MiscFlags;
		backbuffer = std::make_unique<Texture>(&desc, backBufferTexture);

		set_debug_name(backBufferTexture, desc.name);
	}

	return true;
}

void Driver::closeResolutionDependentResources()
{
	backbuffer.reset();
}

template<typename T>
static void release_pool(std::map<ResId, T*>& pool)
{
	for (auto itr = pool.begin(); itr != pool.end();)
		delete (itr++)->second;
	pool.clear();
}

void Driver::releaseAllResources()
{
	assert(textures.size() == 0);
	release_pool(textures);
	assert(buffers.size() == 0);
	release_pool(buffers);
	assert(samplers.size() == 0);
	release_pool(samplers);
	assert(renderStates.size() == 0);
	release_pool(renderStates);
	assert(shaders.size() == 0);
	release_pool(shaders);
	assert(inputLayouts.size() == 0);
	release_pool(inputLayouts);
}
