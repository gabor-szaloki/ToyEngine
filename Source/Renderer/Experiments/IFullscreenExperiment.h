#pragma once

class IFullscreenExperiment
{
public:
	virtual ~IFullscreenExperiment() {}
	virtual void onResize(int display_width, int display_height) {};
	virtual void update(float delta_time) {};
	virtual void render(class ITexture& target) = 0;
	virtual void gui() {}
};