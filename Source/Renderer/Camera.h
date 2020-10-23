#pragma once

#include "RendererCommon.h"

__declspec(align(16)) class Camera
{
public:

	Camera();
	~Camera();

	void* operator new(size_t i) { return _mm_malloc(i, 16); }
	void operator delete(void* p) { _mm_free(p); }

	XMVECTOR GetEye() { return eye; }
	void SetEye(XMVECTOR eye);
	void MoveEye(XMVECTOR direction);
	void SetRotation(float pitch, float yaw);
	void Rotate(float deltaPitch, float deltaYaw);
	XMVECTOR GetUp() { return up; }
	XMVECTOR GetForward() { return XMVectorSet(viewMatrixF4X4.m[0][2], viewMatrixF4X4.m[1][2], viewMatrixF4X4.m[2][2], 0.0f); }
	XMVECTOR GetRight() { return XMVector3Cross(up, GetForward()); }
	void SetViewParams(XMVECTOR eye, XMVECTOR up, float pitch, float yaw);
	XMMATRIX GetViewMatrix() { return viewMatrix; }

	float GetViewportWidth() { return viewportWidth; }
	float GetViewportHeight() { return viewportHeight; }
	float GetFOV() { return fov; }
	float GetNearPlane() { return nearPlane; }
	float GetFarPlane() { return farPlane; }
	void SetProjectionParams(float viewportWidth, float viewportHeight, float fov, float nearPlane, float farPlane);
	XMMATRIX GetProjectionMatrix() { return projectionMatrix; }

private:

	XMVECTOR eye, up;
	float pitch, yaw;

	float viewportWidth, viewportHeight;
	float fov, nearPlane, farPlane;

	XMMATRIX viewMatrix;
	XMFLOAT4X4 viewMatrixF4X4;
	XMMATRIX projectionMatrix;
	XMFLOAT4X4 projectionMatrixF4X4;

	void RecalculateViewMatrix();
	void RecalculateProjectionMatrix();
};

