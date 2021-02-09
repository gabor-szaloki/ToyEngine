#pragma once

#include "PreciseSleep.h"

struct FpsLimiter
{
	std::chrono::steady_clock::time_point frameStart;
	double minFrameTimeSeconds;

	FpsLimiter(int fps_limit)
	{
		frameStart = std::chrono::high_resolution_clock::now();
		minFrameTimeSeconds = fps_limit > 0 ? 1.0 / fps_limit : 0;
	}

	~FpsLimiter()
	{
		double frameTimeSeconds = (std::chrono::high_resolution_clock::now() - frameStart).count() / 1e9;
		if (frameTimeSeconds < minFrameTimeSeconds)
			precise_sleep(minFrameTimeSeconds - frameTimeSeconds);
	}
};