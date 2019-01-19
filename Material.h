#pragma once

#include "EngineCommon.h"

class Material
{

public:
	Material();
	~Material();

	void SetPaths(const char *baseTexturePath, const char *normalTexturePath);
	void LoadTextures();
	void SetToContext(ID3D11DeviceContext *context);

private:
	const char *baseTexturePath, *normalTexturePath;

	ID3D11ShaderResourceView *baseTextureRV;
	ID3D11ShaderResourceView *normalTextureRV;

};

