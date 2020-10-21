#include "Driver.h"

#include <Common.h>

#include <assert.h>

#include "Texture.h"
#include "Buffer.h"
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
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = (HWND)hwnd;
	scd.SampleDesc.Count = 1;
	scd.Windowed = true;
	scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scd.Flags = 0;

	unsigned int creationFlags = 0;
#if defined(_DEBUG)
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags, nullptr, 0, D3D11_SDK_VERSION,
		&scd, &swapchain, &device, nullptr, &context);
	assert(SUCCEEDED(hr));

	initResolutionDependentResources(display_width, display_height);

	return true;
}

void Driver::shutdown()
{
	releaseAllResources();

	swapchain.Reset();
	context.Reset();
	device.Reset();
}

void Driver::resize(int display_width, int display_height)
{
	closeResolutionDependentResources();
	initResolutionDependentResources(display_width, display_height);
}

void drv_d3d11::Driver::getDisplaySize(int& display_width, int& display_height)
{
	display_width = displayWidth;
	display_height = displayHeight;
}

ITexture* Driver::createTexture(const TextureDesc& desc)
{
	return new Texture(desc);
}

IBuffer* Driver::createBuffer(const BufferDesc& desc)
{
	return new Buffer(desc);
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

ResId Driver::createInputLayout(const InputLayoutElementDesc* descs, unsigned int num_descs, ResId shader_set)
{
	assert(descs != nullptr);
	assert(num_descs > 0);
	assert(shaders.find(shader_set) != shaders.end());
	ShaderSet* shaderSet = shaders[shader_set];
	InputLayout* inputLayout = new InputLayout(descs, num_descs, *shaderSet);
	return inputLayout->getId();
}

void Driver::setIndexBuffer(ResId res_id)
{
	assert(res_id != BAD_RESID);
	assert(buffers.find(res_id) != buffers.end());
	assert(buffers[res_id]->getDesc().bindFlags & BIND_INDEX_BUFFER);

	context->IASetIndexBuffer(buffers[res_id]->getResource(), (DXGI_FORMAT)getIndexFormat(), 0);
}

void Driver::setVertexBuffer(ResId res_id)
{
	assert(res_id != BAD_RESID);
	assert(buffers.find(res_id) != buffers.end());
	assert(buffers[res_id]->getDesc().bindFlags & BIND_VERTEX_BUFFER);

	ID3D11Buffer* res = buffers[res_id]->getResource();
	unsigned int stride = buffers[res_id]->getDesc().elementByteSize;
	context->IASetVertexBuffers(0, 1, &res, &stride, nullptr);
}

void Driver::setConstantBuffer(ShaderStage stage, unsigned int slot, ResId res_id)
{
	assert(res_id == BAD_RESID || buffers.find(res_id) != buffers.end());
	assert(res_id == BAD_RESID || buffers[res_id]->getDesc().bindFlags & BIND_CONSTANT_BUFFER);

	ID3D11Buffer* res = res_id != BAD_RESID ? buffers[res_id]->getResource() : nullptr;
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
	assert(res_id == BAD_RESID || buffers.find(res_id) != buffers.end());
	assert(res_id == BAD_RESID || buffers[res_id]->getDesc().bindFlags & BIND_SHADER_RESOURCE);

	ID3D11ShaderResourceView* srv = res_id != BAD_RESID ? buffers[res_id]->getSrv() : nullptr;
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
	assert(res_id == BAD_RESID || buffers.find(res_id) != buffers.end());
	assert(res_id == BAD_RESID || buffers[res_id]->getDesc().bindFlags & BIND_UNORDERED_ACCESS);

	ID3D11UnorderedAccessView* uav = res_id != BAD_RESID ? buffers[res_id]->getUav() : nullptr;
	context->CSSetUnorderedAccessViews(slot, 1, &uav, nullptr);
}

void Driver::setTexture(ShaderStage stage, unsigned int slot, ResId res_id)
{
	assert(res_id == BAD_RESID || textures.find(res_id) != textures.end());
	assert(res_id == BAD_RESID || textures[res_id]->getDesc().bindFlags & BIND_SHADER_RESOURCE);

	ID3D11ShaderResourceView* srv = res_id != BAD_RESID ? textures[res_id]->getSrv() : nullptr;
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
	assert(res_id == BAD_RESID || textures.find(res_id) != textures.end());
	assert(res_id == BAD_RESID || textures[res_id]->getDesc().bindFlags & BIND_UNORDERED_ACCESS);

	ID3D11UnorderedAccessView* uav = res_id != BAD_RESID ? textures[res_id]->getUav() : nullptr;
	context->CSSetUnorderedAccessViews(slot, 1, &uav, nullptr);
}

void Driver::setRenderTarget(ResId target_id, ResId depth_id)
{
	assert(target_id == BAD_RESID || textures.find(target_id) != textures.end());
	assert(target_id == BAD_RESID || textures[target_id]->getDesc().bindFlags & BIND_RENDER_TARGET);
	assert(depth_id == BAD_RESID || textures.find(depth_id) != textures.end());
	assert(depth_id == BAD_RESID || textures[depth_id]->getDesc().bindFlags & BIND_DEPTH_STENCIL);

	ID3D11RenderTargetView* rtv = target_id != BAD_RESID ? textures[target_id]->getRtv() : nullptr;
	ID3D11DepthStencilView* dsv = depth_id != BAD_RESID ? textures[depth_id]->getDsv() : nullptr;
	context->OMSetRenderTargets(1, &rtv, dsv);
}

void Driver::setRenderTargets(unsigned int num_targets, ResId* target_ids, ResId depth_id)
{
	assert(num_targets <= D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
	assert(depth_id == BAD_RESID || textures.find(depth_id) != textures.end());
	assert(depth_id == BAD_RESID || textures[depth_id]->getDesc().bindFlags & BIND_DEPTH_STENCIL);

	ID3D11DepthStencilView* dsv = depth_id != BAD_RESID ? textures[depth_id]->getDsv() : nullptr;

	ID3D11RenderTargetView* rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
	for (unsigned int i = 0; i < num_targets; i++)
	{
		assert(target_ids[i] == BAD_RESID || textures.find(target_ids[i]) != textures.end());
		assert(target_ids[i] == BAD_RESID || textures[target_ids[i]]->getDesc().bindFlags & BIND_RENDER_TARGET);

		rtvs[i] = target_ids[i] != BAD_RESID ? textures[target_ids[i]]->getRtv() : nullptr;
	}

	context->OMSetRenderTargets(num_targets, rtvs, dsv);
}

void Driver::setRenderState(ResId res_id)
{
	assert(res_id == BAD_RESID || renderStates.find(res_id) != renderStates.end());
	
	ResId resId = res_id != BAD_RESID ? res_id : defaultRenderState;
	context->RSSetState(renderStates[resId]->getRasterizerState());
	context->OMSetDepthStencilState(renderStates[resId]->getDepthStencilState(), 0);
}

void drv_d3d11::Driver::setShaderSet(ResId res_id)
{
	assert(shaders.find(res_id) != shaders.end());
	shaders[res_id]->set();
}

void drv_d3d11::Driver::setView(float x, float y, float w, float h, float z_min, float z_max)
{
	D3D11_VIEWPORT viewport{x, y, w, h, z_min, z_max};
	context->RSSetViewports(1, &viewport);
}

void drv_d3d11::Driver::draw(unsigned int vertex_count, unsigned int start_vertex)
{
	context->Draw(vertex_count, start_vertex);
}

void drv_d3d11::Driver::drawIndexed(unsigned int index_count, unsigned int start_index, int base_vertex)
{
	context->DrawIndexed(index_count, start_index, base_vertex);
}

void drv_d3d11::Driver::present()
{
	swapchain->Present(settings.vsync ? 1 : 0, 0);
}

void Driver::setSettings(const DriverSettings& new_settings)
{
	// TODO: handle each change in settings properly
	settings = new_settings;
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
	return emplace_resource(tex, textures);
}

void Driver::unregisterTexture(ResId id)
{
	erase_resource(id, textures);
}

ResId Driver::registerBuffer(Buffer* buf)
{
	return emplace_resource(buf, buffers);
}

void Driver::unregisterBuffer(ResId id)
{
	erase_resource(id, buffers);
}

ResId Driver::registerRenderState(RenderState* rs)
{
	return emplace_resource(rs, renderStates);
}

void Driver::unregisterRenderState(ResId id)
{
	erase_resource(id, renderStates);
}

ResId Driver::registerShaderSet(ShaderSet* shader_set)
{
	return emplace_resource(shader_set, shaders);
}

void Driver::unregisterShaderSet(ResId id)
{
	erase_resource(id, shaders);
}

ResId Driver::registerInputLayout(InputLayout* input_layout)
{
	return emplace_resource(input_layout, inputLayouts);
}

void Driver::unregisterInputLayout(ResId id)
{
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
		desc.hasSampler = false;
		backbuffer = std::make_unique<Texture>(desc, backBufferTexture);
	}

	// Create default render state
	{
		RenderStateDesc desc;
		defaultRenderState = createRenderState(desc);
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
	for (auto&& pair : pool)
	{
		assert(pair.second != nullptr);
		delete pair.second;
	}
	pool.clear();
}

void drv_d3d11::Driver::releaseAllResources()
{
	// Textures and buffers should be destroyed by the user
	assert(textures.size() == 0);
	release_pool(textures);
	assert(buffers.size() == 0);
	release_pool(buffers);

	// These are destroyed automatically
	// TODO: implement a way for the user to destroy these as well to avoid bloating the driver with these
	release_pool(renderStates);
	release_pool(shaders);
	release_pool(inputLayouts);
}
