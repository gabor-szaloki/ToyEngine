#include "CubeRenderHelper.h"

#include <assert.h>
#include <Common.h>
#include <Driver/IDriver.h>
#include <Driver/ITexture.h>
#include <Renderer/WorldRenderer.h>

void CubeRenderHelper::beginRender(ITexture* cube_target)
{
	cubeTarget = cube_target;
	cam.SetViewParams(XMVectorZero(), XMVectorSet(0, 1, 0, 0), 0, 0);
	float cubeFaceWidth = cube_target->getDesc().width;
	cam.SetProjectionParams(cubeFaceWidth, cubeFaceWidth, XM_PIDIV2, 0.1f, 100.f);
	drv->setView(0, 0, cubeFaceWidth, cubeFaceWidth, 0, 1);
}

static XMFLOAT2 get_pitch_yaw_for_cube_face(CubeFace cube_face)
{
	switch (cube_face)
	{
	case CubeFace::POSITIVE_X:
		return XMFLOAT2(0, XM_PIDIV2);
	case CubeFace::NEGATIVE_X:
		return XMFLOAT2(0, -XM_PIDIV2);
	case CubeFace::POSITIVE_Y:
		return XMFLOAT2(-XM_PIDIV2, 0);
	case CubeFace::NEGATIVE_Y:
		return XMFLOAT2(XM_PIDIV2, 0);
	case CubeFace::POSITIVE_Z:
		return XMFLOAT2(0, 0);
	case CubeFace::NEGATIVE_Z:
		return XMFLOAT2(0, XM_PI);
	default:
		assert(false && "Invalid cube face");
		return XMFLOAT2(0, 0);
	}
}

void CubeRenderHelper::renderFace(CubeFace cube_face)
{
	XMFLOAT2 pitchYaw = get_pitch_yaw_for_cube_face(cube_face);
	cam.SetRotation(pitchYaw.x, pitchYaw.y);
	wr->setCameraForShaders(cam);
	drv->setRenderTarget(cubeTarget->getId(), BAD_RESID, static_cast<unsigned int>(cube_face));
	drv->draw(3, 0);
}

void CubeRenderHelper::renderAllFaces()
{
	for (int i = 0; i < 6; i++)
		renderFace((CubeFace)i);
}

void CubeRenderHelper::finishRender()
{
	drv->setRenderTarget(BAD_RESID, BAD_RESID);
}
