#include "Light.h"

Light::Light()
{
	intensity = 1.0f;
	color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	yaw = pitch = 0.0f;
}

Light::~Light()
{
}

float Light::GetIntensity()
{
	return intensity;
}

void Light::SetIntensity(float intensity)
{
	this->intensity = intensity;
}

XMFLOAT4 Light::GetColor()
{
	return color;
}

void Light::SetColor(XMFLOAT4 color)
{
	this->color = color;
}

float Light::GetYaw()
{
	return yaw;
}

void Light::SetYaw(float yaw)
{
	this->yaw = yaw;
}

float Light::GetPitch()
{
	return pitch;
}

void Light::SetPitch(float pitch)
{
	this->pitch = pitch;
}

void Light::SetRotation(float yaw, float pitch)
{
	this->yaw = yaw;
	this->pitch = pitch;
}
