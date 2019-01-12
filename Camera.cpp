#include "Camera.h"

Camera::Camera()
{
	eye = XMVectorSet(0.0f, 0.0f, -5.0f, 0.0f);
	at = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	RecalculateViewMatrix();

	viewportWidth = 1280.0f;
	viewportHeight = 720.0f;
	fov = XM_PIDIV2;
	nearPlane = 0.1f;
	farPlane = 100.0f;
	RecalculateProjectionMatrix();
}

Camera::~Camera()
{
}

void Camera::SetViewParams(XMVECTOR eye, XMVECTOR at, XMVECTOR up)
{
	this->eye = eye;
	this->at = at;
	this->up = up;
	RecalculateViewMatrix();
}

void Camera::SetProjectionParams(float viewportWidth, float viewportHeight, float fov, float nearPlane, float farPlane)
{
	this->viewportWidth = viewportWidth;
	this->viewportHeight = viewportHeight;
	this->fov = fov;
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
	RecalculateProjectionMatrix();
}

void Camera::RecalculateViewMatrix()
{
	viewMatrix = XMMatrixLookAtLH(eye, at, up);
}

void Camera::RecalculateProjectionMatrix()
{
	projectionMatrix = XMMatrixPerspectiveFovLH(fov, viewportWidth / viewportHeight, nearPlane, farPlane);
}
