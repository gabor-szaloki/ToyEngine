#include "TextureD3D12.h"

namespace drv_d3d12
{

static D3D12_RESOURCE_DESC convert_desc(const TextureDesc& desc)
{
	D3D12_RESOURCE_DESC td;

	td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	td.Alignment = 0;
	td.Width = desc.width;
	td.Height = desc.height;
	td.DepthOrArraySize = (desc.miscFlags & RESOURCE_MISC_TEXTURECUBE) ? 6 : 1;
	td.MipLevels = desc.mips;
	td.Format = (DXGI_FORMAT)desc.format;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;
	td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	td.Flags = D3D12_RESOURCE_FLAG_NONE;
	if (desc.bindFlags & BIND_RENDER_TARGET)
		td.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	if (desc.bindFlags & BIND_DEPTH_STENCIL)
		td.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (desc.bindFlags & BIND_UNORDERED_ACCESS)
		td.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	return td;
}

Texture::Texture(const TextureDesc* desc_, ID3D12Resource* tex) : resource(tex)
{
	if (desc_ != nullptr)
	{
		desc = *desc_;
		numMips = desc.calcMipLevels();

		if (resource == nullptr)
		{
			createResource();
		}
		else
		{
			// TODO: Check if desc matches with given desc of resource
		}

		createViews();
	}
	else
	{
		isStub_ = true;
	}

	id = DriverD3D12::get().registerTexture(this);
}

Texture::~Texture()
{
	releaseAll();
	DriverD3D12::get().unregisterTexture(id);
}

void Texture::updateData(unsigned int dst_subresource, const IntBox* dst_box, const void* src_data)
{
	ASSERT_NOT_IMPLEMENTED;
	/*
	unsigned int rowPitch = desc.width * get_byte_size_for_texfmt(desc.format);

	CONTEXT_LOCK_GUARD
	if (dst_box != nullptr)
	{
		D3D11_BOX dstBox;
		dstBox.left   = dst_box->left;   assert(dst_box->left >= 0);
		dstBox.top    = dst_box->top;    assert(dst_box->top >= 0);
		dstBox.front  = dst_box->front;  assert(dst_box->front >= 0);
		dstBox.right  = dst_box->right;  assert(dst_box->right >= 0);
		dstBox.bottom = dst_box->bottom; assert(dst_box->bottom >= 0);
		dstBox.back   = dst_box->back;   assert(dst_box->back >= 0);
		Driver::get().getContext().UpdateSubresource(resource, dst_subresource, &dstBox, src_data, rowPitch, 0);
	}
	else
	{
		Driver::get().getContext().UpdateSubresource(resource, dst_subresource, nullptr, src_data, rowPitch, 0);
	}
	*/
}

void Texture::copyResource(ResId dst_tex_id) const
{
	ASSERT_NOT_IMPLEMENTED;
	//Driver::get().copyTexture(dst_tex_id, id);
}

void Texture::generateMips()
{
	ASSERT_NOT_IMPLEMENTED;
	/*
	assert(desc.miscFlags & RESOURCE_MISC_GENERATE_MIPS);
	assert(desc.bindFlags & BIND_RENDER_TARGET);
	assert(srv != nullptr);
	CONTEXT_LOCK_GUARD
	Driver::get().getContext().GenerateMips(srv);
	*/
}

void Texture::recreate(const TextureDesc& desc_)
{
	releaseAll();

	desc = desc_;
	numMips = desc.calcMipLevels();
	createResource();
	createViews();

	isStub_ = false;
}

void Texture::transition(D3D12_RESOURCE_STATES dest_state, ID3D12GraphicsCommandList2* cmd_list)
{
	if (dest_state == resourceState)
		return;

	DriverD3D12::get().transitionResource(resource, resourceState, dest_state, cmd_list);
	resourceState = dest_state;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Texture::getRtv(unsigned int slice, unsigned int mip) const
{
	unsigned int subresourceIndex = calc_subresource(mip, slice, numMips);
	return rtvs[subresourceIndex];
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Texture::getDsv(unsigned int slice, unsigned int mip) const
{
	unsigned int subresourceIndex = calc_subresource(mip, slice, numMips);
	return dsvs[subresourceIndex];
}

void Texture::createResource()
{
	CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC resourceDesc = convert_desc(desc);
	verify(DriverD3D12::get().getDevice().CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		resourceState,
		nullptr, // TODO: expose optimized clear value to TextureDesc
		IID_PPV_ARGS(&resource)
	));

	set_debug_name(resource, desc.name);
}

void Texture::createViews()
{
	const bool isCubemap = desc.miscFlags & RESOURCE_MISC_TEXTURECUBE;

	if (desc.bindFlags & BIND_SHADER_RESOURCE)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = (DXGI_FORMAT)desc.getSrvFormat();

		if (isCubemap)
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = desc.mips == 0 ? -1 : desc.mips;
			srvDesc.TextureCube.MostDetailedMip = 0;
			srvDesc.TextureCube.ResourceMinLODClamp = 0.f;
		}
		else
		{
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = desc.mips == 0 ? -1 : desc.mips;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
		}
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		srv = DriverD3D12::get().createShaderResourceView(resource, &srvDesc);
	}
	if (desc.bindFlags & BIND_UNORDERED_ACCESS)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = (DXGI_FORMAT)desc.getUavFormat();

