#pragma once

#include "Common.h"

__declspec(align(16)) class Camera
{
public:

	Camera();
	~Camera();

	void* operator new(size_t i) { return _mm_malloc(i, 16); }
	void operator delete(void* p) { _mm_free(p); }

	XMVECTOR GetEye() { return eye; }
	XMVECTOR GetAt() { return at; }
	XMVECTOR GetUp() { return up; }
	XMVECTOR GetDirection() { return XMVector3Normalize(at - eye); }
	void SetViewParams(XMVECTOR eye, XMVECTOR at, XMVECTOR up);
	XMMATRIX GetViewMatrix() { return viewMatrix; }

	float GetViewportWidth() { return viewportWidth; }
	float GetViewportHeight() { return viewportHeight; }
	float GetFOV() { return fov; }
	float GetNearPlane() { return nearPlane; }
	float GetFarPlane() { return farPlane; }
	void SetProjectionParams(float viewportWidth, float viewportHeight, float fov, float nearPlane, float farPlane);
	XMMATRIX GetProjectionMatrix() { return projectionMatrix; }

private:

	XMVECTOR eye, at, up;

	float viewportWidth, viewportHeight;
	float fov, nearPlane, farPlane;

	XMMATRIX viewMatrix;
	XMMATRIX projectionMatrix;

	void RecalculateViewMatrix();
	void RecalculateProjectionMatrix();
};

