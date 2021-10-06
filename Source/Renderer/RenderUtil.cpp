#include "RenderUtil.h"

#include <Common.h>
#include <Driver/IDriver.h>
#include <Driver/ITexture.h>

static ResId blit_shader = BAD_RESID;
static ResId blit_linear_to_srgb_shader = BAD_RESID;
static ResId point_sampler = BAD_RESID;
static ResId linear_sampler = BAD_RESID;

void render_util::init()
{
	ShaderSetDesc blitShaderDesc("Blit", "Source/Shaders/Blit.shader");
	blitShaderDesc.shaderFuncNames[(int)ShaderStage::VS] = "DefaultPostFxVsFunc";
	blitShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "BlitPS";
	blit_shader = drv->createShaderSet(blitShaderDesc);
	blitShaderDesc.shaderFuncNames[(int)ShaderStage::PS] = "BlitLinearToSrgb";
	blit_linear_to_srgb_shader = drv->createShaderSet(blitShaderDesc);

	point_sampler = drv->createSampler(SamplerDesc(FILTER_MIN_MAG_MIP_POINT, TexAddr::CLAMP));
	linear_sampler = drv->createSampler(SamplerDesc(FILTER_MIN_MAG_MIP_LINEAR, TexAddr::CLAMP));
}

void render_util::shutdown()
{
	drv->destroyResource(blit_shader);
	drv->destroyResource(blit_linear_to_srgb_shader);
	drv->destroyResource(point_sampler);
	drv->destroyResource(linear_sampler);
}

static void blit_internal(const ITexture& src_tex, const ITexture& dst_tex, ResId shader, ResId sampler)
{
	PROFILE_SCOPE("Blit");

	drv->setView(0, 0, dst_tex.getDesc().width, dst_tex.getDesc().height, 0, 1);
	drv->setShader(shader, 0);
	drv->setRenderTarget(dst_tex.getId(), BAD_RESID);
	drv->setTexture(ShaderStage::PS, 0, src_tex.getId());
	drv->setSampler(ShaderStage::PS, 0, sampler);

	drv->draw(3, 0);

	// TODO: Implement a way to get and restore current RT setup
	drv->setRenderTarget(BAD_RESID, BAD_RESID);
	drv->setTexture(ShaderStage::PS, 0, BAD_RESID);
	drv->setSampler(ShaderStage::PS, 0, BAD_RESID);
}

void render_util::blit(const ITexture& src_tex, const ITexture& dst_tex, bool bilinear)
{
	blit_internal(src_tex, dst_tex, blit_shader, bilinear ? linear_sampler : point_sampler);
}

void render_util::blit_linear_to_srgb(const ITexture& src_tex, const ITexture& dst_tex, bool bilinear)
{
	blit_internal(src_tex, dst_tex, blit_linear_to_srgb_shader, bilinear ? linear_sampler : point_sampler);
}