		if (isCubemap)
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.MipSlice = 0;
			uavDesc.Texture2DArray.FirstArraySlice = 0;
			uavDesc.Texture2DArray.ArraySize = 6;
			uavDesc.Texture2DArray.PlaneSlice = 0;
		}
		else
		{
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = 0;
			uavDesc.Texture2D.PlaneSlice = 0;
		}

		uav = DriverD3D12::get().createUnorderedAccessView(resource, &uavDesc);
	}
	if (desc.bindFlags & BIND_RENDER_TARGET)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = (DXGI_FORMAT)desc.getRtvFormat();

		if (isCubemap)
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.ArraySize = 1;
			rtvDesc.Texture2DArray.PlaneSlice = 0;
			for (int i = 0; i < 6; i++)
			{
				rtvDesc.Texture2DArray.FirstArraySlice = i;
				for (unsigned int mip = 0; mip < numMips; mip++)
				{
					rtvDesc.Texture2DArray.MipSlice = mip;
					CD3DX12_CPU_DESCRIPTOR_HANDLE rtv = DriverD3D12::get().createRenderTargetView(resource, &rtvDesc);
					rtvs.push_back(rtv);
				}
			}
		}
		else
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			rtvDesc.Texture2D.PlaneSlice = 0;
			for (unsigned int mip = 0; mip < numMips; mip++)
			{
				rtvDesc.Texture2D.MipSlice = mip;
				CD3DX12_CPU_DESCRIPTOR_HANDLE rtv = DriverD3D12::get().createRenderTargetView(resource, &rtvDesc);
				rtvs.push_back(rtv);
			}
		}

	}
	if (desc.bindFlags & BIND_DEPTH_STENCIL)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = (DXGI_FORMAT)desc.getDsvFormat();

		if (isCubemap)
		{
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			dsvDesc.Texture2DArray.ArraySize = 1;
			for (int i = 0; i < 6; i++)
			{
				dsvDesc.Texture2DArray.FirstArraySlice = i;
				for (unsigned int mip = 0; mip < numMips; mip++)
				{
					dsvDesc.Texture2DArray.MipSlice = mip;
					CD3DX12_CPU_DESCRIPTOR_HANDLE dsv = DriverD3D12::get().createDepthStencilView(resource, &dsvDesc);
					dsvs.push_back(dsv);
				}
			}
		}
		else
		{
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			for (unsigned int mip = 0; mip < numMips; mip++)
			{
				dsvDesc.Texture2D.MipSlice = mip;
				CD3DX12_CPU_DESCRIPTOR_HANDLE dsv = DriverD3D12::get().createDepthStencilView(resource, &dsvDesc);
				dsvs.push_back(dsv);
			}
		}

	}
}

void Texture::releaseAll()
{
	safe_release(resource);
	rtvs.clear();
	dsvs.clear();
}

}