#pragma once

#include "DriverCommon.h"

class ITexture
{
public:
	virtual ~ITexture() {};
	virtual const TextureDesc& getDesc() const = 0;
	virtual const ResId& getId() const = 0;
	virtual void* getViewHandle() const = 0;
	virtual void updateData(unsigned int dst_subresource, const IntBox* dst_box, const void* src_data) = 0;
	virtual void copyResource(ResId dst_tex) const = 0;
	virtual void generateMips() = 0;
	virtual bool isStub() = 0;
	virtual void setStub(bool is_stub) = 0;
	virtual void recreate(const TextureDesc& desc_) = 0;
};