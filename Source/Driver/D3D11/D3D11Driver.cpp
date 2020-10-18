#include "D3D11Driver.h"

#include <assert.h>

#include "D3D11Texture.h"
#include "D3D11Buffer.h"
#include "D3D11RenderState.h"

static std::unique_ptr<D3D11Driver> driver;
D3D11Driver* get_drv() { return driver.get(); };

static ResId nextAvailableResId = 0;

bool D3D11Driver::init(void* hwnd, int display_width, int display_height)
{
	HRESULT hr;

	DXGI_SWAP_CHAIN_DESC scd{};
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = 1;
	scd.Windowed = true;

	hr = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
		&scd, &swapchain, &device, nullptr, &context);
	assert(SUCCEEDED(hr));

	driver.reset(this);

	return true;
}

void D3D11Driver::shutdown()
{
	swapchain.Reset();
	context.Reset();
	device.Reset();

	driver.reset();
}

void D3D11Driver::resize(int display_width, int display_height)
{
	closeResolutionDependentResources();
	initResolutionDependentResources(display_width, display_height);
}

ITexture* D3D11Driver::createTexture(const TextureDesc& desc)
{
	return new D3D11Texture(desc);
}

IBuffer* D3D11Driver::createBuffer(const BufferDesc& desc)
{
	return new D3D11Buffer(desc);
}

ResId D3D11Driver::createRenderState(const RenderStateDesc& desc)
{
	D3D11RenderState* rs = new D3D11RenderState(desc);
	return rs->getId();
}

void D3D11Driver::setIndexBuffer(ResId* res_id)
{
	assert(res_id == nullptr || buffers.find(*res_id) != buffers.end());
	assert(res_id == nullptr || buffers[*res_id]->getDesc().bindFlags & BIND_INDEX_BUFFER);

	ID3D11Buffer* res = res_id != nullptr ? buffers[*res_id]->getResource() : nullptr;
	context->IASetIndexBuffer(res, DXGI_FORMAT_R16_UINT, 0);
}

void D3D11Driver::setVertexBuffer(ResId* res_id)
{
	assert(res_id == nullptr || buffers.find(*res_id) != buffers.end());
	assert(res_id == nullptr || buffers[*res_id]->getDesc().bindFlags & BIND_VERTEX_BUFFER);

	ID3D11Buffer* res = res_id != nullptr ? buffers[*res_id]->getResource() : nullptr;
	unsigned int stride = res_id != nullptr ? buffers[*res_id]->getDesc().elementByteSize : 0;
	context->IASetVertexBuffers(0, 1, &res, &stride, nullptr);
}

