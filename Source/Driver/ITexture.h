#pragma once

#include "DriverCommon.h"

class ITexture
{
public:
	virtual const TextureDesc& getDesc() const = 0;
	virtual const ResId& getId() const = 0;
	virtual void updateData(unsigned int dst_subresource, const IntBox* dst_box, const void* src_data) = 0;
	virtual void generateMips() = 0;
};