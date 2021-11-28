#pragma once

#include <memory>

#include <Common.h>
#include <Util/ResIdHolder.h>
#include <Engine/Transform.h>

class IBuffer;

struct WaterCbData
{
	XMMATRIX worldTransform;
};

class Water
{
public:
	Water(const Transform& transform_);
	void onGlobalShaderKeywordsChanged();
	void render();

private:
	Transform transform;

	ResIdHolder shader;
	unsigned int currentVariant = 0;

	ResIdHolder renderState;

	WaterCbData cbData;
	std::unique_ptr<IBuffer> cb;
};