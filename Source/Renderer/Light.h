#pragma once

#include <Common.h>

class Light
{
public:

	Light();

	float GetIntensity() const { return intensity; }
	void SetIntensity(float intensity_) { intensity = intensity_; }
	XMFLOAT4 GetColor() const { return color; }
	void SetColor(XMFLOAT4 color_) { color = color_; }
	float GetYaw() const { return yaw; }
	void SetYaw(float yaw_) { yaw = yaw_; }
	float GetPitch() const { return pitch; }
	void SetPitch(float pitch_) { pitch = pitch_; }
	void SetRotation(float yaw_, float pitch_) { yaw = yaw_; pitch = pitch_; }
	XMVECTOR GetDirection() const;

private:

	float intensity;
	XMFLOAT4 color;
	float yaw, pitch;
};

