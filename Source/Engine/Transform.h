#pragma once

#include <Common.h>

struct Transform
{
	XMVECTOR position = XMVectorZero();
	XMVECTOR rotation = XMVectorZero();
	float scale = 1.0f;

	Transform() {}
	Transform(XMVECTOR position_) : position(position_) {}
	Transform(XMVECTOR position_, XMVECTOR rotation_, float scale_ = 1.0f)
		: position(position_), rotation(rotation_), scale(scale_) {}
	Transform(XMFLOAT3 position_) : Transform(XMLoadFloat3(&position_)) {}
	Transform(XMFLOAT3 position_, XMFLOAT3 rotation_, float scale_ = 1.0f)
		: Transform(XMLoadFloat3(&position_), XMLoadFloat3(&rotation_), scale_) {}

	XMMATRIX getMatrix() const
	{
		return XMMatrixAffineTransformation(
			XMVectorSet(scale, scale, scale, scale),
			XMVectorZero(),
			XMQuaternionRotationRollPitchYawFromVector(rotation),
			position);
	}
};