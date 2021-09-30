#pragma once

struct DriverSettings
{
	bool vsync = true;
	int fpsLimit = 0;
	unsigned int textureFilteringAnisotropy = 16;
	char shaderRecompileFilter[64] = {};
};