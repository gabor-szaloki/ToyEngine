#pragma once

#include <Common.h>

enum class RenderPass
{
	FORWARD,
	SHADOW,
	_COUNT // Not an actual render pass, only used for enumeration, or declaring arrays
};