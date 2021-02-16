#pragma once

#include <Common.h>
#include <Util/ResIdHolder.h>

class PostFx
{
public:
	PostFx();
	void perform();

private:
	ResIdHolder shader;
	ResIdHolder renderState;
};