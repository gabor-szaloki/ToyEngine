#pragma once

#include <memory>

#include "RendererCommon.h"

class IBuffer;
class Material;

class MeshRenderer
{
public:
	MeshRenderer(const char* name_, Material* material_, ResId input_layout_id = BAD_RESID);
	void setInputLayout(ResId res_id);
	void loadVertices(unsigned int num_vertices, unsigned int vertex_byte_size, void* data);
	void loadIndices(unsigned int num_indices, void* data);
	void render(RenderPass render_pass);

	XMMATRIX worldTransform = XMMatrixIdentity();

private:
	const char* name = nullptr;
	Material* material = nullptr;
	ResId inputLayoutId = BAD_RESID;
	std::unique_ptr<IBuffer> cb;
	std::unique_ptr<IBuffer> vb;
	std::unique_ptr<IBuffer> ib;
};
