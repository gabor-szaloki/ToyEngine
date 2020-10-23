#pragma once

#include <DirectXMath.h>
#include <Driver/IDriver.h>
#include "AutoImGui.h"

extern IDriver* drv;
extern class WorldRenderer* wr;
extern void create_driver_d3d11();

// TODO: add a capable logger utility
namespace debug
{
	void log(char* msg);
}

// For DirectXMath
using namespace DirectX;