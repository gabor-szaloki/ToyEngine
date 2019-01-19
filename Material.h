#pragma once

#include "EngineCommon.h"

class Material
{

public:
	Material(
		ID3D11VertexShader *vertexShader, 
		ID3D11PixelShader *pixelShader, 
		ID3D11ShaderResourceView *baseTextureRV, 
		ID3D11ShaderResourceView *normalTextureRV);
	~Material();

	void SetToContext(ID3D11DeviceContext *context);

private:

	ID3D11ShaderResourceView *baseTextureRV;
	ID3D11ShaderResourceView *normalTextureRV;

	ID3D11VertexShader *vertexShader;
	ID3D11PixelShader *pixelShader;

};