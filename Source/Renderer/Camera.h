#pragma once

#include "RendererCommon.h"

__declspec(align(16)) class Camera
{
public:

	Camera();
	~Camera();

	void* operator new(size_t i) { return _mm_malloc(i, 16); }
	void operator delete(void* p) { _mm_free(p); }

	const XMVECTOR& GetEye() const { return eye; }
	void SetEye(const XMVECTOR& eye);
	void MoveEye(const XMVECTOR& direction);
	void SetRotation(float pitch, float yaw);
	void Rotate(float deltaPitch, float deltaYaw);
	const XMVECTOR& GetUp() const { return up; }
	XMVECTOR GetForward() const { return XMVectorSet(viewMatrixF4X4.m[0][2], viewMatrixF4X4.m[1][2], viewMatrixF4X4.m[2][2], 0.0f); }
	XMVECTOR GetRight() const { return XMVector3Normalize(XMVector3Cross(up, GetForward())); }
	float GetYaw() const { return yaw; }
	float GetPitch() const { return pitch; }
	void SetViewParams(const XMVECTOR& eye, const XMVECTOR& up, float pitch, float yaw);
	const XMMATRIX& GetViewMatrix() const { return viewMatrix; }

	float GetViewportWidth() const { return viewportWidth; }
	float GetViewportHeight() const { return viewportHeight; }
	float GetFOV() const { return fov; }
	float GetNearPlane() const { return nearPlane; }
	float GetFarPlane() const { return farPlane; }
	void SetProjectionParams(float viewportWidth, float viewportHeight, float fov, float nearPlane, float farPlane);
	const XMMATRIX& GetProjectionMatrix() const { return projectionMatrix; }
	void Resize(float viewportWidth, float viewportHeight);

	void gui();

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

