#include "Material.h"

#include "Engine.h"

Material::Material(
	ID3D11VertexShader *vertexShader,
	ID3D11PixelShader *pixelShader,
	ID3D11ShaderResourceView *baseTextureRV,
	ID3D11ShaderResourceView *normalTextureRV)
{
	this->vertexShader = vertexShader;
	this->pixelShader = pixelShader;
	this->baseTextureRV = baseTextureRV;
	this->normalTextureRV = normalTextureRV;
}

Material::~Material()
{
}

void Material::SetToContext(ID3D11DeviceContext *context)
{
	context->VSSetShader(vertexShader, nullptr, 0);
	context->PSSetShader(pixelShader, nullptr, 0);
	context->PSSetShaderResources(0, 1, &baseTextureRV);
	// TODO: normalTexture
}
