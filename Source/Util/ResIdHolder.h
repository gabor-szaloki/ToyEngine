#pragma once

#include <Common.h>
#include <Driver/IDriver.h>

struct ResIdHolder
{
	ResId id;
	ResIdHolder(ResId id_ = BAD_RESID) : id(id_) {}
	~ResIdHolder() { close(); }
	void setId(ResId id_) { id = id_; }
	void close() { if (id != BAD_RESID) { drv->destroyResource(id); id = BAD_RESID; } }
	operator ResId() const { return id; }
	ResIdHolder& operator=(const ResId& id_) { close(); setId(id_); return *this; }
};