#pragma once

#include "IFullscreenExperiment.h"

#include <memory>
#include <Common.h>
#include <Util/ResIdHolder.h>

class ITexture;

class SlimeSim : public IFullscreenExperiment
{
public:
	SlimeSim(int display_width, int display_height);
	virtual void onResize(int display_width, int display_height) override;
	virtual void update(float delta_time) override;
	virtual void render(class ITexture& target) override;
	virtual void gui() override;

private:
	void initResolutionDependentResources(int display_width, int display_height);
	void closeResolutionDependentResources();
	void initAgentBuffer();
	void clear();
	void reset();

	struct SlimeSimCbData
	{
		unsigned int width;
		unsigned int height;
		unsigned int numAgents;
		float deltaTime;

		float agentMoveSpeed;
		float agentTurnSpeed;
		float evaporateSpeed;
		float diffuseSpeed;

		float sensorOffsetAngle;
		float sensorOffsetDistance;
		int sensorRadius;
		float pad[1];
	} cbData;

	struct Agent
	{
		XMFLOAT2 position;
		float angle;
	};

	int width = 0, height = 0;
	int displayWidth = 0, displayHeight = 0;
	float timeScale = 1.f;
	float resulutionScale;
	std::unique_ptr<IBuffer> cb;
	std::unique_ptr<IBuffer> agentBuffer;
	std::unique_ptr<ITexture> slimeMaps[2];
	ResIdHolder clearShader, updateShader, postProcessShader;
	int slimeMapIndex = 0, postProcessedSlimeMapIndex = 1;
};