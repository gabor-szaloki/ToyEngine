#include "Drawable.h"

#include "LodePNG/lodepng.h"

Drawable::Drawable()
{
	worldTransform = XMMatrixIdentity();
	indexCount = 0;
}

Drawable::~Drawable()
{
	SAFE_RELEASE(vertexBuffer);
	SAFE_RELEASE(indexBuffer);

	SAFE_RELEASE(texture2d);
	SAFE_RELEASE(textureRV);
}

void Drawable::Draw(ID3D11DeviceContext *context, ID3D11Buffer *perObjectCB)
{
	UINT stride = sizeof(StandardVertexData);
	UINT offset = 0;

	context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	PerObjectConstantBufferData perObjectCBData;
	perObjectCBData.world = worldTransform;

	context->UpdateSubresource(perObjectCB, 0, nullptr, &perObjectCBData, 0, 0);
	context->VSSetConstantBuffers(1, 1, &perObjectCB);
	context->PSSetConstantBuffers(1, 1, &perObjectCB);

	context->PSSetShaderResources(0, 1, &textureRV);
	context->PSSetSamplers(0, 1, &sampler);

	context->DrawIndexed(indexCount, 0, 0);
}

void Drawable::InitBuffers(ID3D11Device *device, ID3D11DeviceContext *context, 
	StandardVertexData *vertices, UINT vertexCount, WORD *indices, UINT indexCount)
{
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));
	bd.Usage = D3D11_USAGE_DYNAMIC; // write access access by CPU and GPU
	bd.ByteWidth = sizeof(StandardVertexData) * vertexCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
	initData.pSysMem = vertices;

	if (FAILED(device->CreateBuffer(&bd, &initData, &vertexBuffer)))
		throw;

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * indexCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	initData.pSysMem = indices;

	if (FAILED(device->CreateBuffer(&bd, &initData, &indexBuffer)))
		throw;

	this->indexCount = indexCount;


	// init texture
	std::vector<unsigned char> textureData;
	UINT width, height;
	UINT error = lodepng::decode(textureData, width, height, "Textures\\Tiles20_col_smooth.png");
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
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem = &textureData[0];
	subResource.SysMemPitch = td.Width * 4;
	subResource.SysMemSlicePitch = 0;

	HRESULT result = device->CreateTexture2D(&td, &subResource, &texture2d);
	if (FAILED(result))
		OutputDebugString("issue creating texture\n");

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = td.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	result = device->CreateShaderResourceView(texture2d, &srvDesc, &textureRV);
	if (FAILED(result))
		OutputDebugString("issue creating shaderResourceView \n");

	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(D3D11_SAMPLER_DESC));
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.MipLODBias = 0.f;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sd.MinLOD = 0.f;
	sd.MaxLOD = D3D11_FLOAT32_MAX;
	result = device->CreateSamplerState(&sd, &sampler);
	if (FAILED(result))
		OutputDebugString("problem creating sampler\n");
}
