#include "Camera.h"

#include <3rdParty/glm/glm.hpp>
#include <3rdParty/imgui/imgui.h>
#include <Util/AutoImGui.h>

#include "WorldRenderer.h"

Camera::Camera()
{
	eye = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	pitch = yaw = 0.0f;
	RecalculateViewMatrix();

	viewportWidth = 1280.0f;
	viewportHeight = 720.0f;
	fov = XM_PI / 3.0f;
	nearPlane = 0.1f;
	farPlane = 100.0f;
	RecalculateProjectionMatrix();
}

Camera::~Camera()
{
}

void Camera::SetEye(const XMVECTOR& eye)
{
	this->eye = eye;
	RecalculateViewMatrix();
}

void Camera::MoveEye(const XMVECTOR& direction)
{
	SetEye(eye + direction);
}

void Camera::SetRotation(float pitch, float yaw)
{
	pitch = glm::clamp(pitch, -XM_PIDIV2, +XM_PIDIV2);
	this->pitch = pitch;

	while (yaw > XM_PI)
		yaw -= XM_2PI;
	while (yaw < -XM_PI)
		yaw += XM_2PI;
	this->yaw = yaw;

	RecalculateViewMatrix();
}

void Camera::Rotate(float deltaPitch, float deltaYaw)
{
	SetRotation(pitch + deltaPitch, yaw + deltaYaw);
}

void Camera::SetViewParams(const XMVECTOR& eye, const XMVECTOR& up, float pitch, float yaw)
{
	this->eye = eye;
	this->up = up;
	this->pitch = pitch;
	this->yaw = yaw;
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

void Camera::Resize(float viewportWidth, float viewportHeight)
{
	this->viewportWidth = viewportWidth;
	this->viewportHeight = viewportHeight;
	RecalculateProjectionMatrix();
}

void Camera::gui()
{
	bool viewChanged = false;
	viewChanged |= ImGui::DragFloat3("Eye", eye.m128_f32, 0.01f);
	viewChanged |= ImGui::SliderAngle("Yaw", &yaw, -180.0f, 180.0f);
	viewChanged |= ImGui::SliderAngle("Pitch", &pitch, -90.0f, 90.0f);
	if (viewChanged)
		RecalculateViewMatrix();
	bool projChanged = false;
	projChanged |= ImGui::SliderAngle("FoV", &fov, 1.0f, 180.0f);
	projChanged |= ImGui::DragFloatRange2("Clipping planes", &nearPlane, &farPlane, 0.1f, 0.1f, 1000.0f, "%.1f");
	if (projChanged)
	{
		fov = fmaxf(fov, to_rad(1.0f));
		nearPlane = fmaxf(nearPlane, 0.1f);
		farPlane = fmaxf(farPlane, nearPlane + 0.1f);
		RecalculateProjectionMatrix();
	}
}
REGISTER_IMGUI_WINDOW("Camera", []() { if (wr != nullptr) wr->getSceneCamera().gui(); });

void Camera::RecalculateViewMatrix()
{
	XMMATRIX cameraRotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f);
	viewMatrix = XMMatrixLookToLH(eye, cameraRotationMatrix.r[2], up);
	XMStoreFloat4x4(&viewMatrixF4X4, viewMatrix);
}

void Camera::RecalculateProjectionMatrix()
{
	projectionMatrix = XMMatrixPerspectiveFovLH(fov, viewportWidth / viewportHeight, nearPlane, farPlane);
	XMStoreFloat4x4(&projectionMatrixF4X4, projectionMatrix);
}
