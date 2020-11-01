#pragma once

#include "RendererCommon.h"

class PostFx
{
public:
	PostFx();
	void perform();

private:
	ResIdHolder shader;
	ResIdHolder renderState;
};