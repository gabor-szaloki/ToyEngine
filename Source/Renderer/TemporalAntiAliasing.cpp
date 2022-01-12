#include "TemporalAntiAliasing.h"

#include <vector>
#include <3rdParty/imgui/imgui.h>
#include <Util/AutoImGui.h>
#include <Driver/ITexture.h>
#include "WorldRenderer.h"

static float halton(const unsigned int base, unsigned int index)
{
	// https://en.wikipedia.org/wiki/Halton_sequence
	float f = 1;
	float r = 0;
	while (index > 0)
	{
		f /= (float)base;
		r += f * (index % base);
		index = (unsigned int)floorf(index / base);
	}
	return r;
}

TemporalAntiAliasing::TemporalAntiAliasing()
{
	ComputeShaderDesc shaderDesc;
	shaderDesc.sourceFilePath = "Source/Shaders/TAA.shader";
	shaderDesc.name = "TAA";
	shaderDesc.shaderFuncName = "TAA";
	taaComputeShader = drv->createComputeShader(shaderDesc);
}

XMFLOAT2 TemporalAntiAliasing::getJitter(unsigned int frame) const
{
	unsigned int haltonFrame = frame % jitterFrameMod + 1; // Halton sequence generator should be indexed from 1
	return enabled ? XMFLOAT2(halton(2, haltonFrame) - 0.5f, halton(3, haltonFrame) - 0.5f) : XMFLOAT2(0.f, 0.f);
}

void TemporalAntiAliasing::perform(const ITexture& current_frame_tex, const ITexture& history_tex, const ITexture& dest_tex)
{
	PROFILE_SCOPE("TAA");

	if (!enabled)
	{
		current_frame_tex.copyResource(dest_tex.getId());
		return;
	}

	drv->setShader(taaComputeShader, 0);
	drv->setTexture(ShaderStage::CS, 0, current_frame_tex.getId());
	drv->setTexture(ShaderStage::CS, 1, history_tex.getId());
	drv->setRwTexture(0, dest_tex.getId());

	drv->dispatch((dest_tex.getDesc().width + 16 - 1) / 16, (dest_tex.getDesc().height + 16 - 1) / 16, 1);

	drv->setTexture(ShaderStage::CS, 0, BAD_RESID);
	drv->setTexture(ShaderStage::CS, 1, BAD_RESID);
	drv->setRwTexture(0, BAD_RESID);
}

void TemporalAntiAliasing::gui()
{
	ImGui::Checkbox("Enabled", &enabled);
	ImGui::SliderInt("Jitter frame modulo", &jitterFrameMod, 1, 64);
}

REGISTER_IMGUI_WINDOW("TAA settings", [] { wr->getTaa().gui(); });