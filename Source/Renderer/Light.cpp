#include "Light.h"

Light::Light()
{
	intensity = 1.0f;
	color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	yaw = pitch = 0.0f;
}

XMVECTOR Light::GetDirection() const
{
	float cosPitch = cosf(pitch);
	float x = sinf(yaw) * cosPitch;
	float y = -sinf(pitch);
	float z = cosf(yaw) * cosPitch;
	return XMVectorSet(x, y, z, 0);
}
