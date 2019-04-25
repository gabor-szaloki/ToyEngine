#pragma once

#include "EngineCommon.h"

class Material
{

public:
	Material(
		ID3D11VertexShader *forwardVS, ID3D11PixelShader *forwardPS,
		ID3D11VertexShader *shadowVS, ID3D11PixelShader *shadowPS,
		ID3D11ShaderResourceView *baseTextureRV, 
		ID3D11ShaderResourceView *normalTextureRV);
	~Material();

	void SetToContext(ID3D11DeviceContext *context, bool shadowPass);

private:

	ID3D11ShaderResourceView *baseTextureRV;
	ID3D11ShaderResourceView *normalTextureRV;

	ID3D11VertexShader *forwardVS, *shadowVS;
	ID3D11PixelShader *forwardPS, *shadowPS;

};