#include "PostFx.h"

PostFx::PostFx()
{
	ShaderSetDesc shaderDesc("PostFx", "Source/Shaders/PostFx.shader");
	shaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "DefaultPostFxVsFunc";
	shaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "PostFxPS";
	shader = drv->createShaderSet(shaderDesc);

	RenderStateDesc rsDesc;
	rsDesc.depthStencilDesc.depthTest = false;
	rsDesc.depthStencilDesc.depthWrite = false;
	renderState = drv->createRenderState(rsDesc);
}

void PostFx::perform()
{
	PROFILE_SCOPE("PostFx");
	drv->setShader(shader, 0);
	drv->setRenderState(renderState);
	drv->draw(3, 0);
}
