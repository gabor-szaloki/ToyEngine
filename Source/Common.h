#pragma once

#include <DirectXMath.h>
#include <3rdParty/plog/Log.h>
#include <Driver/IDriver.h>
#include "AutoImGui.h"

extern IDriver* drv;
extern class WorldRenderer* wr;
extern void create_driver_d3d11();

// For DirectXMath
using namespace DirectX;