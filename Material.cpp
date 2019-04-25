#include "Material.h"

#include "Engine.h"

Material::Material(
	ID3D11VertexShader *forwardVS, ID3D11PixelShader *forwardPS,
	ID3D11VertexShader *shadowVS, ID3D11PixelShader *shadowPS,
	ID3D11ShaderResourceView *baseTextureRV,
	ID3D11ShaderResourceView *normalTextureRV)
{
	this->forwardVS = forwardVS;
	this->forwardPS = forwardPS;
	this->shadowVS = shadowVS;
	this->shadowPS = shadowPS;

	this->baseTextureRV = baseTextureRV;
	this->normalTextureRV = normalTextureRV;
}

Material::~Material()
{
}

void Material::SetToContext(ID3D11DeviceContext *context, bool shadowPass)
{
	context->VSSetShader(shadowPass ? shadowVS : forwardVS, nullptr, 0);
	context->PSSetShader(shadowPass ? shadowPS : forwardPS, nullptr, 0);
	context->PSSetShaderResources(0, 1, &baseTextureRV);
	context->PSSetShaderResources(1, 1, &normalTextureRV);
}
