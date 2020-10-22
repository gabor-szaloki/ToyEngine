#pragma once

#include "DriverCommon.h"

class IBuffer
{
public:
	virtual ~IBuffer() {}
	virtual const BufferDesc& getDesc() const = 0;
	virtual const ResId& getId() const = 0;
	virtual void updateData(const void* src_data) = 0;
};