#include "Material.h"

#include "Engine.h"

Material::Material()
{
	baseTexturePath = normalTexturePath = "";
}

Material::~Material()
{
	SAFE_RELEASE(baseTextureRV);
	SAFE_RELEASE(normalTextureRV);
}

void Material::SetPaths(const char *baseTexturePath, const char *normalTexturePath)
{
	this->baseTexturePath = baseTexturePath;
	this->normalTexturePath = normalTexturePath;
}

void Material::LoadTextures()
{
	baseTextureRV = gEngine->LoadRGBATextureFromPNG(baseTexturePath);
	// TODO: normalTexture
}

void Material::SetToContext(ID3D11DeviceContext *context)
{
	context->PSSetShaderResources(0, 1, &baseTextureRV);
	// TODO: normalTexture
}
