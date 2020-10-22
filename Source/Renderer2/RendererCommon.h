#pragma once

#include <Common.h>

enum class RenderPass
{
	FORWARD,
	SHADOW,
	_COUNT // Not an actual render pass, only used for enumeration, or declaring arrays
};

struct ResIdHolder
{
	ResId id = BAD_RESID;
	ResIdHolder(ResId id_) : id(id_) {}
	~ResIdHolder() { close(); }
	void setId(ResId id_) { id = id_; }
	void close() { drv->destroyResource(id); }
	operator ResId() const { return id; }
};