void D3D11Driver::setConstantBuffer(ShaderStage stage, unsigned int slot, ResId* res_id)
{
	assert(res_id == nullptr || buffers.find(*res_id) != buffers.end());
	assert(res_id == nullptr || buffers[*res_id]->getDesc().bindFlags & BIND_CONSTANT_BUFFER);

	ID3D11Buffer* res = res_id != nullptr ? buffers[*res_id]->getResource() : nullptr;
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

void D3D11Driver::setBuffer(ShaderStage stage, unsigned int slot, ResId* res_id)
{
	assert(res_id == nullptr || buffers.find(*res_id) != buffers.end());
	assert(res_id == nullptr || buffers[*res_id]->getDesc().bindFlags & BIND_SHADER_RESOURCE);

	ID3D11ShaderResourceView* srv = res_id != nullptr ? buffers[*res_id]->getSrv() : nullptr;
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

void D3D11Driver::setRwBuffer(unsigned int slot, ResId* res_id)
{
	assert(res_id == nullptr || buffers.find(*res_id) != buffers.end());
	assert(res_id == nullptr || buffers[*res_id]->getDesc().bindFlags & BIND_UNORDERED_ACCESS);

	ID3D11UnorderedAccessView* uav = res_id != nullptr ? buffers[*res_id]->getUav() : nullptr;
	context->CSSetUnorderedAccessViews(slot, 1, &uav, nullptr);
}

void D3D11Driver::setTexture(ShaderStage stage, unsigned int slot, ResId* res_id)
{
	assert(res_id == nullptr || textures.find(*res_id) != textures.end());
	assert(res_id == nullptr || textures[*res_id]->getDesc().bindFlags & BIND_SHADER_RESOURCE);

	ID3D11ShaderResourceView* srv = res_id != nullptr ? textures[*res_id]->getSrv() : nullptr;
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

void D3D11Driver::setRwTexture(unsigned int slot, ResId* res_id)
{
	assert(res_id == nullptr || textures.find(*res_id) != textures.end());
	assert(res_id == nullptr || textures[*res_id]->getDesc().bindFlags & BIND_UNORDERED_ACCESS);

	ID3D11UnorderedAccessView* uav = res_id != nullptr ? textures[*res_id]->getUav() : nullptr;
	context->CSSetUnorderedAccessViews(slot, 1, &uav, nullptr);
}

void D3D11Driver::setRenderTarget(ResId* target_id, ResId* depth_id)
{
	assert(target_id == nullptr || textures.find(*target_id) != textures.end());
	assert(target_id == nullptr || textures[*target_id]->getDesc().bindFlags & BIND_RENDER_TARGET);
	assert(depth_id == nullptr || textures.find(*depth_id) != textures.end());
	assert(depth_id == nullptr || textures[*depth_id]->getDesc().bindFlags & BIND_DEPTH_STENCIL);

	ID3D11RenderTargetView* rtv = target_id != nullptr ? textures[*target_id]->getRtv() : nullptr;
	ID3D11DepthStencilView* dsv = depth_id != nullptr ? textures[*depth_id]->getDsv() : nullptr;
	context->OMSetRenderTargets(1, &rtv, dsv);
}

void D3D11Driver::setRenderTargets(unsigned int num_targets, ResId** target_ids, ResId* depth_id)
{
	assert(num_targets <= D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
	assert(depth_id == nullptr || textures.find(*depth_id) != textures.end());
	assert(depth_id == nullptr || textures[*depth_id]->getDesc().bindFlags & BIND_DEPTH_STENCIL);
	
	ID3D11RenderTargetView* rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
	for (unsigned int i = 0; i < num_targets; i++)
	{
		assert(target_ids[i] == nullptr || textures.find(*target_ids[i]) != textures.end());
		assert(target_ids[i] == nullptr || textures[*target_ids[i]]->getDesc().bindFlags & BIND_RENDER_TARGET);

		rtvs[i] = target_ids[i] != nullptr ? textures[*target_ids[i]]->getRtv() : nullptr;
	}
	ID3D11DepthStencilView* dsv = depth_id != nullptr ? textures[*depth_id]->getDsv() : nullptr;

	context->OMSetRenderTargets(num_targets, rtvs, dsv);
}

void D3D11Driver::setRenderState(ResId* res_id)
{
	assert(res_id == nullptr || renderStates.find(*res_id) != renderStates.end());
	
	ResId resId = res_id != nullptr ? *res_id : defaultRenderState;
	context->RSSetState(renderStates[resId]->getRasterizerState());
	context->OMSetDepthStencilState(renderStates[resId]->getDepthStencilState(), 0);
}

void D3D11Driver::setSettings(const DriverSettings& new_settings)
{
	// TODO: handle each change in settings properly
	settings = new_settings;
}

ResId D3D11Driver::registerTexture(D3D11Texture* tex)
{
	ResId resId = nextAvailableResId++;
	auto&& emplaceResult = textures.emplace(resId, tex);
	assert(emplaceResult.second);
	return resId;
}

void D3D11Driver::unregisterTexture(ResId id)
{
	size_t numElementsErased = textures.erase(id);
	assert(numElementsErased > 0);
}

ResId D3D11Driver::registerBuffer(D3D11Buffer* buf)
{
	ResId resId = nextAvailableResId++;
	auto&& emplaceResult = buffers.emplace(resId, buf);
	assert(emplaceResult.second);
	return resId;
}

void D3D11Driver::unregisterBuffer(ResId id)
{
	size_t numElementsErased = buffers.erase(id);
	assert(numElementsErased > 0);
}

ResId D3D11Driver::registerRenderState(D3D11RenderState* rs)
{
	ResId resId = nextAvailableResId++;
	auto&& emplaceResult = renderStates.emplace(resId, rs);
	assert(emplaceResult.second);
	return resId;
}

void D3D11Driver::unregisterRenderState(ResId id)
{
	size_t numElementsErased = renderStates.erase(id);
	assert(numElementsErased > 0);
}

bool D3D11Driver::initResolutionDependentResources(int display_width, int display_height)
{
	HRESULT hr;

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
		backbuffer = std::make_unique<D3D11Texture>(desc, backBufferTexture);
	}

	// Create default render state
	{
		RenderStateDesc desc;
		defaultRenderState = createRenderState(desc);
	}

	return true;
}

void D3D11Driver::closeResolutionDependentResources()
{
	backbuffer.reset();
}