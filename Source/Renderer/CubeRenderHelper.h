#pragma once

#include "Camera.h"

class ITexture;

enum class CubeFace { POSITIVE_X, NEGATIVE_X, POSITIVE_Y, NEGATIVE_Y, POSITIVE_Z, NEGATIVE_Z };

class CubeRenderHelper
{
public:
	void beginRender(const XMVECTOR& camera_pos, ITexture* cube_target, unsigned int mip = 0);
	void setupCamera(CubeFace cube_face);
	const Camera& getCamera() const { return cam; };
	void renderFace(CubeFace cube_face);
	void renderAllFaces();
	void finishRender();

private:
	Camera cam;
	ITexture* cubeTarget;
	unsigned int targetMip;
};

