#include "MeshRenderer.h"

#include <Driver/IBuffer.h>

#include "Material2.h"

using namespace renderer;

MeshRenderer::MeshRenderer(const char* name_, Material* material_, ResId input_layout_id)
	: name(name_), material(material_), inputLayoutId(input_layout_id)
{
}

void MeshRenderer::setInputLayout(ResId res_id)
{
	inputLayoutId = res_id;
}

void MeshRenderer::loadVertices(unsigned int num_vertices, unsigned int vertex_byte_size, void* data)
{
	BufferDesc vbDesc(name, vertex_byte_size, num_vertices, ResourceUsage::DEFAULT, BIND_VERTEX_BUFFER);
	vbDesc.initialData = data;
	vb.reset(drv->createBuffer(vbDesc));
}

void MeshRenderer::loadIndices(unsigned int num_indices, void* data)
{
	unsigned int indexByteSize = ::get_byte_size_for_texfmt(drv->getIndexFormat());
	BufferDesc ibDesc(name, indexByteSize, num_indices, ResourceUsage::DEFAULT, BIND_INDEX_BUFFER);
	ibDesc.initialData = data;
	ib.reset(drv->createBuffer(ibDesc));
}

void MeshRenderer::render(RenderPass render_pass)
{
	material->set(render_pass);
	drv->setIndexBuffer(ib->getId());
	drv->setVertexBuffer(vb->getId());
}
