#pragma once

#include <Common.h>

class Light
{
public:

	Light();
	~Light();

	float GetIntensity();
	void SetIntensity(float intensity);
	XMFLOAT4 GetColor();
	void SetColor(XMFLOAT4 color);
	float GetYaw();
	void SetYaw(float yaw);
	float GetPitch();
	void SetPitch(float pitch);
	void SetRotation(float yaw, float pitch);

private:

	float intensity;
	XMFLOAT4 color;
	float yaw, pitch;
};

